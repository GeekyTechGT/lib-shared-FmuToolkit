#pragma once
#include "ports/i_xml_validator.hpp"

namespace libfmu_toolkit {

class Libxml2ValidatorAdapter final : public IXmlValidator {
public:
    Libxml2ValidatorAdapter() = default;
    ~Libxml2ValidatorAdapter() override = default;

    [[nodiscard]] ValidationReport validate(
        const std::string& xmlContent,
        const std::filesystem::path& xsdPath) const override;
};

} // namespace libfmu_toolkit
