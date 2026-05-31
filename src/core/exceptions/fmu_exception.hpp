#pragma once

/// @file fmu_exception.hpp
/// @brief Internal exception hierarchy used by the domain and use-case layers.
/// @internal
///
/// These internal exceptions mirror the public hierarchy in
/// `<libfmu_toolkit/exceptions.hpp>`. The PIMPL facade translates them into the
/// corresponding public exception types before they cross the API boundary, so
/// that no internal type ever escapes the library.

#include <stdexcept>
#include <string>

namespace libfmu_toolkit {

/// @brief Base class for all internal lib-fmu-toolkit errors.
/// @req FR-01.5
class FmuException : public std::runtime_error {
public:
    /// @param message Human-readable explanation of the error.
    /// @req FR-01.5
    explicit FmuException(const std::string& message) : std::runtime_error(message) {}
    /// @param message Human-readable explanation of the error.
    /// @req FR-01.5
    explicit FmuException(const char* message)        : std::runtime_error(message) {}
    ~FmuException() override = default;
};

/// @brief Thrown internally when an FMU archive or its shared library cannot be loaded.
/// @req FR-01.5
class FmuLoadException : public FmuException {
public:
    /// @param message Human-readable explanation of the load failure.
    /// @req FR-01.5
    explicit FmuLoadException(const std::string& message) : FmuException(message) {}
    /// @param message Human-readable explanation of the load failure.
    /// @req FR-01.5
    explicit FmuLoadException(const char* message)        : FmuException(message) {}
    ~FmuLoadException() override = default;
};

/// @brief Thrown internally when `modelDescription.xml` fails validation.
/// @req FR-08.3
class FmuValidationException : public FmuException {
public:
    /// @param message Human-readable explanation of the validation failure.
    /// @req FR-08.3
    explicit FmuValidationException(const std::string& message) : FmuException(message) {}
    /// @param message Human-readable explanation of the validation failure.
    /// @req FR-08.3
    explicit FmuValidationException(const char* message)        : FmuException(message) {}
    ~FmuValidationException() override = default;
};

/// @brief Thrown internally when an error occurs during the co-simulation lifecycle.
/// @req FR-04.1
class FmuSimulationException : public FmuException {
public:
    /// @param message Human-readable explanation of the simulation error.
    /// @req FR-04.1
    explicit FmuSimulationException(const std::string& message) : FmuException(message) {}
    /// @param message Human-readable explanation of the simulation error.
    /// @req FR-04.1
    explicit FmuSimulationException(const char* message)        : FmuException(message) {}
    ~FmuSimulationException() override = default;
};

} // namespace libfmu_toolkit
