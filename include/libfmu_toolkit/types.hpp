/// @file types.hpp
/// @brief All public data types for the lib-fmu-toolkit API.
///
/// This header is the sole source of truth for types exchanged between
/// the library and its callers. It includes only standard C++ headers and
/// `export.hpp`; no third-party types (libxml2, libzip) or internal domain
/// types are exposed here.
///
/// Include this header directly, or use the umbrella:
/// @code
/// #include <libfmu_toolkit/lib_fmu_toolkit.hpp>
/// @endcode
#pragma once

#include "export.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace libfmu_toolkit {

// ---------------------------------------------------------------------------
// Enumerations
// ---------------------------------------------------------------------------

/// @brief Verbosity level for FMU-internal log messages.
///
/// Passed via SimConfig::logLevel. The FMU itself decides how to interpret the
/// level; not all FMUs honour all levels.
/// @req FR-04.1
enum class LogLevel : uint8_t {
    None,       ///< Suppress all log output from the FMU.
    Warning,    ///< Log warnings only (default).
    Info,       ///< Log informational messages and warnings.
    Debug,      ///< Log detailed debug output (verbose).
    All         ///< Log everything the FMU emits.
};

/// @brief FMI specification version of a loaded FMU.
/// @req FR-01.4
enum class FmiVersion : uint8_t {
    FMI2 = 2, ///< Functional Mock-up Interface 2.0.
    FMI3 = 3  ///< Functional Mock-up Interface 3.0.
};

/// @brief Causality of a model variable as defined in FMI.
///
/// Describes the role of a variable in the model's interface contract.
/// @req FR-02.2
enum class Causality : uint8_t {
    Parameter,            ///< Tunable / fixed parameter; set before initialization.
    CalculatedParameter,  ///< Parameter whose value is calculated during initialization.
    Input,                ///< External input driven by the simulation master.
    Output,               ///< Value produced by the FMU and consumed by the master.
    Local,                ///< Internal variable not exchangeable with the outside.
    Independent           ///< The independent variable (typically simulation time).
};

/// @brief Variability of a model variable as defined in FMI.
///
/// Describes how and when a variable's value can change during simulation.
/// @req FR-02.2
enum class Variability : uint8_t {
    Constant,    ///< Constant throughout the simulation (never changes).
    Fixed,       ///< Fixed after initialization; cannot be changed during simulation.
    Tunable,     ///< Can be changed between communication points.
    Discrete,    ///< Changes at event instants only.
    Continuous   ///< Can change continuously between communication points.
};

/// @brief Data type of a model variable.
///
/// FMI 2.0 defines Real, Integer, Boolean, and String. FMI 3.0 extends this
/// with Float32/64, Int8/16/32/64, UInt8/16/32/64, Binary, and Clock.
/// @req FR-02.2
enum class DataType : uint8_t {
    Real,    ///< FMI 2.0 real (64-bit float).
    Integer, ///< FMI 2.0 integer (platform int).
    Boolean, ///< Boolean value (true/false).
    String,  ///< Null-terminated string.
    Float64, ///< FMI 3.0 double-precision floating-point (64-bit).
    Float32, ///< FMI 3.0 single-precision floating-point (32-bit).
    Int8,    ///< FMI 3.0 signed 8-bit integer.
    Int16,   ///< FMI 3.0 signed 16-bit integer.
    Int32,   ///< FMI 3.0 signed 32-bit integer.
    Int64,   ///< FMI 3.0 signed 64-bit integer.
    UInt8,   ///< FMI 3.0 unsigned 8-bit integer.
    UInt16,  ///< FMI 3.0 unsigned 16-bit integer.
    UInt32,  ///< FMI 3.0 unsigned 32-bit integer.
    UInt64,  ///< FMI 3.0 unsigned 64-bit integer.
    Binary,  ///< FMI 3.0 binary blob.
    Clock    ///< FMI 3.0 clock (event-based triggering).
};

/// @brief Initialization source of a model variable's start value.
/// @req FR-02.2
enum class Initial : uint8_t {
    Exact,       ///< The start value is the exact initial value.
    Approx,      ///< The start value is an approximation; solver may refine it.
    Calculated,  ///< The start value is calculated during initialization.
    None         ///< No initial value attribute present.
};

