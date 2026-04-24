#pragma once

#include "core/domain/fmu_package.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace libfmu_toolkit {

using JsonScalarValue = std::variant<double, int64_t, bool, std::string>;

struct JsonVariableSpec {
    std::string                     name;
    std::string                     description;
    std::optional<uint32_t>         valueReference;
    Causality                       causality{Causality::Local};
    Variability                     variability{Variability::Continuous};
    Initial                         initial{Initial::None};
    DataType                        dataType{DataType::Real};
    std::string                     unit;
    std::string                     declaredType;
    std::vector<uint64_t>           dimensions;
    std::vector<JsonScalarValue>    startValues;
};

struct JsonModelDescription {
    FmiVersion                  fmiVersion{FmiVersion::FMI2};
    std::string                 modelName;
    std::string                 modelIdentifier;
    std::string                 guid;
    std::string                 description;
    std::string                 author;
    std::string                 version;
    std::string                 copyright;
    std::string                 license;
    std::string                 generationTool;
    std::string                 generationDateAndTime;
    std::string                 variableNamingConvention{"flat"};
    uint32_t                    numberOfEventIndicators{0};
    std::vector<UnitDef>        unitDefinitions;
    std::vector<TypeDef>        typeDefinitions;
    std::vector<LogCategory>    logCategories;
    DomainDefaultExperiment     defaultExperiment;
    std::vector<JsonVariableSpec> variables;
};

class JsonModelDescriptionParser {
public:
    [[nodiscard]] static JsonModelDescription parseFile(const std::filesystem::path& jsonPath);
};

class ModelDescriptionSemanticValidator {
public:
    static void validateOrThrow(const FmuPackage& pkg);
};

class GeneratedModelDescriptionBuilder {
public:
    [[nodiscard]] static std::string build(const JsonModelDescription& model);
};

class GeneratedCodeBuilder {
public:
    [[nodiscard]] static std::string build(const JsonModelDescription& model);
};

[[nodiscard]] std::string sanitizeIdentifier(std::string_view value);

} // namespace libfmu_toolkit
