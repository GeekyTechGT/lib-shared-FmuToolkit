#pragma once

/// @file log_category.hpp
/// @brief Internal domain entity for an FMI `<LogCategories>` entry.
/// @internal

#include <string>

namespace libfmu_toolkit {

/// @brief A single FMI log category declared under `<LogCategories>`.
///
/// FMUs advertise the diagnostic categories they emit so that a master can
/// selectively enable them via `fmi*SetDebugLogging`.
/// @req FR-01.2
struct LogCategory {
    std::string name;        ///< Category name as declared in the XML (the identity key).
    std::string description; ///< Human-readable description (may be empty).

    /// @brief Equality by category name.
    /// @req FR-01.2
    [[nodiscard]] bool operator==(const LogCategory& other) const noexcept {
        return name == other.name;
    }

    /// @brief Negation of operator==.
    /// @req FR-01.2
    [[nodiscard]] bool operator!=(const LogCategory& other) const noexcept {
        return !(*this == other);
    }
};

} // namespace libfmu_toolkit
