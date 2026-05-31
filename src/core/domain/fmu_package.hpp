#pragma once

/// @file fmu_package.hpp
/// @brief Internal aggregate of everything extracted and parsed from an FMU archive.
/// @internal

#include "fmu_variable_types.hpp"
#include "model_variable.hpp"
#include "log_category.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace libfmu_toolkit {

/// @brief A physical unit definition parsed from `<UnitDefinitions>`.
///
/// SI conversion follows the affine formula `value_SI = factor * value + offset`.
/// @req FR-02.4
struct UnitDef {
    std::string name;        ///< Unit name as declared in the XML (e.g. `"m/s2"`).
    double      factor{1.0}; ///< Multiplicative SI conversion factor.
    double      offset{0.0}; ///< Additive SI conversion offset.
};

/// @brief A named type parsed from `<TypeDefinitions>` (FMI 3.0) or `<SimpleType>` (FMI 2.0).
/// @req FR-02.4
struct TypeDef {
    std::string name;        ///< Declared type name, referenced by ModelVariable::typeName.
    std::string description; ///< Human-readable description.
    DataType    dataType{DataType::Real}; ///< Data type of variables using this type.
    std::string unit;        ///< Physical unit name reference.
    std::string quantity;    ///< Physical quantity (e.g. `"Velocity"`).
    std::string displayUnit; ///< Preferred display unit.
    std::string minStr;      ///< Minimum allowed value as a string (empty = unbounded).
    std::string maxStr;      ///< Maximum allowed value as a string (empty = unbounded).
};

/// @brief Recommended simulation parameters parsed from `<DefaultExperiment>`.
///
/// The `has*` flags indicate which attributes were actually present in the XML;
/// absent attributes keep their default numeric values but are not meaningful.
/// @req FR-02.5
struct DomainDefaultExperiment {
    double startTime{0.0};  ///< Recommended simulation start time.
    double stopTime{1.0};   ///< Recommended simulation stop time.
    double stepSize{0.0};   ///< Recommended communication step size (0.0 = unspecified).
    double tolerance{0.0};  ///< Recommended solver tolerance (0.0 = unspecified).
    bool   hasStartTime{false}; ///< `true` if `startTime` was present in the XML.
    bool   hasStopTime{false};  ///< `true` if `stopTime` was present in the XML.
    bool   hasStepSize{false};  ///< `true` if `stepSize` was present in the XML.
    bool   hasTolerance{false}; ///< `true` if `tolerance` was present in the XML.
};

/// @brief Aggregates everything extracted and parsed from an FMU archive.
///
/// Holds the parsed metadata (variables, units, types, default experiment), the
/// raw `modelDescription.xml`, the on-disk extraction paths, and the resolved
/// FMI 2.0 or 3.0 function pointer tables.
///
/// @note Non-copyable but movable. The shared library OS handle (#sharedLibHandle)
///       is **not** owned here — its lifetime is managed by the loader adapter.
/// @req FR-01.2
class FmuPackage {
public:
    /// @brief Resolved FMI 2.0 co-simulation entry points (raw `void*` symbols).
    /// @req FR-01.3
    struct Fmi2Functions {
        void* fmi2GetTypesPlatform{nullptr};
        void* fmi2GetVersion{nullptr};
        void* fmi2SetDebugLogging{nullptr};
        void* fmi2Instantiate{nullptr};
        void* fmi2FreeInstance{nullptr};
        void* fmi2SetupExperiment{nullptr};
        void* fmi2EnterInitializationMode{nullptr};
        void* fmi2ExitInitializationMode{nullptr};
        void* fmi2Terminate{nullptr};
        void* fmi2Reset{nullptr};
        void* fmi2GetReal{nullptr};
        void* fmi2GetInteger{nullptr};
        void* fmi2GetBoolean{nullptr};
        void* fmi2GetString{nullptr};
        void* fmi2SetReal{nullptr};
        void* fmi2SetInteger{nullptr};
        void* fmi2SetBoolean{nullptr};
        void* fmi2SetString{nullptr};
        void* fmi2DoStep{nullptr};

        /// @brief Returns `true` if every required FMI 2.0 symbol was resolved.
        /// @req FR-01.3
        [[nodiscard]] bool isComplete() const noexcept {
            return fmi2GetTypesPlatform        != nullptr
                && fmi2GetVersion              != nullptr
                && fmi2SetDebugLogging         != nullptr
                && fmi2Instantiate             != nullptr
                && fmi2FreeInstance            != nullptr
                && fmi2SetupExperiment         != nullptr
                && fmi2EnterInitializationMode != nullptr
                && fmi2ExitInitializationMode  != nullptr
                && fmi2Terminate               != nullptr
                && fmi2Reset                   != nullptr
                && fmi2GetReal                 != nullptr
                && fmi2GetInteger              != nullptr
                && fmi2GetBoolean              != nullptr
                && fmi2GetString               != nullptr
                && fmi2SetReal                 != nullptr
                && fmi2SetInteger              != nullptr
                && fmi2SetBoolean              != nullptr
                && fmi2SetString               != nullptr
                && fmi2DoStep                  != nullptr;
        }
    };

