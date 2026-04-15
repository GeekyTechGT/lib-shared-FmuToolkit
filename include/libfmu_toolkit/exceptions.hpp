/// @file exceptions.hpp
/// @brief Public exception hierarchy for lib-fmu-toolkit.
///
/// All exceptions derive from FmuToolkitException, which in turn derives from
/// `std::runtime_error`. Callers can catch the base type to handle any
/// lib-fmu-toolkit error, or specific derived types for finer-grained handling.
#pragma once

#include "export.hpp"

#include <stdexcept>
#include <string>

namespace libfmu_toolkit {

/// @brief Base class for all lib-fmu-toolkit runtime errors.
///
/// Derives from `std::runtime_error`. The error message is accessible via
/// `what()`.
class LIB_FMU_TOOLKIT_API FmuToolkitException : public std::runtime_error {
public:
    /// @brief Constructs the exception with a descriptive message.
    /// @param message Human-readable explanation of the error.
    explicit FmuToolkitException(const std::string& message) : std::runtime_error(message) {}

    /// @brief Constructs the exception with a C-string message.
    /// @param message Human-readable explanation of the error.
    explicit FmuToolkitException(const char* message) : std::runtime_error(message) {}

    ~FmuToolkitException() override = default;
};

/// @brief Thrown when an FMU cannot be loaded.
///
/// Raised by FmuEngine::load() when the FMU archive cannot be opened or
/// extracted, or when the platform shared library inside the archive fails to
/// load or its FMI symbols cannot be resolved.
class LIB_FMU_TOOLKIT_API FmuLoadError : public FmuToolkitException {
public:
    /// @param message Human-readable explanation of the load failure.
    explicit FmuLoadError(const std::string& message) : FmuToolkitException(message) {}
    /// @param message Human-readable explanation of the load failure.
    explicit FmuLoadError(const char* message) : FmuToolkitException(message) {}
    ~FmuLoadError() override = default;
};

/// @brief Thrown when `modelDescription.xml` fails validation.
///
/// Raised internally by use-case code when the XML document does not satisfy
/// structural, XSD schema, or semantic constraints. FmuEngine::validate() does
/// **not** throw this exception — it returns a ValidationResult instead.
class LIB_FMU_TOOLKIT_API FmuValidationError : public FmuToolkitException {
public:
    /// @param message Human-readable explanation of the validation failure.
    explicit FmuValidationError(const std::string& message) : FmuToolkitException(message) {}
    /// @param message Human-readable explanation of the validation failure.
    explicit FmuValidationError(const char* message) : FmuToolkitException(message) {}
    ~FmuValidationError() override = default;
};

/// @brief Thrown when an error occurs during the FMU co-simulation lifecycle.
///
/// Raised by FmuEngine::simulate(), FmuEngine::openSession(), FmuSession::doStep(),
/// FmuSession::setValue(), and FmuSession::getValue() when an FMI function
/// returns a fatal status code or an SEH exception is raised inside the FMU DLL.
class LIB_FMU_TOOLKIT_API FmuSimulationError : public FmuToolkitException {
public:
    /// @param message Human-readable explanation of the simulation error.
    explicit FmuSimulationError(const std::string& message) : FmuToolkitException(message) {}
    /// @param message Human-readable explanation of the simulation error.
    explicit FmuSimulationError(const char* message) : FmuToolkitException(message) {}
    ~FmuSimulationError() override = default;
};

} // namespace libfmu_toolkit
