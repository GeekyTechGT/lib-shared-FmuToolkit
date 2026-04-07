# Examples

The examples directory demonstrates consumer-facing use of `your_library`.

## `minimal/`

This example demonstrates the installed-package workflow expected for downstream applications:

- include the public header `#include <yourlib/yourlib.hpp>`
- consume the exported target `your_library::your_library`
- compile against the installable package config rather than private implementation files

Consumer CMake integration:

```cmake
find_package(your_library CONFIG REQUIRED)
target_link_libraries(app PRIVATE your_library::your_library)
```
