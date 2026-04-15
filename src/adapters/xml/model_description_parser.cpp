#include "model_description_parser.hpp"
#include "core/exceptions/fmu_exception.hpp"
#include "core/domain/log_category.hpp"

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

namespace libfmu_toolkit {

namespace {

struct XmlPropGuard {
    xmlChar* ptr{nullptr};
    explicit XmlPropGuard(xmlChar* p) : ptr(p) {}
    ~XmlPropGuard() { if (ptr) xmlFree(ptr); }
    XmlPropGuard(const XmlPropGuard&) = delete;
    XmlPropGuard& operator=(const XmlPropGuard&) = delete;

    [[nodiscard]] const char* c_str() const noexcept {
        return reinterpret_cast<const char*>(ptr);
    }
    [[nodiscard]] bool empty() const noexcept { return !ptr || ptr[0] == '\0'; }
    [[nodiscard]] std::string str() const {
        return ptr ? std::string(reinterpret_cast<const char*>(ptr)) : std::string{};
    }
};

std::string getProp(xmlNodePtr node, const char* attrName) {
    XmlPropGuard g(xmlGetProp(node, reinterpret_cast<const xmlChar*>(attrName)));
    return g.str();
}

bool isTextOrComment(xmlNodePtr node) {
    return node->type == XML_TEXT_NODE
        || node->type == XML_COMMENT_NODE
        || node->type == XML_CDATA_SECTION_NODE;
}

bool nodeName(xmlNodePtr node, const char* name) {
    return std::strcmp(reinterpret_cast<const char*>(node->name), name) == 0;
}

} // anonymous namespace

double ModelDescriptionParser::parseDouble(const char* str, double fallback) noexcept {
    if (!str || !*str) return fallback;
    try { return std::stod(str); } catch (...) { return fallback; }
}

void ModelDescriptionParser::parse(const std::string& xmlContent, FmuPackage& pkg) {
    pkg.rawModelDescription = xmlContent;

    xmlDocPtr doc = xmlParseMemory(xmlContent.c_str(), static_cast<int>(xmlContent.size()));
    if (!doc) {
        throw FmuLoadException(
            "ModelDescriptionParser::parse — xmlParseMemory failed: invalid XML content");
    }

    struct DocGuard {
        xmlDocPtr d;
        explicit DocGuard(xmlDocPtr p) : d(p) {}
        ~DocGuard() { if (d) xmlFreeDoc(d); }
    } docGuard(doc);

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) {
        throw FmuLoadException(
            "ModelDescriptionParser::parse — XML document has no root element");
    }

    std::string fmiVersionStr = getProp(root, "fmiVersion");
    if (fmiVersionStr.empty()) {
        throw FmuLoadException(
            "ModelDescriptionParser::parse — root element missing 'fmiVersion' attribute");
    }

    if (fmiVersionStr.size() >= 2 && fmiVersionStr[0] == '2' && fmiVersionStr[1] == '.') {
        pkg.fmiVersion = FmiVersion::FMI2;
        parseFmi2(doc, pkg);
    } else if (fmiVersionStr.size() >= 2 && fmiVersionStr[0] == '3' && fmiVersionStr[1] == '.') {
        pkg.fmiVersion = FmiVersion::FMI3;
        parseFmi3(doc, pkg);
    } else {
        throw FmuLoadException(
            "ModelDescriptionParser::parse — unsupported fmiVersion: " + fmiVersionStr);
    }
}

