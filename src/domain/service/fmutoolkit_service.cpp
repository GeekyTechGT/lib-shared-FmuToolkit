#include "fmutoolkit/fmutoolkit.hpp"

#include <utility>

namespace fmutoolkit {

// The implementation class: invisible to consumers, free to change.
class FmuToolkitService::Impl {
public:
    [[nodiscard]] std::string greeting(const std::string& recipient) {
        ++count_;
        return "Hello from FmuToolkit" + (recipient.empty() ? std::string{} : ", " + recipient);
    }

    [[nodiscard]] std::size_t count() const noexcept { return count_; }

private:
    std::size_t count_ = 0;
};

FmuToolkitService::FmuToolkitService()
    : impl_(std::make_unique<Impl>())
{
}

// Defined here, where Impl is a complete type.
FmuToolkitService::~FmuToolkitService() = default;

FmuToolkitService::FmuToolkitService(const FmuToolkitService& other)
    : impl_(std::make_unique<Impl>(*other.impl_))
{
}

FmuToolkitService& FmuToolkitService::operator=(const FmuToolkitService& other) {
    if (this != &other) {
        // Copy-and-swap: strong exception guarantee.
        auto copy = std::make_unique<Impl>(*other.impl_);
        impl_ = std::move(copy);
    }
    return *this;
}

FmuToolkitService::FmuToolkitService(FmuToolkitService&& other) noexcept = default;
FmuToolkitService& FmuToolkitService::operator=(FmuToolkitService&& other) noexcept = default;

// std::unique_ptr does not propagate const: this const member can call a
// non-const Impl method. That is the classic PIMPL constness pitfall - here
// it is deliberate (the greeting counter is bookkeeping, not part of the
// object's logical value), and it is the only such call.
std::string FmuToolkitService::greeting(const std::string& recipient) const {
    return impl_->greeting(recipient);
}

std::size_t FmuToolkitService::greetingCount() const noexcept {
    return impl_->count();
}

} // namespace fmutoolkit
