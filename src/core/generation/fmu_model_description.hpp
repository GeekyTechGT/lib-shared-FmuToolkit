#pragma once

/// @file fmu_model_description.hpp
/// @brief Internal model and builders for JSON-driven FMU artifact generation.
/// @internal
///
/// Declares the in-memory model parsed from a JSON definition
/// (#JsonModelDescription) plus the stateless builders that turn it into an FMI
/// `modelDescription.xml` and a C accessor source file. These back the public
/// generation API in `<libfmu_toolkit/fmu_generation.hpp>`.
///
/// @par Requirements
/// Supports FR-08 (JSON-driven generation of modelDescription.xml and C code).

#include "core/domain/fmu_package.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace libfmu_toolkit {

/// @brief A scalar start value in a JSON variable specification.
/// @req FR-08.1
using JsonScalarValue = std::variant<double, int64_t, bool, std::string>;

/// @brief One variable as specified in the input JSON model definition.
/// @req FR-08.1
struct JsonVariableSpec {
    std::string                     name;           ///< Variable name.
    std::string                     description;    ///< Optional description.
    std::optional<uint32_t>         valueReference; ///< Explicit value reference (auto-assigned if unset).
    Causality                       causality{Causality::Local};        ///< Declared causality.
    Variability                     variability{Variability::Continuous}; ///< Declared variability.
    Initial                         initial{Initial::None};             ///< Declared initial kind.
    DataType                        dataType{DataType::Real};            ///< Declared data type.
    std::string                     unit;           ///< Physical unit name (optional).
    std::string                     declaredType;   ///< Referenced type definition (optional).
    std::vector<uint64_t>           dimensions;     ///< Array dimensions (FMI 3.0; empty = scalar).
    std::vector<JsonScalarValue>    startValues;    ///< Start value(s); multiple for arrays.
};

/// @brief The full FMU model as parsed from a JSON definition file.
/// @req FR-08.1
struct JsonModelDescription {
    FmiVersion                  fmiVersion{FmiVersion::FMI2}; ///< Target FMI version.
    std::string                 modelName;             ///< Model name.
    std::string                 modelIdentifier;       ///< C identifier / library name.
    std::string                 guid;                  ///< GUID (generated if empty).
    std::string                 description;           ///< Model description.
    std::string                 author;                ///< Author metadata.
    std::string                 version;               ///< Model version metadata.
    std::string                 copyright;             ///< Copyright metadata.
    std::string                 license;               ///< License metadata.
    std::string                 generationTool;        ///< Generation tool name.
    std::string                 generationDateAndTime; ///< Generation timestamp.
    std::string                 variableNamingConvention{"flat"}; ///< `"flat"` or `"structured"`.
    uint32_t                    numberOfEventIndicators{0}; ///< Event indicator count (FMI 2.0).
    std::vector<UnitDef>        unitDefinitions;       ///< Unit definitions.
    std::vector<TypeDef>        typeDefinitions;       ///< Named type definitions.
    std::vector<LogCategory>    logCategories;         ///< Log categories.
    DomainDefaultExperiment     defaultExperiment;     ///< Default experiment settings.
    std::vector<JsonVariableSpec> variables;           ///< Model variables.
};

/// @brief Parses a JSON model definition file into a JsonModelDescription.
/// @req FR-08.1
class JsonModelDescriptionParser {
public:
    /// @brief Parses @p jsonPath into a JsonModelDescription.
    /// @throws FmuValidationException on malformed or invalid JSON.
    /// @req FR-08.1
    [[nodiscard]] static JsonModelDescription parseFile(const std::filesystem::path& jsonPath);
};

/// @brief Performs semantic validation of a generated package before emission.
/// @req FR-08.3
class ModelDescriptionSemanticValidator {
public:
    /// @brief Validates @p pkg, throwing on the first semantic violation.
    /// @throws FmuValidationException if a semantic rule is violated.
    /// @req FR-08.3
    static void validateOrThrow(const FmuPackage& pkg);
};

/// @brief Builds an FMI `modelDescription.xml` document from a JSON model.
/// @req FR-08.2
class GeneratedModelDescriptionBuilder {
public:
    /// @brief Returns the serialized `modelDescription.xml` for @p model.
    /// @req FR-08.2
    [[nodiscard]] static std::string build(const JsonModelDescription& model);
};

/// @brief Builds FMI-oriented C getter/setter accessor source from a JSON model.
/// @req FR-08.4
class GeneratedCodeBuilder {
public:
    /// @brief Returns the generated C source for @p model.
    /// @req FR-08.4
    [[nodiscard]] static std::string build(const JsonModelDescription& model);
};

/// @brief Sanitizes an arbitrary string into a valid C identifier.
/// @param value Source string (e.g. a model or variable name).
/// @returns A C-identifier-safe transformation of @p value.
/// @req FR-08.4
[[nodiscard]] std::string sanitizeIdentifier(std::string_view value);

} // namespace libfmu_toolkit