void ModelDescriptionParser::parseFmi2(void* docPtr, FmuPackage& pkg) {
    xmlDocPtr  doc  = static_cast<xmlDocPtr>(docPtr);
    xmlNodePtr root = xmlDocGetRootElement(doc);

    pkg.modelName                = getProp(root, "modelName");
    pkg.guid                     = getProp(root, "guid");
    pkg.description              = getProp(root, "description");
    pkg.generationTool           = getProp(root, "generationTool");
    pkg.generationDateAndTime    = getProp(root, "generationDateAndTime");
    pkg.variableNamingConvention = getProp(root, "variableNamingConvention");

    {
        std::string nEI = getProp(root, "numberOfEventIndicators");
        if (!nEI.empty()) {
            try { pkg.numberOfEventIndicators = static_cast<uint32_t>(std::stoul(nEI)); }
            catch (...) { pkg.numberOfEventIndicators = 0; }
        }
    }

    parseUnitDefinitions(root, pkg);
    parseTypeDefinitionsFmi2(root, pkg);
    parseDefaultExperiment(root, pkg);

    for (xmlNodePtr child = root->children; child; child = child->next) {
        if (isTextOrComment(child)) continue;

        if (nodeName(child, "LogCategories")) {
            for (xmlNodePtr cat = child->children; cat; cat = cat->next) {
                if (isTextOrComment(cat)) continue;
                if (nodeName(cat, "Category")) {
                    LogCategory lc;
                    lc.name        = getProp(cat, "name");
                    lc.description = getProp(cat, "description");
                    pkg.logCategories.push_back(std::move(lc));
                }
            }
        } else if (nodeName(child, "ModelVariables")) {
            parseVariablesFmi2(child, pkg);
        } else if (nodeName(child, "CoSimulation")) {
            pkg.modelIdentifier = getProp(child, "modelIdentifier");
        }
    }

    if (pkg.modelIdentifier.empty())
        pkg.modelIdentifier = pkg.modelName;
}

void ModelDescriptionParser::parseVariablesFmi2(void* rootNode, FmuPackage& pkg) {
    xmlNodePtr mvNode = static_cast<xmlNodePtr>(rootNode);

    for (xmlNodePtr sv = mvNode->children; sv; sv = sv->next) {
        if (isTextOrComment(sv)) continue;
        if (!nodeName(sv, "ScalarVariable")) continue;

        ModelVariable var;
        var.name        = getProp(sv, "name");
        var.description = getProp(sv, "description");

        {
            std::string vr = getProp(sv, "valueReference");
            if (!vr.empty()) {
                try { var.valueReference = static_cast<uint32_t>(std::stoul(vr)); }
                catch (...) { var.valueReference = 0; }
            }
        }

        {
            std::string cs = getProp(sv, "causality");
            if (!cs.empty()) var.causality = parseCausality(cs.c_str());
        }
        {
            std::string vi = getProp(sv, "variability");
            if (!vi.empty()) var.variability = parseVariability(vi.c_str());
        }
        {
            std::string ini = getProp(sv, "initial");
            if (!ini.empty()) var.initial = parseInitial(ini.c_str());
        }

        for (xmlNodePtr typeNode = sv->children; typeNode; typeNode = typeNode->next) {
            if (isTextOrComment(typeNode)) continue;

            const char* typeName = reinterpret_cast<const char*>(typeNode->name);
            DataType dt = parseDataTypeFmi2(typeName);
            var.dataType = dt;

            // unit attribute lives on the type child element for FMI 2.0
            var.unit     = getProp(typeNode, "unit");
            var.typeName = getProp(typeNode, "declaredType");

            std::string startStr = getProp(typeNode, "start");
            if (!startStr.empty()) {
                switch (dt) {
                    case DataType::Real:
                        try { var.startValue = std::stod(startStr); } catch (...) {}
                        break;
                    case DataType::Integer:
                        try { var.startValue = static_cast<int64_t>(std::stoll(startStr)); } catch (...) {}
                        break;
                    case DataType::Boolean:
                        var.startValue = (startStr == "true" || startStr == "1");
                        break;
                    case DataType::String:
                        var.startValue = startStr;
                        break;
                    default:
                        break;
                }
            }
            break;
        }

        pkg.variables.push_back(std::move(var));
    }
}

