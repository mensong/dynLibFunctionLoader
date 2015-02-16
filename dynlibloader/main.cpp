#include <string>
#include <iostream>
#include <typeinfo>

#include "Lib.hpp"

#define TEST(a)\
	std::cout << "\nTEST:\t[" #a "] \n\twas " << std::boolalpha << (a) << std::endl

int main()
{
	try
	{
		Lib lib("testDll.dll");
		
		// poor mans' unit tests
		TEST(42 == lib.call<int>("foo_int"));
		TEST(33 == lib.call<int>("foo_1param", 33));
		TEST(5 == lib.call<int>("foo_2params", 2, 3));
		TEST(std::string("hello from dll") == lib.call<std::string>("bar"));
		TEST(std::string("hello from dll 2") == lib.call<std::string>("bar", 2));

		auto func = lib.getFunction<int, int, int>("foo_2params");
		auto res = func(2, 3);

		try
		{
			lib.call<void>("foo_void");
		}
		catch( int i )
		{
			std::cout << "\nTEST:\t[lib.call<void(\"foo_void\")] \n\twas " << std::boolalpha << (i == 42) << std::endl;
		}

		lib.call<void>("fooIntRef", std::ref(res));

		std::cout << "After fooIntRef: " << res << std::endl;
	}
	catch( std::exception& ex )
	{
		std::cout << "Exception: " << ex.what() << std::endl;
	}
}