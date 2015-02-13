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
						Args...			args);

	template <typename Return>
	Return call(const std::string& func);

	/*
		Returns a vector of all exported functions as pairs of undecoreted and decorated names
	*/
	std::vector<std::pair<std::string, std::string>> getExportedFunctions() const;

	/*
		Returns the name of the loaded library
	*/
	std::string getName() const;

	template <	typename Return,
				typename First,
				typename ...Args>
	Function<Return, First, Args...> getFunction(const std::string& func);
	
	template <typename Return>
	Function<Return> getFunction(const std::string& func);

private:
	static std::vector<std::string> LoadLibFunctions(const std::string& libraryName);

	static std::string MangleName(	const std::string&				func,
									const std::string&				returnType,
									const std::vector<std::string>& parameters );

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

	void* internalGetFunction(			std::string					func, 
								const	std::string&				mangledReturnType,
								const	std::vector<std::string>&	parameters) const;
	
	const std::string				m_name;
	const std::vector<std::string>	m_functions;
	const HINSTANCE					m_instance;
};


template <	typename Return, 
			typename ...Args>
Return Lib::call(const	std::string&	func,
						Args...			args)
{
	return getFunction<Return, Args...>( func )( args... );
}

template <typename Return>
Return Lib::call(const std::string& func)
{
	return getFunction<Return>(func)();
}

template <	typename Return,
			typename First,
			typename ...Args>
Function<Return, First, Args...> Lib::getFunction(const std::string& func)
{
	typedef Return(*type)(First, Args...);

	std::array<std::string, 1 + sizeof...(Args)> decoratedParamInfo
	{{
		// Skip the dot character
		//typeid(First).raw_name() + 1,
		//(typeid(Args).raw_name() + 1)...
		mangledTypeHelper<First>::GetRepresentation(),
		(mangledTypeHelper<Args>::GetRepresentation())...
	}};

	type t = nullptr;

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
Function<Return> Lib::getFunction(const std::string& func)
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