void ModelDescriptionParser::parseFmi3(void* docPtr, FmuPackage& pkg) {
    xmlDocPtr  doc  = static_cast<xmlDocPtr>(docPtr);
    xmlNodePtr root = xmlDocGetRootElement(doc);

    pkg.modelName                = getProp(root, "modelName");
    pkg.guid                     = getProp(root, "instantiationToken");
    if (pkg.guid.empty())
        pkg.guid = getProp(root, "guid");
    pkg.description              = getProp(root, "description");
    pkg.generationTool           = getProp(root, "generationTool");
    pkg.generationDateAndTime    = getProp(root, "generationDateAndTime");
    pkg.variableNamingConvention = getProp(root, "variableNamingConvention");

    parseUnitDefinitions(root, pkg);
    parseDefaultExperiment(root, pkg);

    for (xmlNodePtr child = root->children; child; child = child->next) {
        if (isTextOrComment(child)) continue;

        if (nodeName(child, "LogCategories")) {
            for (xmlNodePtr cat = child->children; cat; cat = cat->next) {
                if (isTextOrComment(cat)) continue;
                if (nodeName(cat, "Category")) {
                    LogCategory lc;
                    lc.name        = getProp(cat, "name");
                    lc.description = getProp(cat, "description");
                    pkg.logCategories.push_back(std::move(lc));
                }
            }
        } else if (nodeName(child, "ModelVariables")) {
            parseVariablesFmi3(child, pkg);
        } else if (nodeName(child, "CoSimulation")) {
            pkg.modelIdentifier = getProp(child, "modelIdentifier");
        }
    }

    if (pkg.modelIdentifier.empty())
        pkg.modelIdentifier = pkg.modelName;
}

void ModelDescriptionParser::parseVariablesFmi3(void* rootNode, FmuPackage& pkg) {
    xmlNodePtr mvNode = static_cast<xmlNodePtr>(rootNode);

    for (xmlNodePtr vn = mvNode->children; vn; vn = vn->next) {
        if (isTextOrComment(vn)) continue;

        const char* typeName = reinterpret_cast<const char*>(vn->name);

        if (getProp(vn, "valueReference").empty()) continue;

        DataType dt = parseDataTypeFmi3(typeName);

        ModelVariable var;
        var.dataType    = dt;
        var.name        = getProp(vn, "name");
        var.description = getProp(vn, "description");

        {
            std::string vr = getProp(vn, "valueReference");
            if (!vr.empty()) {
                try { var.valueReference = static_cast<uint32_t>(std::stoul(vr)); }
                catch (...) { var.valueReference = 0; }
            }
        }

        {
            std::string cs = getProp(vn, "causality");
            if (!cs.empty()) var.causality = parseCausality(cs.c_str());
        }
        {
            std::string vi = getProp(vn, "variability");
            if (!vi.empty()) var.variability = parseVariability(vi.c_str());
        }
        {
            std::string ini = getProp(vn, "initial");
            if (!ini.empty()) var.initial = parseInitial(ini.c_str());
        }

        // unit and declaredType live as direct attributes in FMI 3.0
        var.unit     = getProp(vn, "unit");
        var.typeName = getProp(vn, "declaredType");

        std::string startStr = getProp(vn, "start");
        if (!startStr.empty()) {
            switch (dt) {
                case DataType::Float64:
                case DataType::Float32:
                    try { var.startValue = std::stod(startStr); } catch (...) {}
                    break;
                case DataType::Int8:
                case DataType::Int16:
                case DataType::Int32:
                case DataType::Int64:
                case DataType::UInt8:
                case DataType::UInt16:
                case DataType::UInt32:
                case DataType::UInt64:
                    try { var.startValue = static_cast<int64_t>(std::stoll(startStr)); } catch (...) {}
                    break;
                case DataType::Boolean:
                    var.startValue = (startStr == "true" || startStr == "1");
                    break;
                case DataType::String:
                case DataType::Binary:
                    var.startValue = startStr;
                    break;
                default:
                    break;
            }
        }

        pkg.variables.push_back(std::move(var));
    }
}

Causality ModelDescriptionParser::parseCausality(const char* str) {
    if (!str) return Causality::Local;
    if (std::strcmp(str, "parameter")           == 0) return Causality::Parameter;
    if (std::strcmp(str, "calculatedParameter") == 0) return Causality::CalculatedParameter;
    if (std::strcmp(str, "input")               == 0) return Causality::Input;
    if (std::strcmp(str, "output")              == 0) return Causality::Output;
    if (std::strcmp(str, "independent")         == 0) return Causality::Independent;
    return Causality::Local;
}

Variability ModelDescriptionParser::parseVariability(const char* str) {
    if (!str) return Variability::Continuous;
    if (std::strcmp(str, "constant")   == 0) return Variability::Constant;
    if (std::strcmp(str, "fixed")      == 0) return Variability::Fixed;
    if (std::strcmp(str, "tunable")    == 0) return Variability::Tunable;
    if (std::strcmp(str, "discrete")   == 0) return Variability::Discrete;
    if (std::strcmp(str, "continuous") == 0) return Variability::Continuous;
    return Variability::Continuous;
}

