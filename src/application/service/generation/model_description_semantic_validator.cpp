/// @file model_description_semantic_validator.cpp
/// @brief Implementation of semantic validation for generated models (FR-08).
/// @internal

#include "fmu_model_description.hpp"

#include "domain/model/fmu_variable_types.hpp"
#include "domain/exception/fmu_exception.hpp"

#include <map>
#include <set>
#include <string>

namespace libfmu_toolkit {

void ModelDescriptionSemanticValidator::validateOrThrow(const FmuPackage& pkg) {
    std::map<DataType, std::set<uint32_t>> seenVrByType;
    for (const auto& var : pkg.variables) {
        auto& seenSet = seenVrByType[var.dataType];
        if (!seenSet.insert(var.valueReference).second) {
            throw FmuLoadException(
                "Generated modelDescription.xml is semantically invalid: Duplicate valueReference "
                + std::to_string(var.valueReference) + " for type " + std::string(to_string(var.dataType)));
        }
    }

    for (const auto& var : pkg.variables) {
        const Causality c = var.causality;
        const Variability v = var.variability;

        if (c == Causality::Input && v == Variability::Constant) {
            throw FmuLoadException(
                "Generated modelDescription.xml is semantically invalid: variable '" + var.name
                + "' uses causality=input with variability=constant");
        }
        if (c == Causality::Output && (v == Variability::Fixed || v == Variability::Tunable)) {
            throw FmuLoadException(
                "Generated modelDescription.xml is semantically invalid: variable '" + var.name
                + "' uses causality=output with variability=" + std::string(to_string(v)));
        }
        if (c == Causality::Parameter && (v == Variability::Discrete || v == Variability::Continuous)) {
            throw FmuLoadException(
                "Generated modelDescription.xml is semantically invalid: variable '" + var.name
                + "' uses causality=parameter with variability=" + std::string(to_string(v)));
        }
    }
}

} // namespace libfmu_toolkit
