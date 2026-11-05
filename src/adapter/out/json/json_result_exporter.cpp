/// @file json_result_exporter.cpp
/// @brief Implementation of hand-written JSON result export (FR-06.2, FR-06.3).
/// @internal

#include "json_result_exporter.hpp"

#include <fstream>
#include <ios>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace libfmu_toolkit {

namespace {

std::string outputValueToJson(const OutputValue& v) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, double>) {
            std::ostringstream oss;
            oss << std::setprecision(17) << arg;
            return oss.str();
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::string>) {
            // Escape special JSON characters
            std::string out;
            out.reserve(arg.size() + 2);
            out += '"';
            for (char c : arg) {
                switch (c) {
                    case '"':  out += "\\\""; break;
                    case '\\': out += "\\\\"; break;
                    case '\n': out += "\\n";  break;
                    case '\r': out += "\\r";  break;
                    case '\t': out += "\\t";  break;
                    default:   out += c;      break;
                }
            }
            out += '"';
            return out;
        }
    }, v);
}

} // anonymous namespace

void JsonResultExporter::exportResult(const SimulationResult& result,
                                       const std::filesystem::path& outputPath) {
    exportToJson(result, outputPath);
}

void JsonResultExporter::exportToJson(const SimulationResult& result,
                                       const std::filesystem::path& outputPath) {
    std::ofstream ofs(outputPath);
    if (!ofs.is_open()) {
        throw std::ios_base::failure(
            "JsonResultExporter: cannot open output file: " + outputPath.string());
    }
    ofs.exceptions(std::ofstream::badbit | std::ofstream::failbit);

    ofs << "{\n";

    // timePoints array
    ofs << "  \"timePoints\": [";
    for (size_t i = 0; i < result.timePoints.size(); ++i) {
        if (i > 0) ofs << ", ";
        ofs << std::setprecision(17) << result.timePoints[i];
    }
    ofs << "]";

    // series object
    if (!result.series.empty()) {
        ofs << ",\n  \"series\": {\n";

        bool firstSeries = true;
        for (const auto& [varName, values] : result.series) {
            if (!firstSeries) ofs << ",\n";
            firstSeries = false;

            // JSON-escape the variable name
            ofs << "    \"";
            for (char c : varName) {
                if (c == '"' || c == '\\') ofs << '\\';
                ofs << c;
            }
            ofs << "\": [";

            for (size_t i = 0; i < values.size(); ++i) {
                if (i > 0) ofs << ", ";
                ofs << outputValueToJson(values[i]);
            }
            ofs << "]";
        }

        ofs << "\n  }";
    } else {
        ofs << ",\n  \"series\": {}";
    }

    ofs << "\n}\n";
}

} // namespace libfmu_toolkit
