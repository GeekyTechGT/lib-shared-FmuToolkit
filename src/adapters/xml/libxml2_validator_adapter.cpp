/// @file libxml2_validator_adapter.cpp
/// @brief Implementation of XSD schema validation via libxml2 (FR-03.5).
/// @internal

#include "libxml2_validator_adapter.hpp"

#include <libxml/parser.h>
#include <libxml/xmlschemas.h>
#include <libxml/xmlerror.h>

#include <memory>
#include <string>
#include <type_traits>

// ── libxml2 API portability ──────────────────────────────────────────────────
// The const-ness of the xmlError pointer in xmlStructuredErrorFunc differs
// across libxml2 versions and distro patches:
//   Ubuntu 24.04 / libxml2 2.9.x: void(*)(void*, xmlError*)        — no const
//   MinGW64       / libxml2 2.15.x: void(*)(void*, const xmlError*) — with const
//
// We strip the pointer-to-function layer and extract the second parameter type
// at compile time so the callbacks always match the platform's exact signature.
namespace {
    // Primary template — catches any function-pointer shape.
    template<typename F>
    struct XmlStructuredErrTraits;

    // Specialisation: non-const error pointer (e.g. libxml2 2.9.x)
    template<typename R, typename A0, typename A1>
    struct XmlStructuredErrTraits<R(*)(A0, A1)> {
        using ErrorPtr = A1;  // whatever the second param actually is
    };

    using XmlStructuredErrorPtr =
        typename XmlStructuredErrTraits<xmlStructuredErrorFunc>::ErrorPtr;
} // namespace

namespace libfmu_toolkit {

struct XmlDocDeleter {
    void operator()(xmlDocPtr doc) const noexcept { if (doc) xmlFreeDoc(doc); }
};
using XmlDocPtr = std::unique_ptr<xmlDoc, XmlDocDeleter>;

struct XmlSchemaParserCtxtDeleter {
    void operator()(xmlSchemaParserCtxtPtr ctx) const noexcept {
        if (ctx) xmlSchemaFreeParserCtxt(ctx);
    }
};
using XmlSchemaParserCtxtPtr = std::unique_ptr<xmlSchemaParserCtxt, XmlSchemaParserCtxtDeleter>;

struct XmlSchemaDeleter {
    void operator()(xmlSchemaPtr schema) const noexcept { if (schema) xmlSchemaFree(schema); }
};
using XmlSchemaObjPtr = std::unique_ptr<xmlSchema, XmlSchemaDeleter>;

struct XmlSchemaValidCtxtDeleter {
    void operator()(xmlSchemaValidCtxtPtr ctx) const noexcept {
        if (ctx) xmlSchemaFreeValidCtxt(ctx);
    }
};
using XmlSchemaValidCtxtPtr = std::unique_ptr<xmlSchemaValidCtxt, XmlSchemaValidCtxtDeleter>;

static void schemaParserErrorCallback(void* userData, XmlStructuredErrorPtr error)
{
    auto* report = static_cast<ValidationReport*>(userData);
    if (error && error->message) {
        std::string msg(error->message);
        if (!msg.empty() && msg.back() == '\n') msg.pop_back();
        report->addError(ValidationCategory::Schema, std::move(msg), error->line, 0);
    }
}

static void schemaValidErrorCallback(void* userData, XmlStructuredErrorPtr error)
{
    auto* report = static_cast<ValidationReport*>(userData);
    if (error && error->message) {
        std::string msg(error->message);
        if (!msg.empty() && msg.back() == '\n') msg.pop_back();
        report->addError(ValidationCategory::Schema, std::move(msg), error->line, 0);
    }
}

ValidationReport Libxml2ValidatorAdapter::validate(
    const std::string& xmlContent,
    const std::filesystem::path& xsdPath) const
{
    ValidationReport report;

    if (xmlContent.empty()) {
        report.addError(ValidationCategory::Schema, "XML content is empty");
        return report;
    }

    const int xmlSize = static_cast<int>(xmlContent.size());
    XmlDocPtr doc(xmlParseMemory(xmlContent.c_str(), xmlSize));
    if (!doc) {
        report.addError(ValidationCategory::Schema, "Failed to parse XML content");
        return report;
    }

    const std::string xsdPathStr = xsdPath.string();

    XmlSchemaParserCtxtPtr parserCtxt(xmlSchemaNewParserCtxt(xsdPathStr.c_str()));
    if (!parserCtxt) {
        report.addError(ValidationCategory::Schema,
                        "Failed to create XSD parser context for: " + xsdPathStr);
        return report;
    }

    xmlSchemaSetParserStructuredErrors(parserCtxt.get(), schemaParserErrorCallback, &report);

    XmlSchemaObjPtr schema(xmlSchemaParse(parserCtxt.get()));
    if (!schema || report.hasErrors()) {
        if (!report.hasErrors()) {
            report.addError(ValidationCategory::Schema,
                            "Failed to parse XSD schema: " + xsdPathStr);
        }
        return report;
    }

    XmlSchemaValidCtxtPtr validCtxt(xmlSchemaNewValidCtxt(schema.get()));
    if (!validCtxt) {
        report.addError(ValidationCategory::Schema,
                        "Failed to create schema validation context");
        return report;
    }

    xmlSchemaSetValidStructuredErrors(validCtxt.get(), schemaValidErrorCallback, &report);

    const int ret = xmlSchemaValidateDoc(validCtxt.get(), doc.get());

    if (ret < 0) {
        report.addError(ValidationCategory::Schema,
                        "Internal libxml2 error during schema validation");
    } else if (ret > 0 && !report.hasErrors()) {
        report.addError(ValidationCategory::Schema,
                        "XML document failed schema validation (no details available)");
    }

    report.passed = (ret == 0 && !report.hasErrors());
    return report;
}

} // namespace libfmu_toolkit
