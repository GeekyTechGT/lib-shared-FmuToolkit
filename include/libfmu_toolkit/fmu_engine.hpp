/// @file fmu_engine.hpp
/// @brief Primary public API: FmuEngine and FmuSession.
///
/// Include this header (or the umbrella `lib_fmu_toolkit.hpp`) to access the
/// full library interface. No libxml2, libzip, or internal domain types are
/// exposed by this header.
#pragma once

#include "export.hpp"
#include "types.hpp"
#include "exceptions.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace libfmu_toolkit {

// ---------------------------------------------------------------------------
// FmuSession
// ---------------------------------------------------------------------------

/// @brief Step-by-step FMU co-simulation controller.
///
/// Obtained from FmuEngine::openSession(). Provides fine-grained control over
/// the FMU co-simulation loop. Use this when an external time-sync coordinator
/// (e.g. SIL-Kit, a co-simulation master) drives simulation time rather than
/// lib-fmu-toolkit's own SimulationController.
///
/// **Lifecycle**
/// @code
/// auto session = engine.openSession(handle, config);
/// // The FMU is already initialised at this point.
///
/// while (running) {
///     session.setValue("u", inputValue);   // optional
///     bool ok = session.doStep(t, dt);
///     auto y  = session.getValue("y");
///     t += dt;
/// }
/// session.terminate();
/// @endcode
///
/// @note The FMU handle must **not** be unloaded via FmuEngine::unload() while
///       a session for that handle is still active — doing so is undefined
///       behaviour.
/// @req FR-05.1
class LIB_FMU_TOOLKIT_API FmuSession {
public:
    /// @brief Terminates the FMU and releases all associated resources.
    ///
    /// Calls fmi*Terminate and fmi*FreeInstance on the FMU if the session is
    /// still active. Safe to call even if terminate() was already called.
    /// @req FR-05.6
    ~FmuSession();

    /// @cond INTERNAL
    FmuSession(const FmuSession&)            = delete;
    FmuSession& operator=(const FmuSession&) = delete;
    /// @endcond

    /// @brief Move constructor.
    /// @req NFR-02.2
    FmuSession(FmuSession&&)            noexcept;
    /// @brief Move-assignment operator.
    /// @req NFR-02.2
    FmuSession& operator=(FmuSession&&) noexcept;

    /// @brief Returns the metadata of the loaded FMU.
    ///
    /// The returned reference is valid for the lifetime of the session.
    /// @req FR-05.1
    [[nodiscard]] const FmuInfo& info() const noexcept;

    /// @brief Writes a `double` value to an input or parameter variable.
    ///
    /// @param varName  Name of the variable to set. Accepts any string-like type
    ///                 (`std::string`, `std::string_view`, string literal) without allocation.
    /// @param value    New value to write.
    ///
    /// @throws std::out_of_range   if `varName` is not a variable in this FMU.
    /// @throws FmuSimulationError  on FMI runtime errors.
    /// @req FR-05.3
    void setValue(std::string_view varName, double           value);

    /// @brief Writes an `int64_t` value to an input or parameter variable.
    ///
    /// @param varName  Name of the variable to set.
    /// @param value    New value to write.
    ///
    /// @throws std::out_of_range   if `varName` is unknown.
    /// @throws FmuSimulationError  on FMI runtime errors.
    /// @req FR-05.3
    void setValue(std::string_view varName, int64_t          value);

    /// @brief Writes a `bool` value to an input or parameter variable.
    ///
    /// @param varName  Name of the variable to set.
    /// @param value    New value to write.
    ///
    /// @throws std::out_of_range   if `varName` is unknown.
    /// @throws FmuSimulationError  on FMI runtime errors.
    /// @req FR-05.3
    void setValue(std::string_view varName, bool             value);

    /// @brief Writes a `std::string` value to an input or parameter variable.
    ///
    /// @param varName  Name of the variable to set.
    /// @param value    New value to write.
    ///
    /// @throws std::out_of_range   if `varName` is unknown.
    /// @throws FmuSimulationError  on FMI runtime errors.
    /// @req FR-05.3
    void setValue(std::string_view varName, std::string_view value);

