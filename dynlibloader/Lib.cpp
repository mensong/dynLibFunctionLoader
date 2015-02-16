#include "Lib.hpp"

#include <ciso646>

#include <exception>
#include <algorithm>
#include <sstream>

#include <ImageHlp.h>

#pragma comment(lib, "Imagehlp.lib")

Lib::Lib(const std::string& libName):
	m_name( libName ),
	m_functions( LoadLibFunctions( libName ) ),
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

std::vector<std::string> Lib::LoadLibFunctions(const std::string& libraryName)
{
	_LOADED_IMAGE				loadedImage;
	std::string					functionName;
	std::vector<std::string>	dllFunctions;

    if( MapAndLoad(	libraryName.c_str(), 
					nullptr,
					&loadedImage,
					true,
					true) )
    {
		unsigned long directorySize	= 0;

        auto imageExportDirectory = static_cast<_IMAGE_EXPORT_DIRECTORY*>(ImageDirectoryEntryToData(loadedImage.MappedAddress,
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
                functionName = static_cast<char*>(ImageRvaToVa(	loadedImage.FileHeader, 
																loadedImage.MappedAddress,
																nameRVA[i],
																nullptr ));

				dllFunctions.push_back(functionName);
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

			col[j+1] = std::min(std::min(	prevCol[1 + j] + 1, 
											col[j] + 1 ),
								prevCol[j] + ( str1[i] == str2[j] ? 0 : 1 ) );
		}

		col.swap(prevCol);
	}

	return prevCol[len2];
}

void* Lib::internalGetFunction(			std::string					func, 
								const	std::string&				mangledReturnType, 
								const	std::vector<std::string>&	parameters) const
{
	if( not m_instance )
	{
		throw std::logic_error("No library loaded !");
	}

	if( m_functions.empty() )
	{
		throw std::logic_error("Library " + m_name + " has no exported functions !");
	}

	const auto mangledFuncName = MangleName(func, 
											mangledReturnType,
											parameters );

	std::vector< std::pair< unsigned long, std::string > > levDistanceMangledNamePairVector;
	levDistanceMangledNamePairVector.reserve( m_functions.size() );

	for(const auto& function : m_functions)
	{
		const auto levDistance = LevenshteinDistance(function, mangledFuncName);

		levDistanceMangledNamePairVector.emplace_back(	levDistance, 
														function );

		if( levDistance == 0 )
		{
			break;
		}
	}

	if( levDistanceMangledNamePairVector.size() == m_functions.size() )
	{
		const auto minElement = std::min_element(	levDistanceMangledNamePairVector.begin(), 
													levDistanceMangledNamePairVector.end() );

		if( minElement != levDistanceMangledNamePairVector.end() )
		{
			func = minElement->second;
		}
	}
	else
	{
		func = levDistanceMangledNamePairVector.back().second;
	}

	void* funcPtr = GetProcAddress( m_instance, func.data() );

	if( not funcPtr )
	{
		throw std::invalid_argument("Cannot find function '" + func + "' that accepts as parameters: ");
	}

	return funcPtr;
}

std::vector<std::pair<std::string, std::string>> Lib::getExportedFunctions() const
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

	std::vector<std::pair<std::string, std::string>> functions;

	for(const auto& decoratedFuncName : m_functions)
	{
		functions.emplace_back( UndecorateCppFunctionName( decoratedFuncName ), 
								decoratedFuncName );
	}

	return functions;
}

std::string Lib::getName() const
{
	return m_name;
}

std::string Lib::MangleName(const std::string&				func,
							const std::string&				returnType,
							const std::vector<std::string>& parameters )
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