#pragma once

/// @file model_description_parser.hpp
/// @brief libxml2-backed parser that populates a FmuPackage from XML.
/// @internal

#include "core/domain/fmu_package.hpp"
#include <string>

namespace libfmu_toolkit {

/// @brief Parses `modelDescription.xml` into the domain FmuPackage.
///
/// Stateless utility (all static methods). Auto-detects the FMI version and
/// dispatches to the FMI 2.0 or FMI 3.0 parsing path. The `void*` node/document
/// parameters carry libxml2 `xmlNodePtr`/`xmlDocPtr` values so that libxml2
/// headers do not leak into this header.
///
/// @par Requirements
/// Implements FR-01.2, FR-02.1 … FR-02.5 (model metadata, variables, units,
/// type definitions, default experiment).
/// @req FR-01.2
class ModelDescriptionParser {
public:
    /// @brief Parses @p xmlContent and fills @p pkg with the parsed metadata.
    /// @throws FmuLoadException if the XML is malformed or the FMI version is unknown.
    /// @req FR-01.2
    static void parse(const std::string& xmlContent, FmuPackage& pkg);

private:
    /// @brief Parses the FMI 2.0 document tree into @p pkg.
    /// @req FR-01.4
    static void parseFmi2(void* doc /*xmlDocPtr*/, FmuPackage& pkg);
    /// @brief Parses the FMI 3.0 document tree into @p pkg.
    /// @req FR-01.4
    static void parseFmi3(void* doc /*xmlDocPtr*/, FmuPackage& pkg);

    /// @brief Parses the `<ModelVariables>` node for FMI 2.0.
    /// @req FR-02.2
    static void parseVariablesFmi2(void* modelVariablesNode, FmuPackage& pkg);
    /// @brief Parses the `<ModelVariables>` node for FMI 3.0.
    /// @req FR-02.2
    static void parseVariablesFmi3(void* modelVariablesNode, FmuPackage& pkg);

    /// @brief Parses `<UnitDefinitions>` into FmuPackage::unitDefinitions.
    /// @req FR-02.4
    static void parseUnitDefinitions(void* rootNode, FmuPackage& pkg);
    /// @brief Parses FMI 2.0 `<TypeDefinitions>`/`<SimpleType>` entries.
    /// @req FR-02.4
    static void parseTypeDefinitionsFmi2(void* rootNode, FmuPackage& pkg);
    /// @brief Parses `<DefaultExperiment>` attributes (with presence flags).
    /// @req FR-02.5
    static void parseDefaultExperiment(void* rootNode, FmuPackage& pkg);

    /// @brief Maps an FMI causality string to the Causality enum.
    /// @req FR-02.2
    static Causality    parseCausality(const char* str);
    /// @brief Maps an FMI variability string to the Variability enum.
    /// @req FR-02.2
    static Variability  parseVariability(const char* str);
    /// @brief Maps an FMI initial string to the Initial enum.
    /// @req FR-02.2
    static Initial      parseInitial(const char* str);
    /// @brief Maps an FMI 2.0 type element name to the DataType enum.
    /// @req FR-02.2
    static DataType     parseDataTypeFmi2(const char* typeNodeName);
    /// @brief Maps an FMI 3.0 type element name to the DataType enum.
    /// @req FR-02.2
    static DataType     parseDataTypeFmi3(const char* typeNodeName);

    /// @brief Parses a double, returning @p fallback on failure (never throws).
    /// @req FR-02.5
    static double parseDouble(const char* str, double fallback = 0.0) noexcept;
};

} // namespace libfmu_toolkit
