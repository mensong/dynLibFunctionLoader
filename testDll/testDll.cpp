// testDll.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include <string>

#define export __declspec(dllexport)

export int foo_int()
{
	return 42;
}

export void foo_void()
{
	throw 42;
}

export int foo_1param(int i)
{
	return i;
}

export int foo_2params(int a, int b)
{
	return a + b;
}

export std::string bar()
{
	return "hello from dll";
}

export std::string bar(int i)
{
	return "hello from dll " + std::to_string(i);
}

export int testPtrFunc(int* ptr)
{
	return *ptr;
}