#pragma once

#include "yourlib/export.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace yourlib {

class YOUR_LIBRARY_API Greeter final {
public:
    explicit Greeter(std::string prefix = "Hello");
    ~Greeter();

    Greeter(Greeter&&) noexcept;
    Greeter& operator=(Greeter&&) noexcept;

    Greeter(const Greeter&);
    Greeter& operator=(const Greeter&);

    [[nodiscard]] std::string greet(std::string_view name) const;
    void set_prefix(std::string prefix);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace yourlib
