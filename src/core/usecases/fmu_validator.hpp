#pragma once

#include "core/domain/fmu_package.hpp"
#include "core/domain/validation_report.hpp"
#include "ports/i_xml_validator.hpp"

#include <filesystem>

namespace libfmu_toolkit {

/// @brief Three-phase FMU validator.
///
/// Validates a loaded FmuPackage against the FMI 2.0 and FMI 3.0 co-simulation
/// standards. Validation proceeds in three independent phases so that a single
/// call to validate() returns the complete diagnostic picture:
///
/// **Phase 1 — Structure**
///   Verifies the mandatory archive layout defined by the FMI standard:
///   - The extracted root directory exists.
///   - `modelDescription.xml` is present at the archive root.
///   - A `binaries/` directory is present.
///   - The `binaries/` directory contains at least one platform-specific
///     subdirectory that uses the correct naming convention for the FMI version
///     (FMI 2.0: win64, linux64, darwin64, …;
///      FMI 3.0: x86_64-windows, x86_64-linux, x86_64-darwin, …).
///   - The shared library filename matches the `modelIdentifier` attribute from
///     `modelDescription.xml`, as required by both standards.
///
/// **Phase 2 — Schema**
///   Validates the raw `modelDescription.xml` content against the appropriate
///   FMI XSD schema (fmi2ModelDescription.xsd or fmi3ModelDescription.xsd) via
///   an injected `IXmlValidator`. Line- and column-accurate errors are preserved.
///
/// **Phase 3 — Semantics**
///   Checks invariants beyond what XSD covers:
///   - No duplicate `valueReference` values within the same data type.
///   - Invalid `causality`/`variability` combinations (e.g. input + constant,
///     output + fixed, parameter + continuous).
///   - At least one output variable exists (warning only).
///
/// All three phases run unconditionally so that callers receive a complete set
/// of diagnostics in a single pass.
class FmuValidator {
public:
    /// @param xmlValidator  Injected XML/XSD validator (e.g. Libxml2ValidatorAdapter).
    /// @param fmi2SchemaDir Directory containing `fmi2ModelDescription.xsd`.
    /// @param fmi3SchemaDir Directory containing `fmi3ModelDescription.xsd`.
    explicit FmuValidator(IXmlValidator&        xmlValidator,
                          std::filesystem::path fmi2SchemaDir,
                          std::filesystem::path fmi3SchemaDir);

    /// @brief Run all three validation phases on @p pkg.
    ///
    /// Never throws. The returned report's `passed` flag is `true` if and only
    /// if no errors were found across all three phases.
    [[nodiscard]] ValidationReport validate(const FmuPackage& pkg) const;

private:
    IXmlValidator&        xmlValidator_;
    std::filesystem::path fmi2SchemaDir_;
    std::filesystem::path fmi3SchemaDir_;

    /// Phase 1: full structural check of the FMU archive layout.
    void checkStructure(const FmuPackage& pkg, ValidationReport& report) const;

    /// Phase 1 helper: verify that binary directory names follow the naming
    /// convention mandated by the FMI version (FMI 2.0 vs FMI 3.0 differ).
    void checkBinaryNamingConvention(const FmuPackage& pkg, ValidationReport& report) const;

    /// Phase 1 helper: verify that the shared library filename matches the
    /// `modelIdentifier` declared in `modelDescription.xml`.
    void checkLibraryIdentifier(const FmuPackage& pkg, ValidationReport& report) const;

    /// Phase 2: XSD schema validation via the injected validator.
    void checkSchema(const FmuPackage& pkg, ValidationReport& report) const;

    /// Phase 3: semantic consistency checks on parsed model variables.
    void checkSemantics(const FmuPackage& pkg, ValidationReport& report) const;
};

} // namespace libfmu_toolkit
