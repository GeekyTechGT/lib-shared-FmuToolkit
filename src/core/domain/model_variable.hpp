#pragma once

/// @file model_variable.hpp
/// @brief Internal domain entity describing a single FMU model variable.
/// @internal

#include "fmu_variable_types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace libfmu_toolkit {

/// @brief Typed start value of a model variable.
///
/// Holds one of the four fundamental FMI scalar types. The active alternative
/// is chosen by the parser according to the variable's DataType.
/// @req FR-02.2
using StartValue = std::variant<
    double,
    int64_t,
    bool,
    std::string
>;

/// @brief Domain representation of one `<ScalarVariable>` / `<ModelVariables>` entry.
///
/// This is the internal counterpart of the public VariableInfo. It is populated
/// from `modelDescription.xml` during import and mapped to VariableInfo when
/// metadata crosses the public API boundary.
/// @req FR-02.2
struct ModelVariable {
    std::string  name;             ///< Unique variable name within the FMU.
    std::string  description;      ///< Human-readable description (may be empty).
    uint32_t     valueReference{0}; ///< FMI value reference used in get/set calls.

    Causality    causality{Causality::Local};        ///< Declared causality.
    Variability  variability{Variability::Continuous}; ///< Declared variability.
    Initial      initial{Initial::None};             ///< Declared initial kind.
    DataType     dataType{DataType::Real};            ///< Declared data type.

    std::string  unit;       ///< Physical unit name (e.g. `"m/s"`, `"rad"`; may be empty).
    std::string  typeName;   ///< Referenced TypeDefinition name (FMI 2.0 `declaredType`).

    std::optional<StartValue> startValue; ///< Start value if one was declared in the XML.

    /// @brief Equality by identity: same value reference and same name.
    /// @req FR-02.2
    [[nodiscard]] bool operator==(const ModelVariable& other) const noexcept {
        return valueReference == other.valueReference && name == other.name;
    }

    /// @brief Negation of operator==.
    /// @req FR-02.2
    [[nodiscard]] bool operator!=(const ModelVariable& other) const noexcept {
        return !(*this == other);
    }
};

} // namespace libfmu_toolkit
