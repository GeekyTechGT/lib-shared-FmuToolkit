#include "fmutoolkit/fmutoolkit.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    const fmutoolkit::FmuToolkitService service;
    const std::string message = service.greeting("examples");

    if (message.empty()) {
        std::cerr << "FmuToolkit example failed: empty greeting.\n";
        return EXIT_FAILURE;
    }

    std::cout << message << '\n';
    return EXIT_SUCCESS;
}