    /// @brief Reads the current value of any variable by name.
    ///
    /// Works for outputs, locals, and any other readable variable.
    ///
    /// @param varName  Name of the variable to read. Accepts any string-like type
    ///                 without allocation.
    /// @returns        An OutputValue variant (`double`, `int64_t`, `bool`, or `std::string`).
    ///
    /// @throws std::out_of_range   if `varName` is unknown.
    /// @throws FmuSimulationError  on FMI runtime errors.
    /// @req FR-05.4
    [[nodiscard]] OutputValue getValue(std::string_view varName) const;

    /// @brief Advances the FMU by one communication step.
    ///
    /// Calls `fmi2DoStep` or `fmi3DoStep` with the provided time and step size.
    ///
    /// @param currentTime  Current simulation time (seconds).
    /// @param stepSize     Duration of this communication step (seconds).
    ///
    /// @returns `true` if the step completed successfully; `false` if the FMU
    ///          requests early termination (fmi*Discard or terminateSimulation).
    ///
    /// @throws FmuSimulationError on a fatal FMI error status.
    /// @req FR-05.2
    [[nodiscard]] bool doStep(double currentTime, double stepSize);

    /// @name FMI 3.0 Event Mode, Binary and Clock access
    /// Open the session with `SimConfig::eventModeUsed = true`. After openSession() the FMU is in Step Mode;
    /// enterEventMode() / enterStepMode() switch, and Clocks are set in Event Mode.
    /// @throws FmuSimulationError if the FMU is not FMI 3.0, lacks the functions, or the call is out of order.
    /// @throws std::out_of_range if the variable is unknown.
    /// @{
    /// @brief fmi3DoStep with its full outcome (event handling needed, early return, last successful time).
    [[nodiscard]] StepOutcome doStepEx(double currentTime, double stepSize);
    void enterEventMode();
    void enterStepMode();
    [[nodiscard]] DiscreteStatesOutcome updateDiscreteStates();
    /// @brief Copy of the Binary variable's current buffer (empty = no data).
    [[nodiscard]] std::vector<uint8_t> getBinary(std::string_view varName) const;
    void setBinary(std::string_view varName, std::span<const uint8_t> bytes);
    [[nodiscard]] bool getClock(std::string_view varName) const;
    void setClock(std::string_view varName, bool active);
    [[nodiscard]] ClockInterval getClockInterval(std::string_view varName) const;
    /// @}

    /// @brief Terminates the FMU co-simulation.
    ///
    /// Sends `fmi*Terminate` followed by `fmi*FreeInstance`. Subsequent calls
    /// to doStep(), getValue(), or setValue() will result in undefined behaviour.
    /// Safe to call multiple times — subsequent calls after the first are no-ops.
    ///
    /// @throws FmuSimulationError if termination fails at the FMI level.
    /// @req FR-05.5
    void terminate();

    /// @brief Returns `true` if the session has been initialised and not yet terminated.
    /// @req FR-05.1
    [[nodiscard]] bool isActive() const noexcept;

private:
    /// @cond INTERNAL
    struct Impl;
    std::unique_ptr<Impl> impl_;
    friend class FmuEngine;
    explicit FmuSession(std::unique_ptr<Impl> impl);
    /// @endcond
};

// ---------------------------------------------------------------------------
// FmuEngine
// ---------------------------------------------------------------------------