// ---------------------------------------------------------------------------
// ValidationCategory
// ---------------------------------------------------------------------------

/// @brief Which validation phase produced a diagnostic entry.
///
/// Returned as part of ValidationIssue so callers can filter or display
/// diagnostics by phase without parsing the message text.
/// @req FR-03.1
enum class ValidationCategory : uint8_t {
    Structure, ///< Phase 1 — archive layout and binary directory naming.
    Schema,    ///< Phase 2 — XSD schema validation of modelDescription.xml.
    Semantic   ///< Phase 3 — causality/variability and value-reference checks.
};

// ---------------------------------------------------------------------------
// UnitInfo
// ---------------------------------------------------------------------------

/// @brief A physical unit definition parsed from `<UnitDefinitions>`.
///
/// SI conversion follows the affine formula:
/// @f$ \text{value}_{SI} = \text{factor} \times \text{value} + \text{offset} @f$
/// @req FR-02.4
struct LIB_FMU_TOOLKIT_API UnitInfo {
    std::string name;        ///< Unit name as declared in the XML, e.g. `"m/s2"`, `"rad/s"`.
    double      factor{1.0}; ///< Multiplicative SI conversion factor (default: 1.0).
    double      offset{0.0}; ///< Additive SI conversion offset (default: 0.0).
};

// ---------------------------------------------------------------------------
// TypeDefinitionInfo
// ---------------------------------------------------------------------------

/// @brief A named type parsed from `<SimpleType>` (FMI 2.0) or `<TypeDefinitions>` (FMI 3.0).
///
/// Named types provide reusable attribute sets (unit, quantity, min/max constraints)
/// that individual variables may reference via their `declaredType` attribute.
/// @req FR-02.4
struct LIB_FMU_TOOLKIT_API TypeDefinitionInfo {
    std::string name;        ///< Declared type name, referenced by VariableInfo::typeName.
    std::string description; ///< Human-readable description.
    DataType    dataType{DataType::Real}; ///< Data type of variables using this type.
    std::string unit;        ///< Physical unit name reference (e.g. `"m/s"`).
    std::string quantity;    ///< Physical quantity (e.g. `"Velocity"`, `"Acceleration"`).
    std::string displayUnit; ///< Preferred display unit (may differ from SI base unit).
    std::string minStr;      ///< Minimum allowed value as a string (empty = unbounded).
    std::string maxStr;      ///< Maximum allowed value as a string (empty = unbounded).
};

// ---------------------------------------------------------------------------
// DefaultExperiment
// ---------------------------------------------------------------------------

/// @brief Recommended simulation parameters from `<DefaultExperiment>`.
///
/// These values are suggestions from the FMU author. Callers are free to
/// override them in SimConfig. The `has*` flags indicate which attributes
/// were actually present in the XML; absent attributes retain their default
/// numeric values but should not be treated as meaningful.
/// @req FR-02.5
struct LIB_FMU_TOOLKIT_API DefaultExperiment {
    double startTime{0.0};  ///< Recommended simulation start time.
    double stopTime{1.0};   ///< Recommended simulation stop time.
    double stepSize{0.0};   ///< Recommended communication step size (0.0 = not specified).
    double tolerance{0.0};  ///< Recommended solver tolerance (0.0 = not specified).
    bool   hasStartTime{false}; ///< `true` if `startTime` was present in the XML.
    bool   hasStopTime{false};  ///< `true` if `stopTime` was present in the XML.
    bool   hasStepSize{false};  ///< `true` if `stepSize` was present in the XML.
    bool   hasTolerance{false}; ///< `true` if `tolerance` was present in the XML.
};

// ---------------------------------------------------------------------------
// VariableInfo
// ---------------------------------------------------------------------------

