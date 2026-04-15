#pragma once

#include <string>

namespace libfmu_toolkit {

struct LogCategory {
    std::string name;
    std::string description;

    [[nodiscard]] bool operator==(const LogCategory& other) const noexcept {
        return name == other.name;
    }

    [[nodiscard]] bool operator!=(const LogCategory& other) const noexcept {
        return !(*this == other);
    }
};

} // namespace libfmu_toolkit