/// @brief PIMPL facade for the entire lib-fmu-toolkit library.
///
/// `FmuEngine` is the entry point for all library operations. It manages the
/// lifecycle of one or more loaded FMUs identified by opaque `uint64_t` handles.
///
/// **Typical usage**
/// @code
/// libfmu_toolkit::FmuEngine engine;
///
/// // Load — extracts ZIP, parses XML, loads DLL, resolves FMI symbols.
/// uint64_t handle = engine.load("path/to/my.fmu");
///
/// // Inspect metadata.
/// auto info = engine.info(handle);
///
/// // Optionally validate.
/// auto report = engine.validate(handle);
///
/// // Run a simulation.
/// libfmu_toolkit::SimConfig cfg;
/// cfg.stopTime = 5.0;
/// cfg.stepSize = 0.01;
/// auto result  = engine.simulate(handle, cfg);
///
/// // Export to CSV or JSON.
/// libfmu_toolkit::FmuEngine::exportCsv(result, "output.csv");
/// libfmu_toolkit::FmuEngine::exportJson(result, "output.json");
///
/// // Unload optional — destructor cleans up all handles.
/// engine.unload(handle);
/// @endcode
///
/// **Thread safety**
/// `FmuEngine` is **not** thread-safe. Do not share an instance across threads
/// without external synchronisation.
/// @req FR-01.1
class LIB_FMU_TOOLKIT_API FmuEngine {
public:
    /// @brief Constructs a FmuEngine, optionally specifying where FMI XSD schemas live.
    ///
    /// @param xsdBasePath  Path to the directory that contains the FMI XSD schema
    ///   sub-trees:
    ///   @code
    ///   <xsdBasePath>/fmi-standard-2.0/schema/fmi2ModelDescription.xsd
    ///   <xsdBasePath>/fmi-standard-3.0/schema/fmi3ModelDescription.xsd
    ///   @endcode
    ///
    ///   If empty (the default), the engine resolves schemas automatically — see
    ///   the documentation on runtime XSD schema discovery for the priority order.
    /// @note Accepts any string-like type (`std::string`, `std::string_view`,
    ///       string literal) without allocation.
    /// @req FR-07.1
    explicit FmuEngine(std::string_view xsdBasePath = {});

    /// @brief Destructor. Unloads all remaining FMU handles.
    /// @req FR-01.6
    ~FmuEngine();

    /// @cond INTERNAL
    FmuEngine(const FmuEngine&)            = delete;
    FmuEngine& operator=(const FmuEngine&) = delete;
    /// @endcond

    /// @brief Move constructor.
    /// @req NFR-02.2
    FmuEngine(FmuEngine&&)            noexcept;
    /// @brief Move-assignment operator.
    /// @req NFR-02.2
    FmuEngine& operator=(FmuEngine&&) noexcept;

    /// @brief Loads an FMU from a file path.
    ///
    /// Performs the following operations in sequence:
    ///   1. Extracts the FMU ZIP archive to a temporary directory.
    ///   2. Parses `modelDescription.xml` into an internal FmuPackage.
    ///   3. Loads the platform shared library (`binaries/<platform>/<modelIdentifier>.dll/.so`).
    ///   4. Resolves all required FMI function symbols.
    ///
    /// @param fmuPath  Absolute or relative path to the `.fmu` file. Accepts any
    ///                 string-like type without allocation.
    /// @returns        Opaque handle ID for use in subsequent calls.
    ///
    /// @throws FmuLoadError        if the archive, DLL, or symbol resolution fails.
    /// @throws std::invalid_argument if the path is invalid or the file does not exist.
    /// @req FR-01.1
    [[nodiscard]] uint64_t load(std::string_view fmuPath);

    /// @brief Returns the metadata of a loaded FMU.
    ///
    /// @param handle  Handle returned by load().
    /// @returns       FmuInfo populated from `modelDescription.xml`.
    ///
    /// @throws std::out_of_range  if `handle` was not returned by load().
    /// @req FR-02.1
    [[nodiscard]] FmuInfo info(uint64_t handle) const;

    /// @brief Validates a loaded FMU through three phases.
    ///
    /// **Phase 1 — Structure:** verifies the FMU archive layout mandated by the
    ///   FMI standard, including:
    ///   - `modelDescription.xml` at the archive root
    ///   - presence of `binaries/`
    ///   - platform directory naming consistent with the declared FMI version
    ///   - shared library filename matching `modelIdentifier`
    /// **Phase 2 — XSD schema:** validates `modelDescription.xml` against the
    ///   official FMI 2.0 or 3.0 XSD schema using libxml2.
    /// **Phase 3 — Semantics:** checks causality/variability combinations,
    ///   duplicate value references, and related FMI invariants.
    ///
    /// This method never throws — all findings are returned in ValidationResult.
    ///
    /// @param handle  Handle returned by load().
    /// @returns       Validation report with `passed` flag, errors, and warnings.
    ///
    /// @throws std::out_of_range  if `handle` is unknown.
    /// @req FR-03.1
    [[nodiscard]] ValidationResult validate(uint64_t handle) const;

