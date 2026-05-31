/// @file generated_code_builder.cpp
/// @brief Implementation of C accessor code generation from a JSON model (FR-08).
/// @internal

#include "fmu_model_description.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <utility>

namespace libfmu_toolkit {

namespace {

struct CodeVariable {
    std::string                  name;
    std::string                  fieldName;
    uint32_t                     valueReference{0};
    Causality                    causality{Causality::Local};
    DataType                     dataType{DataType::Real};
    std::vector<uint64_t>        dimensions;
    std::vector<JsonScalarValue> startValues;
};

[[nodiscard]] size_t flatCount(const std::vector<uint64_t>& dimensions) {
    if (dimensions.empty()) return 1;
    size_t total = 1;
    for (const uint64_t d : dimensions) total *= static_cast<size_t>(d);
    return total;
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
    oss << base;
    for (const uint64_t index : indices) oss << '_' << index;
    return oss.str();
}

[[nodiscard]] std::vector<CodeVariable> buildCodeVariables(const JsonModelDescription& model) {
    std::vector<CodeVariable> variables;
    uint32_t nextAutoValueReference = 0;

    for (const auto& variable : model.variables) {
        if (model.fmiVersion == FmiVersion::FMI2 && !variable.dimensions.empty()) {
            std::vector<std::vector<uint64_t>> indices;
            std::vector<uint64_t> current;
            enumerateIndices(variable.dimensions, 0, current, indices);

            const uint32_t baseVr = variable.valueReference.value_or(nextAutoValueReference);
            nextAutoValueReference = std::max(nextAutoValueReference, baseVr + static_cast<uint32_t>(indices.size()));

            for (size_t offset = 0; offset < indices.size(); ++offset) {
                CodeVariable expanded;
                expanded.name = indexedName(variable.name, indices[offset]);
                expanded.fieldName = sanitizeIdentifier(expanded.name);
                expanded.valueReference = baseVr + static_cast<uint32_t>(offset);
                expanded.causality = variable.causality;
                expanded.dataType = variable.dataType;
                if (!variable.startValues.empty()) expanded.startValues.push_back(variable.startValues[offset]);
                variables.push_back(std::move(expanded));
            }
            continue;
        }

        CodeVariable codeVariable;
        codeVariable.name = variable.name;
        codeVariable.fieldName = sanitizeIdentifier(variable.name);
        codeVariable.valueReference = variable.valueReference.value_or(nextAutoValueReference);
        codeVariable.causality = variable.causality;
        codeVariable.dataType = variable.dataType;
        codeVariable.dimensions = variable.dimensions;
        codeVariable.startValues = variable.startValues;
        nextAutoValueReference = std::max(nextAutoValueReference, codeVariable.valueReference + 1);
        variables.push_back(std::move(codeVariable));
    }

    return variables;
}

[[nodiscard]] bool isSettable(Causality causality) {
    return causality == Causality::Input || causality == Causality::Parameter;
}

[[nodiscard]] std::string cTypeForFmi2(DataType dataType) {
    switch (dataType) {
        case DataType::Real: return "fmi2Real";
        case DataType::Integer: return "fmi2Integer";
        case DataType::Boolean: return "fmi2Boolean";
        case DataType::String: return "fmi2String";
        default: return "fmi2Real";
    }
}

[[nodiscard]] std::string cTypeForFmi3(DataType dataType) {
    switch (dataType) {
        case DataType::Float64:
        case DataType::Real: return "fmi3Float64";
        case DataType::Float32: return "fmi3Float32";
        case DataType::Int8: return "fmi3Int8";
        case DataType::Int16: return "fmi3Int16";
        case DataType::Int32:
        case DataType::Integer: return "fmi3Int32";
        case DataType::Int64: return "fmi3Int64";
        case DataType::UInt8: return "fmi3UInt8";
        case DataType::UInt16: return "fmi3UInt16";
        case DataType::UInt32: return "fmi3UInt32";
        case DataType::UInt64: return "fmi3UInt64";
        case DataType::Boolean: return "fmi3Boolean";
        case DataType::String: return "fmi3String";
        default: return "fmi3Float64";
    }
}

[[nodiscard]] std::string fmi2Suffix(DataType dataType) {
    switch (dataType) {
        case DataType::Real: return "real";
        case DataType::Integer: return "integer";
        case DataType::Boolean: return "boolean";
        case DataType::String: return "string";
        default: return "real";
    }
}

[[nodiscard]] std::string fmi3Suffix(DataType dataType) {
    switch (dataType) {
        case DataType::Real:
        case DataType::Float64: return "float64";
        case DataType::Float32: return "float32";
        case DataType::Int8: return "int8";
        case DataType::Int16: return "int16";
        case DataType::Integer:
        case DataType::Int32: return "int32";
        case DataType::Int64: return "int64";
        case DataType::UInt8: return "uint8";
        case DataType::UInt16: return "uint16";
        case DataType::UInt32: return "uint32";
        case DataType::UInt64: return "uint64";
        case DataType::Boolean: return "boolean";
        case DataType::String: return "string";
        default: return "float64";
    }
}

[[nodiscard]] std::string scalarLiteral(const JsonScalarValue& value, DataType dataType, FmiVersion version) {
    return std::visit([&](const auto& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, double>) {
            std::ostringstream oss;
            oss.precision(17);
            oss << arg;
            if (version == FmiVersion::FMI3 && dataType == DataType::Float32) oss << 'f';
            return oss.str();
        } else if constexpr (std::is_same_v<T, int64_t>) {
            if (version == FmiVersion::FMI3) {
                if (dataType == DataType::Int64) return std::to_string(arg) + "LL";
                if (dataType == DataType::UInt64) return std::to_string(static_cast<unsigned long long>(arg)) + "ULL";
                if (dataType == DataType::UInt32 || dataType == DataType::UInt16 || dataType == DataType::UInt8) {
                    return std::to_string(static_cast<unsigned long long>(arg)) + "U";
                }
            }
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, bool>) {
            if (version == FmiVersion::FMI2) return arg ? "fmi2True" : "fmi2False";
            return arg ? "fmi3True" : "fmi3False";
        } else {
            std::string escaped = "\"";
            for (const char c : arg) {
                if (c == '\\' || c == '"') escaped.push_back('\\');
                escaped.push_back(c);
            }
            escaped.push_back('"');
            return escaped;
        }
    }, value);
}

void appendFieldDeclaration(std::ostringstream& oss, const CodeVariable& variable, FmiVersion version) {
    const std::string cType = version == FmiVersion::FMI2 ? cTypeForFmi2(variable.dataType) : cTypeForFmi3(variable.dataType);
    oss << "    " << cType << ' ' << variable.fieldName;
    const size_t count = flatCount(variable.dimensions);
    if (count > 1) oss << '[' << count << ']';
    oss << ";\n";
}

void appendDefaults(std::ostringstream& oss, const std::string& structName, const std::string& functionPrefix, const std::vector<CodeVariable>& variables, FmiVersion version) {
    oss << "void " << functionPrefix << "_set_defaults(" << structName << "* data) {\n";
    oss << "    if (data == NULL) {\n";
    oss << "        return;\n";
    oss << "    }\n";
    for (const auto& variable : variables) {
        if (variable.startValues.empty()) continue;
        const size_t count = flatCount(variable.dimensions);
        if (count == 1) {
            oss << "    data->" << variable.fieldName << " = " << scalarLiteral(variable.startValues.front(), variable.dataType, version) << ";\n";
            continue;
        }
        for (size_t i = 0; i < variable.startValues.size(); ++i) {
            oss << "    data->" << variable.fieldName << '[' << i << "] = "
                << scalarLiteral(variable.startValues[i], variable.dataType, version) << ";\n";
        }
    }
    oss << "}\n\n";
}

void appendFmi2Accessor(
    std::ostringstream&            oss,
    const std::string&             structName,
    const std::string&             functionPrefix,
    const std::vector<CodeVariable>& variables,
    DataType                       dataType,
    bool                           setter)
{
    const std::string suffix = fmi2Suffix(dataType);
    const std::string cType  = cTypeForFmi2(dataType);
    oss << "fmi2Status " << functionPrefix << '_' << (setter ? "set_" : "get_") << suffix
        << '(' << structName << "* data, const fmi2ValueReference vr[], size_t nvr, "
        << (setter ? "const " : "") << cType << " value[]) {\n";
    oss << "    if (data == NULL || vr == NULL || value == NULL) {\n";
    oss << "        return fmi2Error;\n";
    oss << "    }\n";
    oss << "    for (size_t i = 0; i < nvr; ++i) {\n";
    oss << "        switch (vr[i]) {\n";
    bool hasCase = false;
    for (const auto& variable : variables) {
        if (variable.dataType != dataType) continue;
        hasCase = true;
        oss << "            case " << variable.valueReference << ":\n";
        if (setter && !isSettable(variable.causality)) {
            oss << "                return fmi2Error;\n";
        } else {
            if (setter) {
                oss << "                data->" << variable.fieldName << " = value[i];\n";
            } else {
                oss << "                value[i] = data->" << variable.fieldName << ";\n";
            }
        }
        oss << "                break;\n";
    }
    (void)hasCase;
    oss << "            default:\n";
    oss << "                return fmi2Error;\n";
    oss << "        }\n";
    oss << "    }\n";
    oss << "    return fmi2OK;\n";
    oss << "}\n\n";
}

void appendFmi3Accessor(
    std::ostringstream&              oss,
    const std::string&               structName,
    const std::string&               functionPrefix,
    const std::vector<CodeVariable>& variables,
    DataType                         dataType,
    bool                             setter)
{
    const std::string suffix = fmi3Suffix(dataType);
    const std::string cType  = cTypeForFmi3(dataType);
    oss << "fmi3Status " << functionPrefix << '_' << (setter ? "set_" : "get_") << suffix
        << '(' << structName << "* data, const fmi3ValueReference vr[], size_t nvr, "
        << (setter ? "const " : "") << cType << " values[], size_t nValues) {\n";
    oss << "    size_t valueIndex = 0;\n";
    oss << "    if (data == NULL || vr == NULL || values == NULL) {\n";
    oss << "        return fmi3Error;\n";
    oss << "    }\n";
    oss << "    for (size_t i = 0; i < nvr; ++i) {\n";
    oss << "        switch (vr[i]) {\n";
    bool hasCase = false;
    for (const auto& variable : variables) {
        if (variable.dataType != dataType) continue;
        hasCase = true;
        const size_t count = flatCount(variable.dimensions);
        oss << "            case " << variable.valueReference << ":\n";
        if (setter && !isSettable(variable.causality)) {
            oss << "                return fmi3Error;\n";
        } else {
            oss << "                if (valueIndex + " << count << " > nValues) {\n";
            oss << "                    return fmi3Error;\n";
            oss << "                }\n";
            if (count == 1) {
                if (setter) {
                    oss << "                data->" << variable.fieldName << " = values[valueIndex++];\n";
                } else {
                    oss << "                values[valueIndex++] = data->" << variable.fieldName << ";\n";
                }
            } else {
                oss << "                for (size_t element = 0; element < " << count << "; ++element) {\n";
                if (setter) {
                    oss << "                    data->" << variable.fieldName << "[element] = values[valueIndex++];\n";
                } else {
                    oss << "                    values[valueIndex++] = data->" << variable.fieldName << "[element];\n";
                }
                oss << "                }\n";
            }
        }
        oss << "                break;\n";
    }
    (void)hasCase;
    oss << "            default:\n";
    oss << "                return fmi3Error;\n";
    oss << "        }\n";
    oss << "    }\n";
    oss << "    return valueIndex == nValues ? fmi3OK : fmi3Error;\n";
    oss << "}\n\n";
}

[[nodiscard]] std::vector<DataType> fmi2TypesInOrder() {
    return {DataType::Real, DataType::Integer, DataType::Boolean, DataType::String};
}

[[nodiscard]] std::vector<DataType> fmi3TypesInOrder() {
    return {
        DataType::Float64, DataType::Float32, DataType::Int8, DataType::Int16,
        DataType::Int32, DataType::Int64, DataType::UInt8, DataType::UInt16,
        DataType::UInt32, DataType::UInt64, DataType::Boolean, DataType::String
    };
}

} // namespace

