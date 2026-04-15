#pragma once

#include <stdexcept>
#include <string>

namespace libfmu_toolkit {

class FmuException : public std::runtime_error {
public:
    explicit FmuException(const std::string& message) : std::runtime_error(message) {}
    explicit FmuException(const char* message)        : std::runtime_error(message) {}
    ~FmuException() override = default;
};

class FmuLoadException : public FmuException {
public:
    explicit FmuLoadException(const std::string& message) : FmuException(message) {}
    explicit FmuLoadException(const char* message)        : FmuException(message) {}
    ~FmuLoadException() override = default;
};

class FmuValidationException : public FmuException {
public:
    explicit FmuValidationException(const std::string& message) : FmuException(message) {}
    explicit FmuValidationException(const char* message)        : FmuException(message) {}
    ~FmuValidationException() override = default;
};

class FmuSimulationException : public FmuException {
public:
    explicit FmuSimulationException(const std::string& message) : FmuException(message) {}
    explicit FmuSimulationException(const char* message)        : FmuException(message) {}
    ~FmuSimulationException() override = default;
};

} // namespace libfmu_toolkit