/// @brief Public description of one model variable from `<ModelVariables>`.
///
/// Populated from `modelDescription.xml` during FmuEngine::load(). All fields
/// are read-only reflections of the XML content.
/// @req FR-02.2
struct LIB_FMU_TOOLKIT_API VariableInfo {
    std::string name;                         ///< Unique variable name within the FMU.
    std::string description;                  ///< Human-readable description (may be empty).
    uint32_t    valueReference{0};            ///< FMI value reference used in get/set calls.
    Causality   causality{Causality::Local};  ///< Causality as declared in the XML.
    Variability variability{Variability::Continuous}; ///< Variability as declared in the XML.
    DataType    dataType{DataType::Real};     ///< Data type of this variable.
    bool        hasStartValue{false};         ///< `true` if a start value was declared.
    std::string startValueStr;               ///< String representation of the start value.
    std::string unit;                        ///< Physical unit name (e.g. `"m/s"`; may be empty).
    std::string typeName;                    ///< Referenced TypeDefinition name (may be empty).
};

// ---------------------------------------------------------------------------
// FmuInfo
// ---------------------------------------------------------------------------

/// @brief Full metadata of a loaded FMU, populated from `modelDescription.xml`.
///
/// Returned by FmuEngine::info(). All fields are immutable snapshots of the
/// XML content at load time.
/// @req FR-02.1
struct LIB_FMU_TOOLKIT_API FmuInfo {
    std::string modelName;                 ///< Model name as declared in the XML.
    std::string modelIdentifier;           ///< Model identifier used to name the shared library.
    std::string guid;                      ///< Globally unique identifier of the FMU.
    std::string description;               ///< Human-readable model description.
    std::string generationTool;            ///< Tool that generated the FMU (may be empty).
    std::string generationDateAndTime;     ///< ISO 8601 generation timestamp (may be empty).
    FmiVersion  fmiVersion{FmiVersion::FMI2}; ///< FMI specification version.
    uint32_t    numberOfEventIndicators{0};   ///< Number of event indicators (FMI 2.0 only).

    std::vector<VariableInfo>       variables;       ///< All model variables.
    std::vector<UnitInfo>           unitDefinitions; ///< Unit definitions from `<UnitDefinitions>`.
    std::vector<TypeDefinitionInfo> typeDefinitions; ///< Named types from `<TypeDefinitions>`.
    DefaultExperiment               defaultExperiment; ///< Recommended experiment parameters.

    /// @brief Returns pointers to all variables with `causality == Input`.
    /// @returns Vector of non-owning pointers; valid for the lifetime of this FmuInfo.
    /// @req FR-02.3
    std::vector<const VariableInfo*> inputs() const;

    /// @brief Returns pointers to all variables with `causality == Output`.
    /// @returns Vector of non-owning pointers; valid for the lifetime of this FmuInfo.
    /// @req FR-02.3
    std::vector<const VariableInfo*> outputs() const;

    /// @brief Returns pointers to all variables with `causality == Parameter`.
    /// @returns Vector of non-owning pointers; valid for the lifetime of this FmuInfo.
    /// @req FR-02.3
    std::vector<const VariableInfo*> parameters() const;

    /// @brief Finds a variable by name.
    ///
    /// Accepts any string-like type (`std::string`, `std::string_view`, string
    /// literal) without allocation — the search is performed by transparent
    /// comparison against the stored `std::string` names.
    ///
    /// @param name Variable name to search for.
    /// @returns Pointer to the matching VariableInfo, or `nullptr` if not found.
    /// @req FR-02.3
    const VariableInfo* findVariable(std::string_view name) const;
};

// ---------------------------------------------------------------------------
// ValidationIssue
// ---------------------------------------------------------------------------

/// @brief A single diagnostic entry from the three-phase FMU validation.
///
/// `category` identifies which phase produced this entry. Line and column
/// numbers are populated for XSD schema errors (sourced from libxml2).
/// Structural and semantic checks leave them at their default `-1`.
/// @req FR-03.5
struct LIB_FMU_TOOLKIT_API ValidationIssue {
    ValidationCategory category{ValidationCategory::Structure}; ///< Phase that produced this entry.
    std::string message; ///< Human-readable description of the issue.
    int         line{-1};   ///< Source line number in `modelDescription.xml` (-1 = unknown).
    int         column{-1}; ///< Source column number in `modelDescription.xml` (-1 = unknown).
};

// ---------------------------------------------------------------------------
// ValidationResult
// ---------------------------------------------------------------------------

