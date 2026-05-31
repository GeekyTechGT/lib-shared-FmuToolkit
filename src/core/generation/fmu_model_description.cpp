/// @file fmu_model_description.cpp
/// @brief Implementation of JSON model parsing and identifier sanitization (FR-08).
/// @internal

#include "fmu_model_description.hpp"

#include "core/domain/fmu_variable_types.hpp"
#include "core/exceptions/fmu_exception.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

namespace libfmu_toolkit {

namespace {

using json = nlohmann::json;

[[noreturn]] void fail(std::string_view message) {
    throw FmuLoadException(std::string("JsonModelDescriptionParser: ") + std::string(message));
}

[[nodiscard]] std::string requireString(const json& object, std::string_view key, std::string_view context) {
    const auto it = object.find(std::string(key));
    if (it == object.end()) {
        fail("missing required key '" + std::string(key) + "' in " + std::string(context));
    }
    if (!it->is_string()) {
        fail("expected string for " + std::string(context) + "." + std::string(key));
    }
    return it->get<std::string>();
}

[[nodiscard]] std::string formatCurrentUtc() {
    const auto now = std::chrono::system_clock::now();
    const auto t   = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &t);
#else
    gmtime_r(&t, &utc);
#endif
    std::ostringstream oss;
    oss << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

[[nodiscard]] Causality parseCausalityString(std::string_view value) {
    if (value == "parameter") return Causality::Parameter;
    if (value == "calculatedParameter") return Causality::CalculatedParameter;
    if (value == "input") return Causality::Input;
    if (value == "output") return Causality::Output;
    if (value == "local") return Causality::Local;
    if (value == "independent") return Causality::Independent;
    fail("unsupported causality '" + std::string(value) + "'");
}

[[nodiscard]] Variability parseVariabilityString(std::string_view value) {
    if (value == "constant") return Variability::Constant;
    if (value == "fixed") return Variability::Fixed;
    if (value == "tunable") return Variability::Tunable;
    if (value == "discrete") return Variability::Discrete;
    if (value == "continuous") return Variability::Continuous;
    fail("unsupported variability '" + std::string(value) + "'");
}

[[nodiscard]] Initial parseInitialString(std::string_view value) {
    if (value == "exact") return Initial::Exact;
    if (value == "approx") return Initial::Approx;
    if (value == "calculated") return Initial::Calculated;
    if (value == "none") return Initial::None;
    fail("unsupported initial '" + std::string(value) + "'");
}

[[nodiscard]] DataType parseDataTypeString(std::string_view value) {
    if (value == "Real") return DataType::Real;
    if (value == "Integer") return DataType::Integer;
    if (value == "Boolean") return DataType::Boolean;
    if (value == "String") return DataType::String;
    if (value == "Float64") return DataType::Float64;
    if (value == "Float32") return DataType::Float32;
    if (value == "Int8") return DataType::Int8;
    if (value == "Int16") return DataType::Int16;
    if (value == "Int32") return DataType::Int32;
    if (value == "Int64") return DataType::Int64;
    if (value == "UInt8") return DataType::UInt8;
    if (value == "UInt16") return DataType::UInt16;
    if (value == "UInt32") return DataType::UInt32;
    if (value == "UInt64") return DataType::UInt64;
    if (value == "Binary") return DataType::Binary;
    if (value == "Clock") return DataType::Clock;
    fail("unsupported data type '" + std::string(value) + "'");
}

void validateDataTypeForStandard(DataType dt, FmiVersion version) {
    if (dt == DataType::Binary || dt == DataType::Clock) {
        fail("data type '" + std::string(to_string(dt)) + "' is not supported by the JSON generator");
    }
    if (version == FmiVersion::FMI2) {
        switch (dt) {
            case DataType::Real:
            case DataType::Integer:
            case DataType::Boolean:
            case DataType::String:
                return;
            default:
                fail("data type '" + std::string(to_string(dt)) + "' is not allowed in FMI 2.0");
        }
    }
}

[[nodiscard]] size_t flatElementCount(const std::vector<uint64_t>& dimensions) {
    if (dimensions.empty()) return 1;
    size_t total = 1;
    for (const uint64_t d : dimensions) {
        if (d == 0) fail("array dimensions must be greater than zero");
        if (d > static_cast<uint64_t>(std::numeric_limits<size_t>::max() / total)) {
            fail("array dimensions overflow");
        }
        total *= static_cast<size_t>(d);
    }
    return total;
}

[[nodiscard]] JsonScalarValue parseStartScalar(const json& value) {
    if (value.is_boolean()) return value.get<bool>();
    if (value.is_number_integer() || value.is_number_unsigned()) return value.get<int64_t>();
    if (value.is_number_float()) return value.get<double>();
    if (value.is_string()) return value.get<std::string>();
    fail("start values must be scalar JSON values");
}

void validateStartValues(const JsonVariableSpec& variable, FmiVersion version) {
    validateDataTypeForStandard(variable.dataType, version);

    const size_t expectedCount = flatElementCount(variable.dimensions);
    if (variable.startValues.empty()) return;

    if (expectedCount == 1 && variable.startValues.size() != 1) {
        fail("scalar variable '" + variable.name + "' must have exactly one start value");
    }
    if (expectedCount > 1 && variable.startValues.size() != expectedCount) {
        fail("array variable '" + variable.name + "' expects " + std::to_string(expectedCount) + " start values");
    }

    for (const auto& start : variable.startValues) {
        switch (variable.dataType) {
            case DataType::Real:
            case DataType::Float64:
            case DataType::Float32:
                if (!std::holds_alternative<double>(start) && !std::holds_alternative<int64_t>(start)) {
                    fail("floating-point start values expected for '" + variable.name + "'");
                }
                break;
            case DataType::Integer:
            case DataType::Int8:
            case DataType::Int16:
            case DataType::Int32:
            case DataType::Int64:
            case DataType::UInt8:
            case DataType::UInt16:
            case DataType::UInt32:
            case DataType::UInt64:
                if (!std::holds_alternative<int64_t>(start)) {
                    fail("integer start values expected for '" + variable.name + "'");
                }
                break;
            case DataType::Boolean:
            case DataType::Clock:
                if (!std::holds_alternative<bool>(start)) {
                    fail("boolean start values expected for '" + variable.name + "'");
                }
                break;
            case DataType::String:
            case DataType::Binary:
                if (!std::holds_alternative<std::string>(start)) {
                    fail("string start values expected for '" + variable.name + "'");
                }
                break;
        }
    }
}

[[nodiscard]] std::vector<uint64_t> parseDimensions(const json& value) {
    if (!value.is_array()) fail("expected array for variable.dimensions");
    std::vector<uint64_t> dimensions;
    for (const auto& entry : value) {
        if (!entry.is_number_integer() && !entry.is_number_unsigned()) {
            fail("variable.dimensions entries must be positive integers");
        }
        const auto raw = entry.get<int64_t>();
        if (raw <= 0) fail("variable.dimensions entries must be positive integers");
        dimensions.push_back(static_cast<uint64_t>(raw));
    }
    return dimensions;
}

[[nodiscard]] std::vector<JsonScalarValue> parseStartValues(const json& value) {
    std::vector<JsonScalarValue> out;
    if (value.is_array()) {
        for (const auto& entry : value) out.push_back(parseStartScalar(entry));
        return out;
    }
    out.push_back(parseStartScalar(value));
    return out;
}

[[nodiscard]] JsonVariableSpec parseVariable(const json& object, FmiVersion version) {
    if (!object.is_object()) fail("variables[] entries must be objects");

    JsonVariableSpec variable;
    variable.name = requireString(object, "name", "variable");
    variable.causality = parseCausalityString(requireString(object, "causality", "variable"));
    variable.variability = parseVariabilityString(requireString(object, "variability", "variable"));
    variable.dataType = parseDataTypeString(requireString(object, "type", "variable"));

    if (const auto it = object.find("description"); it != object.end()) variable.description = it->get<std::string>();
    if (const auto it = object.find("valueReference"); it != object.end()) {
        if (!it->is_number_integer() && !it->is_number_unsigned()) fail("expected integer for variable.valueReference");
        const auto raw = it->get<int64_t>();
        if (raw < 0 || raw > static_cast<int64_t>(std::numeric_limits<uint32_t>::max())) fail("out-of-range uint32 for variable.valueReference");
        variable.valueReference = static_cast<uint32_t>(raw);
    }
    if (const auto it = object.find("initial"); it != object.end()) variable.initial = parseInitialString(it->get<std::string>());
    if (const auto it = object.find("unit"); it != object.end()) variable.unit = it->get<std::string>();
    if (const auto it = object.find("declaredType"); it != object.end()) variable.declaredType = it->get<std::string>();
    if (const auto it = object.find("dimensions"); it != object.end()) variable.dimensions = parseDimensions(*it);
    if (const auto it = object.find("start"); it != object.end()) variable.startValues = parseStartValues(*it);

    validateStartValues(variable, version);
    return variable;
}

[[nodiscard]] DomainDefaultExperiment parseDefaultExperiment(const json& root) {
    DomainDefaultExperiment out;
    const auto it = root.find("defaultExperiment");
    if (it == root.end()) return out;
    if (!it->is_object()) fail("defaultExperiment must be an object");

    if (const auto value = it->find("startTime"); value != it->end()) {
        out.startTime = value->get<double>();
        out.hasStartTime = true;
    }
    if (const auto value = it->find("stopTime"); value != it->end()) {
        out.stopTime = value->get<double>();
        out.hasStopTime = true;
    }
    if (const auto value = it->find("stepSize"); value != it->end()) {
        out.stepSize = value->get<double>();
        out.hasStepSize = true;
    }
    if (const auto value = it->find("tolerance"); value != it->end()) {
        out.tolerance = value->get<double>();
        out.hasTolerance = true;
    }
    return out;
}

[[nodiscard]] std::vector<UnitDef> parseUnitDefinitions(const json& root) {
    std::vector<UnitDef> units;
    const auto it = root.find("unitDefinitions");
    if (it == root.end()) return units;
    if (!it->is_array()) fail("unitDefinitions must be an array");

    for (const auto& entry : *it) {
        if (!entry.is_object()) fail("unitDefinitions[] entries must be objects");
        UnitDef unit;
        unit.name = requireString(entry, "name", "unitDefinitions[]");
        if (const auto value = entry.find("factor"); value != entry.end()) unit.factor = value->get<double>();
        if (const auto value = entry.find("offset"); value != entry.end()) unit.offset = value->get<double>();
        units.push_back(std::move(unit));
    }
    return units;
}

[[nodiscard]] std::vector<TypeDef> parseTypeDefinitions(const json& root) {
    std::vector<TypeDef> types;
    const auto it = root.find("typeDefinitions");
    if (it == root.end()) return types;
    if (!it->is_array()) fail("typeDefinitions must be an array");

    for (const auto& entry : *it) {
        if (!entry.is_object()) fail("typeDefinitions[] entries must be objects");
        TypeDef type;
        type.name = requireString(entry, "name", "typeDefinitions[]");
        if (const auto value = entry.find("description"); value != entry.end()) type.description = value->get<std::string>();
        if (const auto value = entry.find("type"); value != entry.end()) type.dataType = parseDataTypeString(value->get<std::string>());
        if (const auto value = entry.find("unit"); value != entry.end()) type.unit = value->get<std::string>();
        if (const auto value = entry.find("quantity"); value != entry.end()) type.quantity = value->get<std::string>();
        if (const auto value = entry.find("displayUnit"); value != entry.end()) type.displayUnit = value->get<std::string>();
        if (const auto value = entry.find("min"); value != entry.end()) type.minStr = value->get<std::string>();
        if (const auto value = entry.find("max"); value != entry.end()) type.maxStr = value->get<std::string>();
        types.push_back(std::move(type));
    }
    return types;
}

[[nodiscard]] std::vector<LogCategory> parseLogCategories(const json& root) {
    std::vector<LogCategory> categories;
    const auto it = root.find("logCategories");
    if (it == root.end()) return categories;
    if (!it->is_array()) fail("logCategories must be an array");

    for (const auto& entry : *it) {
        if (!entry.is_object()) fail("logCategories[] entries must be objects");
        LogCategory category;
        category.name = requireString(entry, "name", "logCategories[]");
        if (const auto value = entry.find("description"); value != entry.end()) category.description = value->get<std::string>();
        categories.push_back(std::move(category));
    }
    return categories;
}

} // namespace

std::string sanitizeIdentifier(std::string_view value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const char c : value) {
        if (std::isalnum(static_cast<unsigned char>(c)) != 0) {
            out.push_back(c);
        } else {
            out.push_back('_');
        }
    }
    if (out.empty() || std::isdigit(static_cast<unsigned char>(out.front())) != 0) {
        out.insert(out.begin(), '_');
    }
    return out;
}

