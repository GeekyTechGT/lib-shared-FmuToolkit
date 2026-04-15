#pragma once
#include "core/domain/fmu_package.hpp"
#include <string>

namespace libfmu_toolkit {

class ModelDescriptionParser {
public:
    static void parse(const std::string& xmlContent, FmuPackage& pkg);

private:
    static void parseFmi2(void* doc /*xmlDocPtr*/, FmuPackage& pkg);
    static void parseFmi3(void* doc /*xmlDocPtr*/, FmuPackage& pkg);

    static void parseVariablesFmi2(void* modelVariablesNode, FmuPackage& pkg);
    static void parseVariablesFmi3(void* modelVariablesNode, FmuPackage& pkg);

    static void parseUnitDefinitions(void* rootNode, FmuPackage& pkg);
    static void parseTypeDefinitionsFmi2(void* rootNode, FmuPackage& pkg);
    static void parseDefaultExperiment(void* rootNode, FmuPackage& pkg);

    static Causality    parseCausality(const char* str);
    static Variability  parseVariability(const char* str);
    static Initial      parseInitial(const char* str);
    static DataType     parseDataTypeFmi2(const char* typeNodeName);
    static DataType     parseDataTypeFmi3(const char* typeNodeName);

    static double parseDouble(const char* str, double fallback = 0.0) noexcept;
};

} // namespace libfmu_toolkit
