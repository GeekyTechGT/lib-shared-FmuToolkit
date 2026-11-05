/// @file validation_report.cpp
/// @brief Implementation of ValidationReport error/warning accumulation.
/// @internal

#include "validation_report.hpp"
#include <utility>

namespace libfmu_toolkit {

void ValidationReport::addError(ValidationCategory cat, std::string msg, int line, int col)
{
    errors.push_back(ValidationError{cat, std::move(msg), line, col});
}

void ValidationReport::addWarning(ValidationCategory cat, std::string msg)
{
    warnings.push_back(ValidationWarning{cat, std::move(msg)});
}

} // namespace libfmu_toolkit