/// @brief Aggregated result of FmuEngine::validate().
///
/// Contains the overall pass/fail verdict plus lists of errors and warnings
/// from all three validation phases (structural, XSD schema, semantic).
/// FmuEngine::validate() never throws — all findings are reported here.
/// @req FR-03.1
struct LIB_FMU_TOOLKIT_API ValidationResult {
    bool passed{false}; ///< `true` if all phases passed with no errors.
    std::vector<ValidationIssue> errors;   ///< Error-level issues (cause `passed == false`).
    std::vector<ValidationIssue> warnings; ///< Warning-level issues (do not affect `passed`).

    /// @brief Returns `true` if there is at least one error.
    ///
    /// Implemented out-of-line in the library to avoid inline STL container
    /// access across translation unit boundaries (ABI stability).
    /// @req FR-03.8
    [[nodiscard]] bool hasErrors()   const noexcept;

    /// @brief Returns `true` if there is at least one warning.
    ///
    /// Implemented out-of-line in the library to avoid inline STL container
    /// access across translation unit boundaries (ABI stability).
    /// @req FR-03.1
    [[nodiscard]] bool hasWarnings() const noexcept;
};

// ---------------------------------------------------------------------------
// SimConfig
// ---------------------------------------------------------------------------

/// @brief Value type for initial parameter and input overrides.
///
/// Holds one of `double`, `int64_t`, `bool`, or `std::string`, matching the
/// four fundamental FMI variable types.
/// @req FR-04.3
using ParameterValue = std::variant<double, int64_t, bool, std::string>;

/// @brief Configuration for a co-simulation run.
///
/// Passed to FmuEngine::simulate() or FmuEngine::openSession() to specify
/// timing, tolerances, initial values, and recording policy.
/// @req FR-04.1
struct LIB_FMU_TOOLKIT_API SimConfig {
    double   startTime{0.0};    ///< Simulation start time (seconds).
    double   stopTime{1.0};     ///< Simulation stop time (seconds).
    double   stepSize{0.001};   ///< Communication step size (seconds).
    double   tolerance{1e-6};   ///< Solver relative tolerance.
    LogLevel logLevel{LogLevel::Warning}; ///< Verbosity of FMU-internal log output.

    /// @brief Initial values for parameters and inputs, keyed by variable name.
    ///
    /// Applied during initialization (before `fmi*ExitInitializationMode`).
    /// Variables not listed here retain their XML-declared start values.
    std::map<std::string, ParameterValue> initialParameters;

    /// @brief Names of output variables to record. Empty = record all outputs.
    ///
    /// When non-empty, only variables listed here are captured in SimResult::series.
    /// Variable names that do not match any output variable are silently ignored.
    std::vector<std::string> outputVariables;

    /// @brief Record output values every N communication steps (default: 1 = every step).
    ///
    /// Values of 2 or more reduce memory usage for long simulations at the cost
    /// of lower time-series resolution.
    uint32_t recordingInterval{1};
};

// ---------------------------------------------------------------------------
// SimResult
// ---------------------------------------------------------------------------

/// @brief Value type for a single recorded variable value in a time series.
///
/// Holds one of `double`, `int64_t`, `bool`, or `std::string`, matching the
/// four fundamental FMI variable types.
/// @req FR-04.6
using OutputValue = std::variant<double, int64_t, bool, std::string>;

/// @brief Time-series result of a completed co-simulation run.
///
/// Returned by FmuEngine::simulate(). Can be exported to CSV via
/// FmuEngine::exportCsv() or to JSON via FmuEngine::exportJson().
/// @req FR-04.6
struct LIB_FMU_TOOLKIT_API SimResult {
    /// @brief Recorded time points, one per recording interval.
    std::vector<double> timePoints;

    /// @brief Per-variable time series: variable name → vector of values.
    ///
    /// Each inner vector has the same length as `timePoints`. Values are stored
    /// in the same order as the simulation progressed.
    std::map<std::string, std::vector<OutputValue>> series;

    /// @brief Returns the number of recorded time points.
    ///
    /// Implemented out-of-line in the library (ABI stability).
    /// @req FR-04.6
    [[nodiscard]] size_t stepCount() const noexcept;

    /// @brief Returns `true` if no time points were recorded.
    ///
    /// Implemented out-of-line in the library (ABI stability).
    /// @req FR-04.6
    [[nodiscard]] bool   empty()     const noexcept;
};

} // namespace libfmu_toolkit
