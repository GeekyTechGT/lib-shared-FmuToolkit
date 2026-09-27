#pragma once

#include "fmutoolkit/fmutoolkit_export.hpp"

#include <cstddef>
#include <memory>
#include <string>

namespace fmutoolkit {

/// Public API of FmuToolkit, implemented with the PIMPL idiom
/// ("pointer to implementation", a.k.a. compilation firewall).
///
/// Only this interface is visible to consumers: every data member lives in
/// `Impl`, defined in src/. Private state can therefore change without
/// recompiling consumers and - for the shared library - without breaking the
/// ABI, and none of the implementation's headers leak into client code.
///
/// Value semantics: copies are deep and independent; moves are noexcept. A
/// moved-from object may only be assigned to or destroyed.
class FMUTOOLKIT_EXPORT FmuToolkitService {
public:
    FmuToolkitService();
    ~FmuToolkitService();

    FmuToolkitService(const FmuToolkitService& other);
    FmuToolkitService& operator=(const FmuToolkitService& other);
    FmuToolkitService(FmuToolkitService&& other) noexcept;
    FmuToolkitService& operator=(FmuToolkitService&& other) noexcept;

    /// Greeting for `recipient`; an empty recipient greets the library itself.
    [[nodiscard]] std::string greeting(const std::string& recipient = {}) const;

    /// Number of greetings produced by this instance (state held by Impl).
    [[nodiscard]] std::size_t greetingCount() const noexcept;

private:
    // Declared, not defined: std::unique_ptr only needs a complete type
    // where it is destroyed, which is why every special member function is
    // defined out of line in the .cpp, after Impl is complete.
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace fmutoolkit
