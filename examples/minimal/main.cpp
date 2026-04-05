#include <yourlib/yourlib.hpp>

#include <iostream>

int main() {
    const yourlib::Greeter greeter{"Hello"};
    std::cout << greeter.greet("example") << '\n';
    return 0;
}