Initial ModelDescriptionParser::parseInitial(const char* str) {
    if (!str) return Initial::None;
    if (std::strcmp(str, "exact")      == 0) return Initial::Exact;
    if (std::strcmp(str, "approx")     == 0) return Initial::Approx;
    if (std::strcmp(str, "calculated") == 0) return Initial::Calculated;
    return Initial::None;
}

DataType ModelDescriptionParser::parseDataTypeFmi2(const char* typeNodeName) {
    if (!typeNodeName) return DataType::Real;
    if (std::strcmp(typeNodeName, "Real")        == 0) return DataType::Real;
    if (std::strcmp(typeNodeName, "Integer")     == 0) return DataType::Integer;
    if (std::strcmp(typeNodeName, "Boolean")     == 0) return DataType::Boolean;
    if (std::strcmp(typeNodeName, "String")      == 0) return DataType::String;
    if (std::strcmp(typeNodeName, "Enumeration") == 0) return DataType::Integer;
    return DataType::Real;
}

DataType ModelDescriptionParser::parseDataTypeFmi3(const char* typeNodeName) {
    if (!typeNodeName) return DataType::Float64;
    if (std::strcmp(typeNodeName, "Float64Variable") == 0 ||
        std::strcmp(typeNodeName, "Float64")         == 0) return DataType::Float64;
    if (std::strcmp(typeNodeName, "Float32Variable") == 0 ||
        std::strcmp(typeNodeName, "Float32")         == 0) return DataType::Float32;
    if (std::strcmp(typeNodeName, "Int8Variable")    == 0 ||
        std::strcmp(typeNodeName, "Int8")            == 0) return DataType::Int8;
    if (std::strcmp(typeNodeName, "Int16Variable")   == 0 ||
        std::strcmp(typeNodeName, "Int16")           == 0) return DataType::Int16;
    if (std::strcmp(typeNodeName, "Int32Variable")   == 0 ||
        std::strcmp(typeNodeName, "Int32")           == 0) return DataType::Int32;
    if (std::strcmp(typeNodeName, "Int64Variable")   == 0 ||
        std::strcmp(typeNodeName, "Int64")           == 0) return DataType::Int64;
    if (std::strcmp(typeNodeName, "UInt8Variable")   == 0 ||
        std::strcmp(typeNodeName, "UInt8")           == 0) return DataType::UInt8;
    if (std::strcmp(typeNodeName, "UInt16Variable")  == 0 ||
        std::strcmp(typeNodeName, "UInt16")          == 0) return DataType::UInt16;
    if (std::strcmp(typeNodeName, "UInt32Variable")  == 0 ||
        std::strcmp(typeNodeName, "UInt32")          == 0) return DataType::UInt32;
    if (std::strcmp(typeNodeName, "UInt64Variable")  == 0 ||
        std::strcmp(typeNodeName, "UInt64")          == 0) return DataType::UInt64;
    if (std::strcmp(typeNodeName, "BooleanVariable") == 0 ||
        std::strcmp(typeNodeName, "Boolean")         == 0) return DataType::Boolean;
    if (std::strcmp(typeNodeName, "StringVariable")  == 0 ||
        std::strcmp(typeNodeName, "String")          == 0) return DataType::String;
    if (std::strcmp(typeNodeName, "BinaryVariable")  == 0 ||
        std::strcmp(typeNodeName, "Binary")          == 0) return DataType::Binary;
    if (std::strcmp(typeNodeName, "ClockVariable")   == 0 ||
        std::strcmp(typeNodeName, "Clock")           == 0) return DataType::Clock;
    return DataType::Float64;
}

// =============================================================================
// UnitDefinitions parser (FMI 2.0 and 3.0 share the same element structure)
// =============================================================================

