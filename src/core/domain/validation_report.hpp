#pragma once

// ValidationCategory is part of the public API — defined in types.hpp.
// Including the public header here ensures the internal domain uses the
// same enum (same underlying type, same enumerators) as the public API.
#include <libfmu_toolkit/types.hpp>

#include <string>
#include <vector>

namespace libfmu_toolkit {

struct ValidationError {
    ValidationCategory category;
    std::string message;
    int line{-1};
    int column{-1};
};

struct ValidationWarning {
    ValidationCategory category;
    std::string message;
};

struct ValidationReport {
    bool passed{false};
    std::vector<ValidationError> errors;
    std::vector<ValidationWarning> warnings;

    [[nodiscard]] bool hasErrors() const noexcept { return !errors.empty(); }
    [[nodiscard]] bool hasWarnings() const noexcept { return !warnings.empty(); }
    void addError(ValidationCategory cat, std::string msg, int line = -1, int col = -1);
    void addWarning(ValidationCategory cat, std::string msg);
};

} // namespace libfmu_toolkit
