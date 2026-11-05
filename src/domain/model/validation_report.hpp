#pragma once

/// @file validation_report.hpp
/// @brief Internal domain aggregate of three-phase FMU validation findings.
/// @internal
///
/// Internal counterpart of the public ValidationResult. The use-case layer
/// accumulates errors and warnings here; the PIMPL facade maps it to the public
/// ValidationResult. Implements requirements FR-03.7 and FR-03.8 together with
/// FmuValidator.

// ValidationCategory is part of the public API — defined in types.hpp.
// Including the public header here ensures the internal domain uses the
// same enum (same underlying type, same enumerators) as the public API.
#include <libfmu_toolkit/types.hpp>

#include <string>
#include <vector>

namespace libfmu_toolkit {

/// @brief An error-level validation finding (causes the report to fail).
/// @req FR-03.1
struct ValidationError {
    ValidationCategory category;  ///< Phase that produced this finding.
    std::string message;          ///< Human-readable description.
    int line{-1};                 ///< Source line in the XML (-1 = unknown).
    int column{-1};               ///< Source column in the XML (-1 = unknown).
};

/// @brief A warning-level validation finding (does not affect the verdict).
/// @req FR-03.1
struct ValidationWarning {
    ValidationCategory category;  ///< Phase that produced this finding.
    std::string message;          ///< Human-readable description.
};

/// @brief Aggregated result of the three validation phases.
/// @req FR-03.7
struct ValidationReport {
    bool passed{false};                       ///< `true` if no errors were recorded.
    std::vector<ValidationError> errors;      ///< Error-level findings.
    std::vector<ValidationWarning> warnings;  ///< Warning-level findings.

    /// @brief Returns `true` if at least one error was recorded.
    /// @req FR-03.8
    [[nodiscard]] bool hasErrors() const noexcept { return !errors.empty(); }
    /// @brief Returns `true` if at least one warning was recorded.
    /// @req FR-03.1
    [[nodiscard]] bool hasWarnings() const noexcept { return !warnings.empty(); }

    /// @brief Appends an error-level finding.
    /// @param cat  Validation phase that produced the finding.
    /// @param msg  Human-readable description.
    /// @param line Source line in the XML (-1 if not applicable).
    /// @param col  Source column in the XML (-1 if not applicable).
    /// @req FR-03.7
    void addError(ValidationCategory cat, std::string msg, int line = -1, int col = -1);

    /// @brief Appends a warning-level finding.
    /// @param cat  Validation phase that produced the finding.
    /// @param msg  Human-readable description.
    /// @req FR-03.7
    void addWarning(ValidationCategory cat, std::string msg);
};

} // namespace libfmu_toolkit
