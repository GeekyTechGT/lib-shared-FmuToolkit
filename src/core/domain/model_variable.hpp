#pragma once

#include "fmu_variable_types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace libfmu_toolkit {

using StartValue = std::variant<
    double,
    int64_t,
    bool,
    std::string
>;

struct ModelVariable {
    std::string  name;
    std::string  description;
    uint32_t     valueReference{0};

    Causality    causality{Causality::Local};
    Variability  variability{Variability::Continuous};
    Initial      initial{Initial::None};
    DataType     dataType{DataType::Real};

    std::string  unit;       // physical unit name (e.g. "m/s", "rad")
    std::string  typeName;   // reference to a TypeDefinition (FMI 2.0 declaredType)

    std::optional<StartValue> startValue;

    [[nodiscard]] bool operator==(const ModelVariable& other) const noexcept {
        return valueReference == other.valueReference && name == other.name;
    }

    [[nodiscard]] bool operator!=(const ModelVariable& other) const noexcept {
        return !(*this == other);
    }
};

} // namespace libfmu_toolkit
