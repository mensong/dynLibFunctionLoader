#include "Lib.hpp"

#include <ciso646>

#include <exception>
#include <algorithm>
#include <sstream>

#include <ImageHlp.h>

#pragma comment(lib, "Imagehlp.lib")

Lib::Lib(const std::string& libName):
	m_name( libName ),
	m_exportedSymbols( LoadLibSymbols( libName ) ),
	m_instance( LoadLibrary( libName.data() ) )
{
	if( not m_instance )
	{
		throw std::invalid_argument("Cannot find " + libName);
	}
}

Lib::~Lib()
{
	if( m_instance )
	{
		FreeLibrary( m_instance );
	}
}

std::vector<std::string> Lib::LoadLibSymbols(const std::string& libraryName)
{
	LOADED_IMAGE				loadedImage;
	std::string					symbolName;
	std::vector<std::string>	dllFunctions;

    if( MapAndLoad(	libraryName.c_str(), 
					nullptr,
					&loadedImage,
					true,
					true) )
    {
		auto directorySize = 0ul;

        auto imageExportDirectory = static_cast<IMAGE_EXPORT_DIRECTORY*>(ImageDirectoryEntryToData(loadedImage.MappedAddress,
																									false,
																									IMAGE_DIRECTORY_ENTRY_EXPORT,
																									&directorySize ) );

        if (imageExportDirectory != nullptr)
        {
            auto nameRVA = static_cast<DWORD*>(ImageRvaToVa(	loadedImage.FileHeader,
																loadedImage.MappedAddress,
																imageExportDirectory->AddressOfNames,
																nullptr ));

            for(size_t i = 0; i < imageExportDirectory->NumberOfNames; i++)
            {
                symbolName = static_cast<char*>(ImageRvaToVa(	loadedImage.FileHeader, 
																loadedImage.MappedAddress,
																nameRVA[i],
																nullptr ));

				dllFunctions.push_back(symbolName);
            }
        }

        UnMapAndLoad( &loadedImage );
    }

	return dllFunctions;
}

unsigned int Lib::LevenshteinDistance(const std::string& str1, const std::string& str2)
{
	const auto len1 = str1.length();
	const auto len2 = str2.length();

	std::vector<unsigned int> col(len2 + 1);
	std::vector<unsigned int> prevCol(len2 + 1);
 
	for (unsigned int i = 0; i < prevCol.size(); i++)
	{
		prevCol[i] = i;
	}

	for (unsigned int i = 0; i < len1; i++) 
	{
		col[0] = i + 1;

		for (unsigned int j = 0; j < len2; j++)
		{
			#ifdef min
				#undef min
			#endif
		
			col[j + 1] = std::min(	std::min(	prevCol[1 + j] + 1, 
												col[j] + 1 ),
									prevCol[j] + ( str1[i] == str2[j] ? 0 : 1 ) );
		}

		col.swap(prevCol);
	}

	return prevCol[ len2 ];
}

void* Lib::internalGetFunction(			std::string					func, 
								const	std::string&				mangledReturnType, 
								const	std::vector<std::string>&	parameters) const
{
	if( not m_instance )
	{
		throw std::logic_error("No library loaded !");
	}

	if( m_exportedSymbols.empty() )
	{
		throw std::logic_error("Library " + m_name + " has no exported symbols !");
	}

	func = guessSymbolName( MangleFunction(	func, 
											mangledReturnType,
											parameters ) );

	auto funcPtr = GetProcAddress(	m_instance,
									func.data() );

	if( not funcPtr )
	{
		throw std::invalid_argument("Cannot find function '" + func + "' that accepts as parameters: ");
	}

	return funcPtr;
}

std::vector<std::pair<std::string, std::string>> Lib::getExportedSymbols() const
{
	static const auto UndecorateCppFunctionName = [](const std::string& functionName)
	{
		char outputStr[2048] = {0};

		UnDecorateSymbolName(	functionName.data(),
								outputStr,
								2048,
								UNDNAME_NAME_ONLY );

		return std::string( outputStr );
	};

	std::vector<std::pair<std::string, std::string>> symbols;

	for(const auto& decoratedSymbolName : m_exportedSymbols)
	{
		symbols.emplace_back(	UndecorateCppFunctionName( decoratedSymbolName ), 
								decoratedSymbolName );
	}

	return symbols;
}

std::string Lib::getName() const
{
	return m_name;
}

std::string Lib::MangleFunction(	const std::string&				func,
									const std::string&				returnType,
									const std::vector<std::string>&	parameters )
{
	std::stringstream ss;

	ss	<< "?"
		<< func
		<< "@@YA"
		<< returnType
		<< std::accumulate(	parameters.begin(),
							parameters.end(),
							std::string() );

	if( parameters.empty() )
	{
		ss << "X";
	}
	else
	{
		ss << "@";
	}

	ss << "Z";

	return ss.str();
}

std::string Lib::MangleVar(	const std::string& name,
							const std::string& type )
{
	std::stringstream ss;

	ss	<< "?"
		<< name
		<< "@@3"
		<< type
		<< "A";

	return ss.str();
}

void* Lib::internalGetValue(		std::string		name,
							const	std::string&	type ) const
{
	if( not m_instance )
	{
		throw std::logic_error("No library loaded !");
	}

	if( m_exportedSymbols.empty() )
	{
		throw std::logic_error("Library " + m_name + " has no exported symbols !");
	}

	name = guessSymbolName( MangleVar(	name, 
										type ) );

	auto varPtr = GetProcAddress(	m_instance,
									name.data() );

	if( not varPtr )
	{
		throw std::invalid_argument("Cannot find symbol '" + name + "' of type ");
	}

	return varPtr;
}

std::string Lib::guessSymbolName(const std::string& mangledName) const
{
	std::vector< std::pair< unsigned long, std::string > > levDistanceMangledNamePairVector;
	levDistanceMangledNamePairVector.reserve( m_exportedSymbols.size() );

	for(const auto& symbol : m_exportedSymbols)
	{
		const auto levDistance = LevenshteinDistance(	symbol, 
														mangledName	);

		levDistanceMangledNamePairVector.emplace_back(	levDistance, 
														symbol );

		// stop at exact match
		if( levDistance == 0 )
		{
			break;
		}
	}

	if( levDistanceMangledNamePairVector.size() == m_exportedSymbols.size() )
	{
		const auto minElement = std::min_element(	levDistanceMangledNamePairVector.begin(), 
													levDistanceMangledNamePairVector.end() );

		if( minElement != levDistanceMangledNamePairVector.end() )
		{
			return minElement->second;
		}
	}
	else
	{
		return levDistanceMangledNamePairVector.back().second;
	}

	return mangledName;
}