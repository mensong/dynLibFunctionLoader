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
