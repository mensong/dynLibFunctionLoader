#pragma once

#include <vector>
#include <string>
#include <array>
#include <numeric>
#include <utility>
#include <typeinfo>

#include <Windows.h>

#include "Function.hpp"

class Lib
{
public:
	Lib(const std::string& libName);

	~Lib();

	/*
		Calls the given function with the specified parameters.
		
		Can throw std::logic_error or std::invalid_argument
	*/
	template <	typename Return, 
				typename ...Args>
	Return call(const	std::string&	func,
						Args...			args) const;

	template <typename Return>
	Return call(const std::string& func) const;

	/*
		Returns a vector of all exported symbols as pairs of undecoreted and decorated names
	*/
	std::vector<std::pair<std::string, std::string>> getExportedSymbols() const;

	/*
		Returns the name of the loaded library
	*/
	std::string getName() const;

	/*
		Returns a proxy function object binded to an exported function

		Can throw std::logic_error or std::invalid_argument
	*/
	template <	typename Return,
				typename First,
				typename ...Args>
	Function<Return, First, Args...> getFunction(const std::string& func) const;
	
	template <typename Return>
	Function<Return> getFunction(const std::string& func) const;

	/*
		Returns a reference to an exported variable

		Can throw std::logic_error or std::invalid_argument
	*/
	template <typename Type>
	Type& getVar(const std::string& name) const;

private:
	static std::vector<std::string> LoadLibSymbols(const std::string& libraryName);

	static std::string MangleFunction(	const std::string&				func,
										const std::string&				returnType,
										const std::vector<std::string>&	parameters );

	static std::string MangleVar(	const std::string& name,
									const std::string& type );

	static unsigned int LevenshteinDistance(const std::string& str1, 
											const std::string& str2);

	void* internalGetFunction(			std::string					func, 
								const	std::string&				mangledReturnType,
								const	std::vector<std::string>&	parameters) const;

	void* internalGetValue(			std::string		name,
							const	std::string&	type ) const;

	std::string guessSymbolName(const std::string& mangledName) const;
	

	const std::string				m_name;
	const std::vector<std::string>	m_exportedSymbols;
	const HINSTANCE					m_instance;


	template <typename T>
	struct mangledTypeHelper
	{
		static std::string GetRepresentation()
		{
			return typeid(T).raw_name() + 1;
		} 
	};

	template <typename T>
	struct mangledTypeHelper<T&>
	{
		static std::string GetRepresentation()
		{
			return "AA" + std::string( typeid(T).raw_name() + 1 );
		}
	};
};


template <	typename Return, 
			typename ...Args>
Return Lib::call(const	std::string&	func,
						Args...			args) const
{
	return getFunction<Return, Args...>( func )( args... );
}

template <typename Return>
Return Lib::call(const std::string& func) const
{
	return getFunction<Return>(func)();
}

template <	typename Return,
			typename First,
			typename ...Args>
Function<Return, First, Args...> Lib::getFunction(const std::string& func) const
{
	typedef Return(*type)(First, Args...);

	std::array<std::string, 1 + sizeof...(Args)> decoratedParamInfo
	{{
		// Skip the dot character
		mangledTypeHelper<First>::GetRepresentation(),
		(mangledTypeHelper<Args>::GetRepresentation())...
	}};

	try
	{
		return Function<Return, First, Args...>(reinterpret_cast<type>(internalGetFunction(	func, 
																							mangledTypeHelper<Return>::GetRepresentation(), 
																							std::vector<std::string>(	decoratedParamInfo.begin(), 
																														decoratedParamInfo.end() ) ) ) );
	}
	catch( std::invalid_argument& ex )
	{
		std::array<std::string, 1 + sizeof...(Args)> humanReadableParamInfo
		{{
			typeid(First).name(),
			typeid(Args).name()...
		}};

		throw std::invalid_argument( std::string(ex.what()) + std::accumulate(	humanReadableParamInfo.begin(),
																				humanReadableParamInfo.end(),
																				std::string(),
																				[](const std::string& s1, const std::string& s2)
																				{
																					return s1.empty() ? s2 : s1 + ", " + s2;
																				}) );
	}
}

template <typename Return>
Function<Return> Lib::getFunction(const std::string& func) const
{
	typedef Return(*type)();

	try
	{
		return Function<Return>(reinterpret_cast<type>(internalGetFunction(	func, 
																			mangledTypeHelper<Return>::GetRepresentation(), 
																			std::vector<std::string>())) );
	}
	catch( std::invalid_argument& ex )
	{
		throw std::invalid_argument( std::string(ex.what()) + "void" );
	}
}

template <typename Type>
Type& Lib::getVar(const std::string& name) const
{
	typedef Type& return_type;

	try
	{
		return return_type( *reinterpret_cast<Type*>(internalGetValue(	name,
																		mangledTypeHelper<Type>::GetRepresentation() ) ) );
	}
	catch( std::invalid_argument& ex )
	{
		throw std::invalid_argument( std::string(ex.what()) + typeid(Type).name() );
	}
}