JsonModelDescription JsonModelDescriptionParser::parseFile(const std::filesystem::path& jsonPath) {
    std::ifstream ifs(jsonPath);
    if (!ifs.is_open()) {
        throw FmuLoadException("JsonModelDescriptionParser: cannot open JSON file: " + jsonPath.string());
    }

    json root;
    try {
        ifs >> root;
    } catch (const json::exception& ex) {
        throw FmuLoadException("JsonModelDescriptionParser: invalid JSON: " + std::string(ex.what()));
    }

    if (!root.is_object()) fail("root JSON value must be an object");
    const auto modelIt = root.find("model");
    if (modelIt == root.end() || !modelIt->is_object()) fail("missing required object 'model' in root");
    const json& model = *modelIt;

    JsonModelDescription out;
    const std::string standard = requireString(model, "standard", "model");
    if (standard == "fmi2" || standard == "FMI2" || standard == "2.0") {
        out.fmiVersion = FmiVersion::FMI2;
    } else if (standard == "fmi3" || standard == "FMI3" || standard == "3.0") {
        out.fmiVersion = FmiVersion::FMI3;
    } else {
        fail("unsupported standard '" + standard + "'");
    }

    out.modelName = requireString(model, "name", "model");
    if (const auto it = model.find("identifier"); it != model.end()) {
        out.modelIdentifier = it->get<std::string>();
    } else {
        out.modelIdentifier = sanitizeIdentifier(out.modelName);
    }

    if (out.fmiVersion == FmiVersion::FMI2) {
        out.guid = requireString(model, "guid", "model");
    } else if (const auto tokenIt = model.find("instantiationToken"); tokenIt != model.end()) {
        out.guid = tokenIt->get<std::string>();
    } else if (const auto guidIt = model.find("guid"); guidIt != model.end()) {
        out.guid = guidIt->get<std::string>();
    } else {
        fail("missing instantiationToken/guid for FMI 3.0 model");
    }

    if (const auto it = model.find("description"); it != model.end()) out.description = it->get<std::string>();
    if (const auto it = model.find("author"); it != model.end()) out.author = it->get<std::string>();
    if (const auto it = model.find("version"); it != model.end()) out.version = it->get<std::string>();
    if (const auto it = model.find("copyright"); it != model.end()) out.copyright = it->get<std::string>();
    if (const auto it = model.find("license"); it != model.end()) out.license = it->get<std::string>();
    if (const auto it = model.find("generationTool"); it != model.end()) out.generationTool = it->get<std::string>();
    else out.generationTool = "lib-fmu-toolkit";
    if (const auto it = model.find("generationDateAndTime"); it != model.end()) out.generationDateAndTime = it->get<std::string>();
    else out.generationDateAndTime = formatCurrentUtc();
    if (const auto it = model.find("variableNamingConvention"); it != model.end()) out.variableNamingConvention = it->get<std::string>();
    if (const auto it = model.find("numberOfEventIndicators"); it != model.end()) {
        if (!it->is_number_integer() && !it->is_number_unsigned()) fail("expected integer for model.numberOfEventIndicators");
        const auto raw = it->get<int64_t>();
        if (raw < 0 || raw > static_cast<int64_t>(std::numeric_limits<uint32_t>::max())) fail("out-of-range uint32 for model.numberOfEventIndicators");
        out.numberOfEventIndicators = static_cast<uint32_t>(raw);
    }

    out.defaultExperiment = parseDefaultExperiment(root);
    out.unitDefinitions = parseUnitDefinitions(root);
    out.typeDefinitions = parseTypeDefinitions(root);
    out.logCategories = parseLogCategories(root);

    const auto variablesIt = root.find("variables");
    if (variablesIt == root.end() || !variablesIt->is_array()) fail("variables must be a non-empty array");
    if (variablesIt->empty()) fail("variables array must not be empty");

    std::vector<std::string> names;
    for (const auto& entry : *variablesIt) {
        JsonVariableSpec variable = parseVariable(entry, out.fmiVersion);
        if (std::find(names.begin(), names.end(), variable.name) != names.end()) {
            fail("duplicate variable name '" + variable.name + "'");
        }
        names.push_back(variable.name);
        out.variables.push_back(std::move(variable));
    }

    return out;
}

} // namespace libfmu_toolkit
