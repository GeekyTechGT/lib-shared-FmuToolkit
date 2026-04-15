#include "csv_result_exporter.hpp"

#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>

namespace libfmu_toolkit {

namespace {

std::string outputValueToString(const OutputValue& v) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, double>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? "1" : "0";
        } else if constexpr (std::is_same_v<T, std::string>) {
            std::string escaped;
            escaped.reserve(arg.size() + 2);
            escaped += '"';
            for (char c : arg) {
                if (c == '"') escaped += '"';
                escaped += c;
            }
            escaped += '"';
            return escaped;
        }
    }, v);
}

} // anonymous namespace

void CsvResultExporter::exportResult(const SimulationResult& result,
                                      const std::filesystem::path& outputPath)
{
    exportToCsv(result, outputPath);
}

void CsvResultExporter::exportToCsv(const SimulationResult& result,
                                      const std::filesystem::path& outputPath)
{
    std::ofstream ofs(outputPath);
    if (!ofs.is_open()) {
        throw std::ios_base::failure("CsvResultExporter: cannot open output file: " +
                                      outputPath.string());
    }
    ofs.exceptions(std::ofstream::badbit | std::ofstream::failbit);

    if (result.empty()) {
        ofs << "time\n";
        return;
    }

    std::vector<std::string> varNames;
    varNames.reserve(result.series.size());
    for (const auto& [name, _] : result.series)
        varNames.push_back(name);

    ofs << "time";
    for (const auto& name : varNames)
        ofs << ',' << name;
    ofs << '\n';

    const size_t nSteps = result.timePoints.size();
    for (size_t i = 0; i < nSteps; ++i) {
        ofs << result.timePoints[i];
        for (const auto& name : varNames) {
            auto it = result.series.find(name);
            if (it != result.series.end() && i < it->second.size()) {
                ofs << ',' << outputValueToString(it->second[i]);
            } else {
                ofs << ',';
            }
        }
        ofs << '\n';
    }
}

} // namespace libfmu_toolkit