std::string GeneratedCodeBuilder::build(const JsonModelDescription& model) {
    const std::vector<CodeVariable> variables = buildCodeVariables(model);
    const std::string structPrefix = sanitizeIdentifier(model.modelIdentifier);
    const std::string structName   = structPrefix + "Data";
    const std::string functionPrefix = structPrefix;

    std::ostringstream oss;
    oss << "/* Generated by lib-fmu-toolkit from JSON model description. */\n";
    if (model.fmiVersion == FmiVersion::FMI2) {
        oss << "#include \"fmi2FunctionTypes.h\"\n";
    } else {
        oss << "#include \"fmi3FunctionTypes.h\"\n";
    }
    oss << "#include <stddef.h>\n\n";

    oss << "typedef struct " << structName << " {\n";
    for (const auto& variable : variables) {
        appendFieldDeclaration(oss, variable, model.fmiVersion);
    }
    oss << "} " << structName << ";\n\n";

    appendDefaults(oss, structName, functionPrefix, variables, model.fmiVersion);

    if (model.fmiVersion == FmiVersion::FMI2) {
        for (const auto dataType : fmi2TypesInOrder()) {
            appendFmi2Accessor(oss, structName, functionPrefix, variables, dataType, false);
            appendFmi2Accessor(oss, structName, functionPrefix, variables, dataType, true);
        }
    } else {
        for (const auto dataType : fmi3TypesInOrder()) {
            appendFmi3Accessor(oss, structName, functionPrefix, variables, dataType, false);
            appendFmi3Accessor(oss, structName, functionPrefix, variables, dataType, true);
        }
    }

    return oss.str();
}

} // namespace libfmu_toolkit
