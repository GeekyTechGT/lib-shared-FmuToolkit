/// @file generated_model_description_builder.cpp
/// @brief Implementation of modelDescription.xml generation from a JSON model (FR-08).
/// @internal

#include "fmu_model_description.hpp"

#include "domain/model/fmu_variable_types.hpp"

#include <algorithm>
#include <sstream>
#include <utility>

namespace libfmu_toolkit {

namespace {

struct ExpandedVariable {
    std::string                  name;
    std::string                  description;
    uint32_t                     valueReference{0};
    Causality                    causality{Causality::Local};
    Variability                  variability{Variability::Continuous};
    Initial                      initial{Initial::None};
    DataType                     dataType{DataType::Real};
    std::string                  unit;
    std::string                  declaredType;
    std::vector<uint64_t>        dimensions;
    std::vector<JsonScalarValue> startValues;
};

[[nodiscard]] std::string xmlEscape(std::string_view value) {
    std::string out;
    out.reserve(value.size() + 16);
    for (const char c : value) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

template<typename T>
void appendAttr(std::ostringstream& oss, std::string_view name, const T& value) {
    oss << ' ' << name << "=\"" << value << '"';
}

void appendOptAttr(std::ostringstream& oss, std::string_view name, std::string_view value) {
    if (!value.empty()) appendAttr(oss, name, xmlEscape(value));
}

[[nodiscard]] std::string doubleToString(double value) {
    std::ostringstream oss;
    oss.precision(17);
    oss << value;
    return oss.str();
}

[[nodiscard]] std::string scalarToString(const JsonScalarValue& value) {
    return std::visit([](const auto& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, double>) {
            return doubleToString(arg);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? "true" : "false";
        } else {
            return xmlEscape(arg);
        }
    }, value);
}

[[nodiscard]] std::string baseTypeElementNameFmi2(DataType dataType) {
    switch (dataType) {
        case DataType::Real: return "Real";
        case DataType::Integer: return "Integer";
        case DataType::Boolean: return "Boolean";
        case DataType::String: return "String";
        default: return "Real";
    }
}

[[nodiscard]] std::string baseTypeElementNameFmi3(DataType dataType) {
    return std::string(to_string(dataType));
}

void enumerateIndices(
    const std::vector<uint64_t>& dimensions,
    size_t                       depth,
    std::vector<uint64_t>&       current,
    std::vector<std::vector<uint64_t>>& out)
{
    if (depth == dimensions.size()) {
        out.push_back(current);
        return;
    }
    for (uint64_t i = 1; i <= dimensions[depth]; ++i) {
        current.push_back(i);
        enumerateIndices(dimensions, depth + 1, current, out);
        current.pop_back();
    }
}

[[nodiscard]] std::string indexedName(std::string_view base, const std::vector<uint64_t>& indices) {
    if (indices.empty()) return std::string(base);
    std::ostringstream oss;
    oss << base << '[';
    for (size_t i = 0; i < indices.size(); ++i) {
        if (i != 0) oss << ',';
        oss << indices[i];
    }
    oss << ']';
    return oss.str();
}

[[nodiscard]] std::vector<ExpandedVariable> expandVariables(const JsonModelDescription& model) {
    std::vector<ExpandedVariable> out;
    uint32_t nextAutoValueReference = 0;

    for (const auto& variable : model.variables) {
        if (model.fmiVersion == FmiVersion::FMI2 && !variable.dimensions.empty()) {
            std::vector<std::vector<uint64_t>> indices;
            std::vector<uint64_t> current;
            enumerateIndices(variable.dimensions, 0, current, indices);

            const uint32_t baseVr = variable.valueReference.value_or(nextAutoValueReference);
            nextAutoValueReference = std::max(nextAutoValueReference, baseVr + static_cast<uint32_t>(indices.size()));

            for (size_t offset = 0; offset < indices.size(); ++offset) {
                ExpandedVariable expanded;
                expanded.name          = indexedName(variable.name, indices[offset]);
                expanded.description   = variable.description;
                expanded.valueReference = baseVr + static_cast<uint32_t>(offset);
                expanded.causality     = variable.causality;
                expanded.variability   = variable.variability;
                expanded.initial       = variable.initial;
                expanded.dataType      = variable.dataType;
                expanded.unit          = variable.unit;
                expanded.declaredType  = variable.declaredType;
                if (!variable.startValues.empty()) {
                    expanded.startValues.push_back(variable.startValues[offset]);
                }
                out.push_back(std::move(expanded));
            }
            continue;
        }

        ExpandedVariable expanded;
        expanded.name           = variable.name;
        expanded.description    = variable.description;
        expanded.valueReference = variable.valueReference.value_or(nextAutoValueReference);
        expanded.causality      = variable.causality;
        expanded.variability    = variable.variability;
        expanded.initial        = variable.initial;
        expanded.dataType       = variable.dataType;
        expanded.unit           = variable.unit;
        expanded.declaredType   = variable.declaredType;
        expanded.dimensions     = variable.dimensions;
        expanded.startValues    = variable.startValues;
        nextAutoValueReference  = std::max(nextAutoValueReference, expanded.valueReference + 1);
        out.push_back(std::move(expanded));
    }

    return out;
}

void appendDefaultExperiment(std::ostringstream& oss, const DomainDefaultExperiment& experiment) {
    if (!experiment.hasStartTime && !experiment.hasStopTime
        && !experiment.hasStepSize && !experiment.hasTolerance) {
        return;
    }

    oss << "  <DefaultExperiment";
    if (experiment.hasStartTime) appendAttr(oss, "startTime", doubleToString(experiment.startTime));
    if (experiment.hasStopTime) appendAttr(oss, "stopTime", doubleToString(experiment.stopTime));
    if (experiment.hasStepSize) appendAttr(oss, "stepSize", doubleToString(experiment.stepSize));
    if (experiment.hasTolerance) appendAttr(oss, "tolerance", doubleToString(experiment.tolerance));
    oss << "/>\n";
}

void appendUnitDefinitions(std::ostringstream& oss, const std::vector<UnitDef>& units) {
    if (units.empty()) return;
    oss << "  <UnitDefinitions>\n";
    for (const auto& unit : units) {
        oss << "    <Unit";
        appendAttr(oss, "name", xmlEscape(unit.name));
        oss << ">\n";
        oss << "      <BaseUnit";
        appendAttr(oss, "factor", doubleToString(unit.factor));
        appendAttr(oss, "offset", doubleToString(unit.offset));
        oss << "/>\n";
        oss << "    </Unit>\n";
    }
    oss << "  </UnitDefinitions>\n";
}

void appendTypeDefinitionsFmi2(std::ostringstream& oss, const std::vector<TypeDef>& typeDefinitions) {
    if (typeDefinitions.empty()) return;
    oss << "  <TypeDefinitions>\n";
    for (const auto& type : typeDefinitions) {
        oss << "    <SimpleType";
        appendAttr(oss, "name", xmlEscape(type.name));
        appendOptAttr(oss, "description", type.description);
        oss << ">\n";
        oss << "      <" << baseTypeElementNameFmi2(type.dataType);
        appendOptAttr(oss, "unit", type.unit);
        appendOptAttr(oss, "quantity", type.quantity);
        appendOptAttr(oss, "displayUnit", type.displayUnit);
        appendOptAttr(oss, "min", type.minStr);
        appendOptAttr(oss, "max", type.maxStr);
        oss << "/>\n";
        oss << "    </SimpleType>\n";
    }
    oss << "  </TypeDefinitions>\n";
}

void appendTypeDefinitionsFmi3(std::ostringstream& oss, const std::vector<TypeDef>& typeDefinitions) {
    if (typeDefinitions.empty()) return;
    oss << "  <TypeDefinitions>\n";
    for (const auto& type : typeDefinitions) {
        oss << "    <" << baseTypeElementNameFmi3(type.dataType) << "Type";
        appendAttr(oss, "name", xmlEscape(type.name));
        appendOptAttr(oss, "description", type.description);
        appendOptAttr(oss, "unit", type.unit);
        appendOptAttr(oss, "quantity", type.quantity);
        appendOptAttr(oss, "displayUnit", type.displayUnit);
        appendOptAttr(oss, "min", type.minStr);
        appendOptAttr(oss, "max", type.maxStr);
        oss << "/>\n";
    }
    oss << "  </TypeDefinitions>\n";
}

void appendLogCategories(std::ostringstream& oss, const std::vector<LogCategory>& logCategories) {
    if (logCategories.empty()) return;
    oss << "  <LogCategories>\n";
    for (const auto& category : logCategories) {
        oss << "    <Category";
        appendAttr(oss, "name", xmlEscape(category.name));
        appendOptAttr(oss, "description", category.description);
        oss << "/>\n";
    }
    oss << "  </LogCategories>\n";
}

void appendModelVariablesFmi2(std::ostringstream& oss, const std::vector<ExpandedVariable>& variables) {
    oss << "  <ModelVariables>\n";
    for (const auto& variable : variables) {
        oss << "    <ScalarVariable";
        appendAttr(oss, "name", xmlEscape(variable.name));
        appendAttr(oss, "valueReference", variable.valueReference);
        appendAttr(oss, "causality", to_string(variable.causality));
        appendAttr(oss, "variability", to_string(variable.variability));
        if (variable.initial != Initial::None) appendAttr(oss, "initial", to_string(variable.initial));
        appendOptAttr(oss, "description", variable.description);
        oss << ">\n";
        oss << "      <" << baseTypeElementNameFmi2(variable.dataType);
        appendOptAttr(oss, "unit", variable.unit);
        appendOptAttr(oss, "declaredType", variable.declaredType);
        if (!variable.startValues.empty()) appendAttr(oss, "start", scalarToString(variable.startValues.front()));
        oss << "/>\n";
        oss << "    </ScalarVariable>\n";
    }
    oss << "  </ModelVariables>\n";
}

void appendModelVariablesFmi3(std::ostringstream& oss, const std::vector<ExpandedVariable>& variables) {
    oss << "  <ModelVariables>\n";
    for (const auto& variable : variables) {
        oss << "    <" << baseTypeElementNameFmi3(variable.dataType);
        appendAttr(oss, "name", xmlEscape(variable.name));
        appendAttr(oss, "valueReference", variable.valueReference);
        appendAttr(oss, "causality", to_string(variable.causality));
        appendAttr(oss, "variability", to_string(variable.variability));
        if (variable.initial != Initial::None) appendAttr(oss, "initial", to_string(variable.initial));
        appendOptAttr(oss, "description", variable.description);
        appendOptAttr(oss, "unit", variable.unit);
        appendOptAttr(oss, "declaredType", variable.declaredType);

        if (variable.dimensions.empty() && variable.dataType != DataType::String) {
            if (!variable.startValues.empty()) appendAttr(oss, "start", scalarToString(variable.startValues.front()));
            oss << "/>\n";
            continue;
        }

        if (!variable.dimensions.empty() && !variable.startValues.empty() && variable.dataType != DataType::String) {
            std::ostringstream start;
            for (size_t i = 0; i < variable.startValues.size(); ++i) {
                if (i != 0) start << ' ';
                start << scalarToString(variable.startValues[i]);
            }
            appendAttr(oss, "start", start.str());
        }

        oss << ">\n";
        for (const uint64_t dimension : variable.dimensions) {
            oss << "      <Dimension";
            appendAttr(oss, "start", dimension);
            oss << "/>\n";
        }
        if (!variable.startValues.empty() && variable.dataType == DataType::String) {
            for (const auto& start : variable.startValues) {
                oss << "      <Start";
                appendAttr(oss, "value", scalarToString(start));
                oss << "/>\n";
            }
        }
        oss << "    </" << baseTypeElementNameFmi3(variable.dataType) << ">\n";
    }
    oss << "  </ModelVariables>\n";
}

void appendModelStructureFmi2(std::ostringstream& oss, const std::vector<ExpandedVariable>& variables) {
    oss << "  <ModelStructure>\n";
    bool hasOutputs = false;
    for (const auto& variable : variables) {
        if (variable.causality == Causality::Output) {
            hasOutputs = true;
            break;
        }
    }
    if (hasOutputs) {
        oss << "    <Outputs>\n";
        for (size_t i = 0; i < variables.size(); ++i) {
            if (variables[i].causality != Causality::Output) continue;
            oss << "      <Unknown";
            appendAttr(oss, "index", static_cast<uint32_t>(i + 1));
            oss << "/>\n";
        }
        oss << "    </Outputs>\n";
    }

    bool hasInitialUnknowns = false;
    for (const auto& variable : variables) {
        if (variable.causality == Causality::CalculatedParameter
            || (variable.causality == Causality::Output
                && (variable.initial == Initial::Approx || variable.initial == Initial::Calculated))) {
            hasInitialUnknowns = true;
            break;
        }
    }
    if (hasInitialUnknowns) {
        oss << "    <InitialUnknowns>\n";
        for (size_t i = 0; i < variables.size(); ++i) {
            const auto& variable = variables[i];
            if (variable.causality != Causality::CalculatedParameter
                && !(variable.causality == Causality::Output
                    && (variable.initial == Initial::Approx || variable.initial == Initial::Calculated))) {
                continue;
            }
            oss << "      <Unknown";
            appendAttr(oss, "index", static_cast<uint32_t>(i + 1));
            oss << "/>\n";
        }
        oss << "    </InitialUnknowns>\n";
    }
    oss << "  </ModelStructure>\n";
}

void appendModelStructureFmi3(std::ostringstream& oss, const std::vector<ExpandedVariable>& variables) {
    oss << "  <ModelStructure>\n";
    for (const auto& variable : variables) {
        if (variable.causality != Causality::Output) continue;
        oss << "    <Output";
        appendAttr(oss, "valueReference", variable.valueReference);
        oss << "/>\n";
    }
    for (const auto& variable : variables) {
        if (variable.causality != Causality::CalculatedParameter
            && !(variable.causality == Causality::Output
                && (variable.initial == Initial::Approx || variable.initial == Initial::Calculated))) {
            continue;
        }
        oss << "    <InitialUnknown";
        appendAttr(oss, "valueReference", variable.valueReference);
        oss << "/>\n";
    }
    oss << "  </ModelStructure>\n";
}

} // namespace

std::string GeneratedModelDescriptionBuilder::build(const JsonModelDescription& model) {
    const std::vector<ExpandedVariable> variables = expandVariables(model);

    std::ostringstream oss;
    oss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    oss << "<fmiModelDescription";

    if (model.fmiVersion == FmiVersion::FMI2) {
        appendAttr(oss, "fmiVersion", "2.0");
        appendAttr(oss, "modelName", xmlEscape(model.modelName));
        appendAttr(oss, "guid", xmlEscape(model.guid));
        appendOptAttr(oss, "description", model.description);
        appendOptAttr(oss, "author", model.author);
        appendOptAttr(oss, "version", model.version);
        appendOptAttr(oss, "generationTool", model.generationTool);
        appendOptAttr(oss, "generationDateAndTime", model.generationDateAndTime);
        appendOptAttr(oss, "variableNamingConvention", model.variableNamingConvention);
        appendAttr(oss, "numberOfEventIndicators", model.numberOfEventIndicators);
        oss << ">\n";
        oss << "  <CoSimulation";
        appendAttr(oss, "modelIdentifier", xmlEscape(model.modelIdentifier));
        oss << "/>\n";
        appendUnitDefinitions(oss, model.unitDefinitions);
        appendTypeDefinitionsFmi2(oss, model.typeDefinitions);
        appendLogCategories(oss, model.logCategories);
        appendDefaultExperiment(oss, model.defaultExperiment);
        appendModelVariablesFmi2(oss, variables);
        appendModelStructureFmi2(oss, variables);
    } else {
        appendAttr(oss, "fmiVersion", "3.0");
        appendAttr(oss, "modelName", xmlEscape(model.modelName));
        appendAttr(oss, "instantiationToken", xmlEscape(model.guid));
        appendOptAttr(oss, "description", model.description);
        appendOptAttr(oss, "author", model.author);
        appendOptAttr(oss, "version", model.version);
        appendOptAttr(oss, "copyright", model.copyright);
        appendOptAttr(oss, "license", model.license);
        appendOptAttr(oss, "generationTool", model.generationTool);
        appendOptAttr(oss, "generationDateAndTime", model.generationDateAndTime);
        appendOptAttr(oss, "variableNamingConvention", model.variableNamingConvention);
        oss << ">\n";
        oss << "  <CoSimulation";
        appendAttr(oss, "modelIdentifier", xmlEscape(model.modelIdentifier));
        oss << "/>\n";
        appendUnitDefinitions(oss, model.unitDefinitions);
        appendTypeDefinitionsFmi3(oss, model.typeDefinitions);
        appendLogCategories(oss, model.logCategories);
        appendDefaultExperiment(oss, model.defaultExperiment);
        appendModelVariablesFmi3(oss, variables);
        appendModelStructureFmi3(oss, variables);
    }

    oss << "</fmiModelDescription>\n";
    return oss.str();
}

} // namespace libfmu_toolkit
