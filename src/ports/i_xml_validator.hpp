#pragma once
#include "core/domain/validation_report.hpp"
#include <filesystem>
#include <string>

namespace libfmu_toolkit {

/// @brief Port interface for XML schema validation.
///
/// Implemented by Libxml2ValidatorAdapter. Inject an alternative in tests by
/// passing a custom implementation to FmuEngine::Impl's injection constructor.
class IXmlValidator {
public:
    virtual ~IXmlValidator() = default;

    /// @brief Validates XML content against an XSD schema file.
    ///
    /// @param xmlContent  Full XML document as a UTF-8 string.
    /// @param xsdPath     Path to the XSD schema file.
    ///
    /// @returns ValidationReport with `passed` flag, errors, and warnings.
    [[nodiscard]] virtual ValidationReport validate(
        const std::string& xmlContent,
        const std::filesystem::path& xsdPath) const = 0;

protected:
    IXmlValidator() = default;
};

} // namespace libfmu_toolkit
