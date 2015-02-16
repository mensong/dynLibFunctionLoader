## dynLibFunctionLoader

C++11 library that makes loading functions safely from external libraries (.dll or .exe) a walk in the park

### Example

Say we want to load and call a function `int test(int a) { return a; }` from `test.dll`

```C++
#include <iostream>

#include "Lib.hpp"

int main()
{
    Lib test("test.dll");

    std::cout << test.call<int>("test", 42) << std::endl;
    /*                      ^     ^     ^
                            |     |     |
                            |     |     |_ function paramater
                            |     |_ function name
                            |_ function return type
    */
}
```

Storing a function for a later call is also possible

```C++
auto testFunc = test.getFunction<int, int>("test");
/*                                ^    ^      ^
                                  |    |      |
                                  |    |      |_ function name
                                  |    |_ function parameter types
                                  |_ function return value
*/
```

### How it works
When calling `Lib.call<ReturnType>("FunctionName")` the library will try to do the same thing (well, only 1 of the tings) the compiler did when building the `.dll` or `.exe` and that is name mangling.
After name mangling is done it computes the Levenshtein distance of the result and all of the exported functions and then chooses the one with the smallest distance.

### Tested on
- Visual Studio 2012 Express with Nov CTP update.

### Known issues
- Reference paramters do not work ( Workaround: use `std::reference_wrapper` or pointers )
