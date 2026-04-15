#pragma once
#include "core/domain/fmu_package.hpp"
#include "core/domain/simulation_config.hpp"
#include <cstdint>
#include <variant>

namespace libfmu_toolkit {

/// @brief FMI value variant: one of double, int64_t, bool, or std::string.
///
/// Used for get/set operations in ISimulationBackend. Mirrors the public
/// `OutputValue` / `ParameterValue` types without depending on the public API.
using FmiValue = std::variant<double, int64_t, bool, std::string>;

/// @brief Port interface for FMU co-simulation backends.
///
/// Implemented by StandaloneSimulationAdapter. Inject an alternative in tests
/// by passing a custom implementation to FmuSession::Impl's injection constructor.
class ISimulationBackend {
public:
    virtual ~ISimulationBackend() = default;

    /// @brief Instantiates and initialises the FMU.
    ///
    /// Drives the FMI lifecycle through `fmi*Instantiate`, `fmi*SetupExperiment`
    /// (FMI 2.0) or `fmi*EnterInitializationMode` (FMI 3.0), parameter injection,
    /// and `fmi*ExitInitializationMode`.
    ///
    /// @param pkg     The loaded FMU package (provides function-pointer tables).
    /// @param config  Simulation configuration.
    virtual void initialize(const FmuPackage& pkg, const SimulationConfig& config) = 0;

    /// @brief Writes a value to an FMU variable by value reference.
    ///
    /// @param valueReference  FMI value reference identifying the variable.
    /// @param dtype           Data type of the variable.
    /// @param value           Value to write.
    virtual void setInput(uint32_t valueReference, DataType dtype, const FmiValue& value) = 0;

    /// @brief Reads a value from an FMU variable by value reference.
    ///
    /// @param valueReference  FMI value reference identifying the variable.
    /// @param dtype           Data type of the variable.
    /// @returns               Current value of the variable.
    virtual FmiValue getOutput(uint32_t valueReference, DataType dtype) = 0;

    /// @brief Advances the FMU by one communication step.
    ///
    /// @param currentTime  Current simulation time.
    /// @param stepSize     Duration of this communication step.
    /// @returns `true` on success, `false` if the FMU requests early termination.
    virtual bool doStep(double currentTime, double stepSize) = 0;

    /// @brief Terminates the FMU and releases the FMI component.
    ///
    /// Calls `fmi*Terminate` followed by `fmi*FreeInstance`.
    virtual void terminate() = 0;

    /// @brief Returns `true` if initialize() has been called successfully.
    virtual bool isInitialized() const noexcept = 0;

protected:
    ISimulationBackend() = default;
};

} // namespace libfmu_toolkit
