#pragma once

/// @file i_simulation_backend.hpp
/// @brief Port interface for FMU co-simulation backends.
/// @internal

#include "domain/exception/fmu_exception.hpp"
#include "domain/model/fmu_package.hpp"
#include "domain/model/simulation_config.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace libfmu_toolkit {

/// @brief FMI value variant: one of double, int64_t, bool, or std::string.
///
/// Used for get/set operations in ISimulationBackend. Mirrors the public
/// `OutputValue` / `ParameterValue` types without depending on the public API.
/// @req FR-04.1
using FmiValue = std::variant<double, int64_t, bool, std::string>;

/// @brief Port interface for FMU co-simulation backends.
///
/// Implemented by StandaloneSimulationAdapter. Inject an alternative in tests
/// by passing a custom implementation to FmuSession::Impl's injection constructor.
/// @req FR-04.1
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
    /// @req FR-04.1
    virtual void initialize(const FmuPackage& pkg, const SimulationConfig& config) = 0;

    /// @brief Writes a value to an FMU variable by value reference.
    ///
    /// @param valueReference  FMI value reference identifying the variable.
    /// @param dtype           Data type of the variable.
    /// @param value           Value to write.
    /// @req FR-04.3
    virtual void setInput(uint32_t valueReference, DataType dtype, const FmiValue& value) = 0;

    /// @brief Reads a value from an FMU variable by value reference.
    ///
    /// @param valueReference  FMI value reference identifying the variable.
    /// @param dtype           Data type of the variable.
    /// @returns               Current value of the variable.
    /// @req FR-04.6
    virtual FmiValue getOutput(uint32_t valueReference, DataType dtype) = 0;

    /// @brief Advances the FMU by one communication step.
    ///
    /// @param currentTime  Current simulation time.
    /// @param stepSize     Duration of this communication step.
    /// @returns `true` on success, `false` if the FMU requests early termination.
    /// @req FR-04.2
    virtual bool doStep(double currentTime, double stepSize) = 0;

    /// @brief Terminates the FMU and releases the FMI component.
    ///
    /// Calls `fmi*Terminate` followed by `fmi*FreeInstance`.
    /// @req FR-04.1
    virtual void terminate() = 0;

    /// @brief Returns `true` if initialize() has been called successfully.
    /// @req FR-04.1
    virtual bool isInitialized() const noexcept = 0;

    /// @name FMI 3.0 Event Mode, Binary and Clock access (optional)
    /// Backends that cannot do it keep these defaults, which report the operation as unsupported.
    /// @{
    /// @brief One fmi3DoStep with its full outcome.
    /// @brief One step with its full outcome. @throws FmuSimulationException if unsupported.
    virtual StepOutcome doStepEx(double, double) { throw unsupported("doStepEx"); }
    /// @brief Step Mode to Event Mode. @throws FmuSimulationException if unsupported.
    virtual void enterEventMode() { throw unsupported("enterEventMode"); }
    /// @brief Event Mode to Step Mode. @throws FmuSimulationException if unsupported.
    virtual void enterStepMode() { throw unsupported("enterStepMode"); }
    /// @brief Settles the discrete states in Event Mode. @throws FmuSimulationException if unsupported.
    virtual DiscreteStatesOutcome updateDiscreteStates() { throw unsupported("updateDiscreteStates"); }
    /// @brief Copy of a Binary variable's buffer. @throws FmuSimulationException if unsupported.
    virtual std::vector<uint8_t> getBinary(uint32_t) { throw unsupported("getBinary"); }
    /// @brief Sets a Binary variable. @throws FmuSimulationException if unsupported.
    virtual void setBinary(uint32_t, std::span<const uint8_t>) { throw unsupported("setBinary"); }
    /// @brief Whether a Clock is active. @throws FmuSimulationException if unsupported.
    virtual bool getClock(uint32_t) { throw unsupported("getClock"); }
    /// @brief Activates or deactivates a Clock. @throws FmuSimulationException if unsupported.
    virtual void setClock(uint32_t, bool) { throw unsupported("setClock"); }
    /// @brief Next interval of a Clock. @throws FmuSimulationException if unsupported.
    virtual ClockInterval getClockInterval(uint32_t) { throw unsupported("getClockInterval"); }
    /// @}

protected:
    ISimulationBackend() = default;

    /// @brief The exception the default implementations above throw.
    static FmuSimulationException unsupported(const char* what) {
        return FmuSimulationException(std::string("ISimulationBackend::") + what + " is not supported by this backend");
    }
};

} // namespace libfmu_toolkit
