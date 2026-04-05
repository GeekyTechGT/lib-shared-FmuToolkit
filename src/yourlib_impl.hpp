#pragma once

#include "yourlib/yourlib.hpp"

#include <string>
#include <string_view>

namespace yourlib {

class Greeter::Impl final {
public:
    explicit Impl(std::string prefix_value);

    [[nodiscard]] std::string greet(std::string_view name) const;
    void set_prefix(std::string prefix_value);

private:
    std::string prefix_;
};

}  // namespace yourlib