    /// @brief Runs a complete co-simulation lifecycle.
    ///
    /// Drives the FMU through the following sequence:
    ///   - `fmi*Instantiate`
    ///   - `fmi*SetupExperiment` / `fmi*EnterInitializationMode`
    ///   - Initial parameter injection
    ///   - `fmi*ExitInitializationMode`
    ///   - Communication step loop until `config.stopTime`
    ///   - `fmi*Terminate` + `fmi*FreeInstance`
    ///
    /// @param handle    Handle returned by load().
    /// @param config    Simulation configuration (timing, step size, parameters, etc.).
    /// @param progress  Optional callback invoked at each recorded step:
    ///                  `(recordIndex, currentTime, totalSteps)`.
    ///
    /// @returns SimResult containing recorded time points and variable time series.
    ///
    /// @throws FmuSimulationError  on fatal FMI errors during the simulation loop.
    /// @throws FmuLoadError        if the DLL cannot be re-loaded inside the adapter.
    /// @req FR-04.1
    [[nodiscard]] SimResult simulate(
        uint64_t handle,
        const SimConfig& config,
        std::function<void(size_t, double, size_t)> progress = nullptr);

    /// @brief Exports a SimResult to a CSV file.
    ///
    /// The CSV file format is:
    /// @code
    /// time,var1,var2,...
    /// 0.000000,1.000000,0,...
    /// @endcode
    /// String values are RFC-4180 quoted and any embedded double-quotes are
    /// escaped by doubling. Boolean values are written as `0` or `1`.
    ///
    /// This is a static method — no loaded FMU is required.
    ///
    /// @param result      SimResult to serialise.
    /// @param outputPath  Destination file path. Overwritten if it already exists.
    ///                    Accepts any string-like type without allocation.
    ///
    /// @throws std::ios_base::failure  if the file cannot be opened or written.
    /// @req FR-06.1
    static void exportCsv(const SimResult& result, std::string_view outputPath);

    /// @brief Exports a SimResult to a JSON file.
    ///
    /// The JSON file format is:
    /// @code
    /// {
    ///   "timePoints": [0.0, 0.01, ...],
    ///   "series": {
    ///     "varName": [1.0, 1.1, ...],
    ///     ...
    ///   }
    /// }
    /// @endcode
    /// Boolean values are written as JSON `true`/`false`. String values are
    /// JSON-escaped (double-quotes and backslashes). No third-party JSON library
    /// is required.
    ///
    /// This is a static method — no loaded FMU is required.
    ///
    /// @param result      SimResult to serialise.
    /// @param outputPath  Destination file path. Overwritten if it already exists.
    ///                    Accepts any string-like type without allocation.
    ///
    /// @throws std::ios_base::failure  if the file cannot be opened or written.
    /// @req FR-06.2
    static void exportJson(const SimResult& result, std::string_view outputPath);

    /// @brief Initialises a FMU and returns an interactive session.
    ///
    /// Use this instead of simulate() when an external time coordinator (e.g.
    /// SIL-Kit) drives the simulation clock. The FMU is instantiated and
    /// initialised immediately; the returned FmuSession is ready for doStep() calls.
    ///
    /// @param handle   Handle returned by load().
    /// @param config   Simulation configuration (start/stop time, step size,
    ///                 tolerance, initial parameters).
    ///
    /// @returns A live FmuSession in the initialized state.
    ///
    /// @throws FmuSimulationError  on FMU initialisation errors.
    /// @throws std::out_of_range   if `handle` is unknown.
    /// @req FR-05.1
    [[nodiscard]] FmuSession openSession(uint64_t handle, const SimConfig& config);

    /// @brief Releases the FMU associated with a handle.
    ///
    /// Destroys the FmuPackage and closes the OS shared library handle.
    /// This is optional — the FmuEngine destructor unloads all remaining handles.
    ///
    /// @warning Do not call unload() while an active FmuSession for this handle
    ///          exists. Doing so is undefined behaviour.
    ///
    /// @param handle  Handle returned by load().
    ///
    /// @throws std::out_of_range  if `handle` is unknown.
    /// @req FR-01.6
    void unload(uint64_t handle);

private:
    /// @cond INTERNAL
    struct Impl;
    std::unique_ptr<Impl> impl_;
    /// @endcond
};

} // namespace libfmu_toolkit