    /// @brief Resolved FMI 3.0 co-simulation entry points (raw `void*` symbols).
    /// @req FR-01.3
    struct Fmi3Functions {
        void* fmi3GetVersion{nullptr};
        void* fmi3SetDebugLogging{nullptr};
        void* fmi3InstantiateCoSimulation{nullptr};
        void* fmi3FreeInstance{nullptr};
        void* fmi3EnterInitializationMode{nullptr};
        void* fmi3ExitInitializationMode{nullptr};
        void* fmi3Terminate{nullptr};
        void* fmi3Reset{nullptr};
        void* fmi3GetFloat64{nullptr};
        void* fmi3GetFloat32{nullptr};
        void* fmi3GetInt8{nullptr};
        void* fmi3GetInt16{nullptr};
        void* fmi3GetInt32{nullptr};
        void* fmi3GetInt64{nullptr};
        void* fmi3GetUInt8{nullptr};
        void* fmi3GetUInt16{nullptr};
        void* fmi3GetUInt32{nullptr};
        void* fmi3GetUInt64{nullptr};
        void* fmi3GetBoolean{nullptr};
        void* fmi3GetString{nullptr};
        void* fmi3GetBinary{nullptr};
        void* fmi3GetClock{nullptr};
        void* fmi3SetFloat64{nullptr};
        void* fmi3SetFloat32{nullptr};
        void* fmi3SetInt8{nullptr};
        void* fmi3SetInt16{nullptr};
        void* fmi3SetInt32{nullptr};
        void* fmi3SetInt64{nullptr};
        void* fmi3SetUInt8{nullptr};
        void* fmi3SetUInt16{nullptr};
        void* fmi3SetUInt32{nullptr};
        void* fmi3SetUInt64{nullptr};
        void* fmi3SetBoolean{nullptr};
        void* fmi3SetString{nullptr};
        void* fmi3SetBinary{nullptr};
        void* fmi3SetClock{nullptr};
        void* fmi3DoStep{nullptr};

        /// @brief Returns `true` if the minimum required FMI 3.0 symbol set was resolved.
        /// @req FR-01.3
        [[nodiscard]] bool isComplete() const noexcept {
            return fmi3GetVersion              != nullptr
                && fmi3SetDebugLogging         != nullptr
                && fmi3InstantiateCoSimulation != nullptr
                && fmi3FreeInstance            != nullptr
                && fmi3EnterInitializationMode != nullptr
                && fmi3ExitInitializationMode  != nullptr
                && fmi3Terminate               != nullptr
                && fmi3Reset                   != nullptr
                && fmi3GetFloat64              != nullptr
                && fmi3SetFloat64              != nullptr
                && fmi3DoStep                  != nullptr;
        }
    };

    FmuPackage() = default;
    ~FmuPackage() = default;

    FmuPackage(const FmuPackage&)            = delete; ///< Non-copyable (owns parsed state).
    FmuPackage& operator=(const FmuPackage&) = delete;
    FmuPackage(FmuPackage&&)                 = default; ///< Movable.
    FmuPackage& operator=(FmuPackage&&)      = default;

    FmiVersion  fmiVersion{FmiVersion::FMI2}; ///< FMI specification version of this FMU.

    std::string modelName;             ///< Model name as declared in the XML.
    std::string modelIdentifier;       ///< Identifier used to name the shared library.
    std::string guid;                  ///< Globally unique identifier of the FMU.
    std::string description;           ///< Human-readable model description.
    std::string generationTool;        ///< Tool that generated the FMU (may be empty).
    std::string generationDateAndTime; ///< ISO 8601 generation timestamp (may be empty).
    std::string variableNamingConvention; ///< `"flat"` or `"structured"`.

    uint32_t    numberOfEventIndicators{0}; ///< Number of event indicators (FMI 2.0 only).

    std::vector<ModelVariable>  variables;       ///< All parsed model variables.
    std::vector<LogCategory>    logCategories;   ///< Declared log categories.
    std::vector<UnitDef>        unitDefinitions; ///< Parsed `<UnitDefinitions>` entries.
    std::vector<TypeDef>        typeDefinitions; ///< Parsed named type definitions.
    DomainDefaultExperiment     defaultExperiment; ///< Recommended experiment parameters.

    std::string rawModelDescription; ///< Verbatim `modelDescription.xml` text (for XSD validation).

    std::filesystem::path extractedPath;     ///< Temporary directory the archive was extracted to.
    std::filesystem::path sharedLibraryPath; ///< Resolved path to the platform shared library.

    /// @brief Raw OS shared library handle. @warning NOT owned by FmuPackage.
    void* sharedLibHandle{nullptr};

    std::optional<Fmi2Functions> fmi2; ///< Resolved FMI 2.0 symbols (set when #isFmi2()).
    std::optional<Fmi3Functions> fmi3; ///< Resolved FMI 3.0 symbols (set when #isFmi3()).

    /// @brief Returns `true` if this is an FMI 2.0 FMU.
    /// @req FR-01.4
    [[nodiscard]] bool isFmi2() const noexcept { return fmiVersion == FmiVersion::FMI2; }
    /// @brief Returns `true` if this is an FMI 3.0 FMU.
    /// @req FR-01.4
    [[nodiscard]] bool isFmi3() const noexcept { return fmiVersion == FmiVersion::FMI3; }

    /// @brief Returns non-owning pointers to all variables with `causality == Input`.
    /// @req FR-02.3
    [[nodiscard]] std::vector<const ModelVariable*> inputs()     const;
    /// @brief Returns non-owning pointers to all variables with `causality == Output`.
    /// @req FR-02.3
    [[nodiscard]] std::vector<const ModelVariable*> outputs()    const;
    /// @brief Returns non-owning pointers to all variables with `causality == Parameter`.
    /// @req FR-02.3
    [[nodiscard]] std::vector<const ModelVariable*> parameters() const;
    /// @brief Finds a variable by exact name, or returns `nullptr` if absent.
    /// @req FR-02.3
    [[nodiscard]] const ModelVariable*               findVariable(const std::string& name) const;
};

} // namespace libfmu_toolkit