void ModelDescriptionParser::parseUnitDefinitions(void* rootNodePtr, FmuPackage& pkg) {
    xmlNodePtr root = static_cast<xmlNodePtr>(rootNodePtr);
    for (xmlNodePtr child = root->children; child; child = child->next) {
        if (isTextOrComment(child)) continue;
        if (!nodeName(child, "UnitDefinitions")) continue;

        for (xmlNodePtr unitNode = child->children; unitNode; unitNode = unitNode->next) {
            if (isTextOrComment(unitNode)) continue;
            if (!nodeName(unitNode, "Unit")) continue;

            UnitDef ud;
            ud.name = getProp(unitNode, "name");

            // FMI 2.0/3.0 BaseUnit child: factor and offset
            for (xmlNodePtr bu = unitNode->children; bu; bu = bu->next) {
                if (isTextOrComment(bu)) continue;
                if (nodeName(bu, "BaseUnit")) {
                    std::string factorStr = getProp(bu, "factor");
                    std::string offsetStr = getProp(bu, "offset");
                    if (!factorStr.empty()) ud.factor = parseDouble(factorStr.c_str(), 1.0);
                    if (!offsetStr.empty()) ud.offset = parseDouble(offsetStr.c_str(), 0.0);
                }
            }

            if (!ud.name.empty())
                pkg.unitDefinitions.push_back(std::move(ud));
        }
        break;
    }
}

// =============================================================================
// TypeDefinitions parser (FMI 2.0 only: <TypeDefinitions> / <SimpleType>)
// =============================================================================

void ModelDescriptionParser::parseTypeDefinitionsFmi2(void* rootNodePtr, FmuPackage& pkg) {
    xmlNodePtr root = static_cast<xmlNodePtr>(rootNodePtr);
    for (xmlNodePtr child = root->children; child; child = child->next) {
        if (isTextOrComment(child)) continue;
        if (!nodeName(child, "TypeDefinitions")) continue;

        for (xmlNodePtr stNode = child->children; stNode; stNode = stNode->next) {
            if (isTextOrComment(stNode)) continue;
            if (!nodeName(stNode, "SimpleType")) continue;

            TypeDef td;
            td.name        = getProp(stNode, "name");
            td.description = getProp(stNode, "description");

            // Type kind is determined by the first child element
            for (xmlNodePtr typeKind = stNode->children; typeKind; typeKind = typeKind->next) {
                if (isTextOrComment(typeKind)) continue;
                const char* kn = reinterpret_cast<const char*>(typeKind->name);
                td.dataType     = parseDataTypeFmi2(kn);
                td.unit         = getProp(typeKind, "unit");
                td.quantity     = getProp(typeKind, "quantity");
                td.displayUnit  = getProp(typeKind, "displayUnit");
                td.minStr       = getProp(typeKind, "min");
                td.maxStr       = getProp(typeKind, "max");
                break;
            }

            if (!td.name.empty())
                pkg.typeDefinitions.push_back(std::move(td));
        }
        break;
    }
}

// =============================================================================
// DefaultExperiment parser (FMI 2.0 and 3.0)
// =============================================================================

void ModelDescriptionParser::parseDefaultExperiment(void* rootNodePtr, FmuPackage& pkg) {
    xmlNodePtr root = static_cast<xmlNodePtr>(rootNodePtr);
    for (xmlNodePtr child = root->children; child; child = child->next) {
        if (isTextOrComment(child)) continue;
        if (!nodeName(child, "DefaultExperiment")) continue;

        std::string startTimeStr = getProp(child, "startTime");
        std::string stopTimeStr  = getProp(child, "stopTime");
        std::string stepSizeStr  = getProp(child, "stepSize");
        std::string tolStr       = getProp(child, "tolerance");

        if (!startTimeStr.empty()) {
            pkg.defaultExperiment.startTime    = parseDouble(startTimeStr.c_str(), 0.0);
            pkg.defaultExperiment.hasStartTime = true;
        }
        if (!stopTimeStr.empty()) {
            pkg.defaultExperiment.stopTime    = parseDouble(stopTimeStr.c_str(), 1.0);
            pkg.defaultExperiment.hasStopTime = true;
        }
        if (!stepSizeStr.empty()) {
            pkg.defaultExperiment.stepSize    = parseDouble(stepSizeStr.c_str(), 0.0);
            pkg.defaultExperiment.hasStepSize = true;
        }
        if (!tolStr.empty()) {
            pkg.defaultExperiment.tolerance    = parseDouble(tolStr.c_str(), 0.0);
            pkg.defaultExperiment.hasTolerance = true;
        }
        break;
    }
}

} // namespace libfmu_toolkit
