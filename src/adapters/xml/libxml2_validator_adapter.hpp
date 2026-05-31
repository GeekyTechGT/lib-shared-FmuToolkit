#pragma once

/// @file libxml2_validator_adapter.hpp
/// @brief libxml2-backed adapter implementing the IXmlValidator port.
/// @internal

#include "ports/i_xml_validator.hpp"

namespace libfmu_toolkit {

/// @brief Validates `modelDescription.xml` against an FMI XSD schema using libxml2.
///
/// Concrete IXmlValidator implementation. XSD errors are reported with accurate
/// line and column numbers as sourced from libxml2's structured error callback.
///
/// @par Requirements
/// Implements FR-03.5 (XSD schema validation with line/column diagnostics).
/// @req FR-03.5
class Libxml2ValidatorAdapter final : public IXmlValidator {
public:
    Libxml2ValidatorAdapter() = default;
    ~Libxml2ValidatorAdapter() override = default;

    /// @brief Validates @p xmlContent against the schema at @p xsdPath (IXmlValidator port).
    /// @param xmlContent Raw `modelDescription.xml` text.
    /// @param xsdPath    Path to the FMI 2.0 or 3.0 XSD schema file.
    /// @returns A report containing any schema errors (with line/column).
    /// @req FR-03.5
    [[nodiscard]] ValidationReport validate(
        const std::string& xmlContent,
        const std::filesystem::path& xsdPath) const override;
};

} // namespace libfmu_toolkit
