#include "yourlib/yourlib.hpp"

#include "yourlib_impl.hpp"

#include <utility>

namespace yourlib {

Greeter::Impl::Impl(std::string prefix_value)
    : prefix_(std::move(prefix_value)) {}

std::string Greeter::Impl::greet(std::string_view name) const {
    return prefix_ + ", " + std::string{name} + "!";
}

void Greeter::Impl::set_prefix(std::string prefix_value) {
    prefix_ = std::move(prefix_value);
}

Greeter::Greeter(std::string prefix)
    : impl_(std::make_unique<Impl>(std::move(prefix))) {}

Greeter::~Greeter() = default;
Greeter::Greeter(Greeter&&) noexcept = default;
Greeter& Greeter::operator=(Greeter&&) noexcept = default;

Greeter::Greeter(const Greeter& other)
    : impl_(std::make_unique<Impl>(*other.impl_)) {}

Greeter& Greeter::operator=(const Greeter& other) {
    if (this != &other) {
        impl_ = std::make_unique<Impl>(*other.impl_);
    }
    return *this;
}

std::string Greeter::greet(std::string_view name) const {
    return impl_->greet(name);
}

void Greeter::set_prefix(std::string prefix) {
    impl_->set_prefix(std::move(prefix));
}

}  // namespace yourlib
