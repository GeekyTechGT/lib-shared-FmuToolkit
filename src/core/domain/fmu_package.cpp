#include "fmu_package.hpp"

namespace libfmu_toolkit {

std::vector<const ModelVariable*> FmuPackage::inputs() const {
    std::vector<const ModelVariable*> result;
    result.reserve(variables.size());
    for (const auto& var : variables) {
        if (var.causality == Causality::Input)
            result.push_back(&var);
    }
    return result;
}

std::vector<const ModelVariable*> FmuPackage::outputs() const {
    std::vector<const ModelVariable*> result;
    result.reserve(variables.size());
    for (const auto& var : variables) {
        if (var.causality == Causality::Output)
            result.push_back(&var);
    }
    return result;
}

std::vector<const ModelVariable*> FmuPackage::parameters() const {
    std::vector<const ModelVariable*> result;
    result.reserve(variables.size());
    for (const auto& var : variables) {
        if (var.causality == Causality::Parameter)
            result.push_back(&var);
    }
    return result;
}

const ModelVariable* FmuPackage::findVariable(const std::string& name) const {
    for (const auto& var : variables) {
        if (var.name == name)
            return &var;
    }
    return nullptr;
}

} // namespace libfmu_toolkit
