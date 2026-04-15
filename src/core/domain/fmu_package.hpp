#pragma once

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

// ---------------------------------------------------------------------------
// UnitDef — a physical unit parsed from <UnitDefinitions>
// ---------------------------------------------------------------------------

struct UnitDef {
    std::string name;
    double      factor{1.0};
    double      offset{0.0};
};

// ---------------------------------------------------------------------------
// TypeDef — a named type from <TypeDefinitions> / <SimpleType>
// ---------------------------------------------------------------------------

struct TypeDef {
    std::string name;
    std::string description;
    DataType    dataType{DataType::Real};
    std::string unit;
    std::string quantity;
    std::string displayUnit;
    std::string minStr;
    std::string maxStr;
};

// ---------------------------------------------------------------------------
// DomainDefaultExperiment — default simulation settings from XML
// ---------------------------------------------------------------------------

struct DomainDefaultExperiment {
    double startTime{0.0};
    double stopTime{1.0};
    double stepSize{0.0};
    double tolerance{0.0};
    bool   hasStartTime{false};
    bool   hasStopTime{false};
    bool   hasStepSize{false};
    bool   hasTolerance{false};
};

// ---------------------------------------------------------------------------
// FmuPackage
//
// Aggregates everything extracted and parsed from an FMU archive.
// Non-copyable but movable. The shared library handle is NOT owned here.
// ---------------------------------------------------------------------------

class FmuPackage {
public:
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

    FmuPackage(const FmuPackage&)            = delete;
    FmuPackage& operator=(const FmuPackage&) = delete;
    FmuPackage(FmuPackage&&)                 = default;
    FmuPackage& operator=(FmuPackage&&)      = default;

    FmiVersion  fmiVersion{FmiVersion::FMI2};

    std::string modelName;
    std::string modelIdentifier;
    std::string guid;
    std::string description;
    std::string generationTool;
    std::string generationDateAndTime;
    std::string variableNamingConvention;

    uint32_t    numberOfEventIndicators{0};

    std::vector<ModelVariable>  variables;
    std::vector<LogCategory>    logCategories;
    std::vector<UnitDef>        unitDefinitions;
    std::vector<TypeDef>        typeDefinitions;
    DomainDefaultExperiment     defaultExperiment;

    std::string rawModelDescription;

    std::filesystem::path extractedPath;
    std::filesystem::path sharedLibraryPath;

    /// Raw OS handle — NOT owned by FmuPackage.
    void* sharedLibHandle{nullptr};

    std::optional<Fmi2Functions> fmi2;
    std::optional<Fmi3Functions> fmi3;

    [[nodiscard]] bool isFmi2() const noexcept { return fmiVersion == FmiVersion::FMI2; }
    [[nodiscard]] bool isFmi3() const noexcept { return fmiVersion == FmiVersion::FMI3; }

    [[nodiscard]] std::vector<const ModelVariable*> inputs()     const;
    [[nodiscard]] std::vector<const ModelVariable*> outputs()    const;
    [[nodiscard]] std::vector<const ModelVariable*> parameters() const;
    [[nodiscard]] const ModelVariable*               findVariable(const std::string& name) const;
};

} // namespace libfmu_toolkit
