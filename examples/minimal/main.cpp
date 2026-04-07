#include <yourlib/yourlib.hpp>

#include <iostream>

int main() {
    const yourlib::LibraryInfo info;
    std::cout << info.describe("minimal_example") << '
';
    return 0;
}
