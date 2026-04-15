#include <gtest/gtest.h>

#include "core/domain/simulation_result.hpp"
#include "core/usecases/csv_result_exporter.hpp"
#include "adapters/json/json_result_exporter.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using namespace libfmu_toolkit;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

SimulationResult makeResult() {
    SimulationResult r;
    r.timePoints = {0.0, 0.1, 0.2};
    r.series["y"]  = {OutputValue{1.0},  OutputValue{0.9},  OutputValue{0.8}};
    r.series["ok"] = {OutputValue{true}, OutputValue{false}, OutputValue{true}};
    r.series["n"]  = {OutputValue{int64_t{0}}, OutputValue{int64_t{1}}, OutputValue{int64_t{2}}};
    return r;
}

std::string readFile(const std::filesystem::path& path) {
    std::ifstream ifs(path);
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

struct TmpFile {
    std::filesystem::path path;
    explicit TmpFile(const std::string& suffix) {
        path = std::filesystem::temp_directory_path()
               / ("lib_fmu_toolkit_test_" + suffix);
    }
    ~TmpFile() { std::filesystem::remove(path); }
};

} // anonymous namespace

// ===========================================================================
// CSV exporter tests
// ===========================================================================

TEST(CsvExporterTest, EmptyResultWritesHeaderOnly) {
    TmpFile tmp("empty.csv");
    SimulationResult empty;
    ASSERT_NO_THROW(CsvResultExporter::exportToCsv(empty, tmp.path));

    std::string content = readFile(tmp.path);
    EXPECT_EQ(content, "time\n");
}

TEST(CsvExporterTest, HeaderContainsTimeAndVariableNames) {
    TmpFile tmp("header.csv");
    CsvResultExporter::exportToCsv(makeResult(), tmp.path);

    std::string content = readFile(tmp.path);
    EXPECT_NE(content.find("time"), std::string::npos);
    EXPECT_NE(content.find("y"), std::string::npos);
    EXPECT_NE(content.find("ok"), std::string::npos);
    EXPECT_NE(content.find("n"), std::string::npos);
}

TEST(CsvExporterTest, CorrectNumberOfDataRows) {
    TmpFile tmp("rows.csv");
    CsvResultExporter::exportToCsv(makeResult(), tmp.path);

    std::string content = readFile(tmp.path);
    // Count newlines: 1 header + 3 data rows = 4 newlines
    auto newlines = static_cast<size_t>(std::count(content.begin(), content.end(), '\n'));
    EXPECT_EQ(newlines, 4u);
}

TEST(CsvExporterTest, FirstDataRowStartsWithZero) {
    TmpFile tmp("firstrow.csv");
    CsvResultExporter::exportToCsv(makeResult(), tmp.path);

    std::string content = readFile(tmp.path);
    std::istringstream iss(content);
    std::string line;
    std::getline(iss, line); // header
    std::getline(iss, line); // first data row
    EXPECT_EQ(line.substr(0, 1), "0");
}

TEST(CsvExporterTest, BooleanValuesWrittenAs0And1) {
    TmpFile tmp("bool.csv");
    CsvResultExporter::exportToCsv(makeResult(), tmp.path);
    std::string content = readFile(tmp.path);
    EXPECT_NE(content.find(",1"), std::string::npos);
    EXPECT_NE(content.find(",0"), std::string::npos);
}

TEST(CsvExporterTest, StringValuesAreQuoted) {
    TmpFile tmp("string.csv");
    SimulationResult r;
    r.timePoints = {0.0};
    r.series["s"] = {OutputValue{std::string{"hello"}}};
    CsvResultExporter::exportToCsv(r, tmp.path);
    std::string content = readFile(tmp.path);
    EXPECT_NE(content.find("\"hello\""), std::string::npos);
}

TEST(CsvExporterTest, ThrowsOnUnwritablePath) {
    SimulationResult r;
    EXPECT_THROW(
        CsvResultExporter::exportToCsv(r, "/nonexistent_dir/output.csv"),
        std::ios_base::failure);
}

// ===========================================================================
// JSON exporter tests
// ===========================================================================

TEST(JsonExporterTest, EmptyResultWritesValidJson) {
    TmpFile tmp("empty.json");
    SimulationResult empty;
    ASSERT_NO_THROW(JsonResultExporter::exportToJson(empty, tmp.path));

    std::string content = readFile(tmp.path);
    EXPECT_NE(content.find("timePoints"), std::string::npos);
    EXPECT_NE(content.find("series"), std::string::npos);
    // Should be valid JSON structure
    EXPECT_NE(content.find("{"), std::string::npos);
    EXPECT_NE(content.find("}"), std::string::npos);
}

TEST(JsonExporterTest, ContainsAllVariableNames) {
    TmpFile tmp("vars.json");
    JsonResultExporter::exportToJson(makeResult(), tmp.path);
    std::string content = readFile(tmp.path);
    EXPECT_NE(content.find("\"y\""), std::string::npos);
    EXPECT_NE(content.find("\"ok\""), std::string::npos);
    EXPECT_NE(content.find("\"n\""), std::string::npos);
}

TEST(JsonExporterTest, TimePointsArrayHasCorrectCount) {
    TmpFile tmp("tp.json");
    auto result = makeResult();
    JsonResultExporter::exportToJson(result, tmp.path);
    std::string content = readFile(tmp.path);

    // timePoints: [0, 0.1, 0.2] — 2 commas between 3 elements
    size_t tpPos = content.find("timePoints");
    EXPECT_NE(tpPos, std::string::npos);
    size_t arrStart = content.find('[', tpPos);
    size_t arrEnd   = content.find(']', arrStart);
    EXPECT_NE(arrStart, std::string::npos);
    EXPECT_NE(arrEnd, std::string::npos);
    std::string tpArray = content.substr(arrStart, arrEnd - arrStart + 1);
    // 3 elements → 2 commas
    auto commas = static_cast<size_t>(std::count(tpArray.begin(), tpArray.end(), ','));
    EXPECT_EQ(commas, 2u);
}

TEST(JsonExporterTest, BooleanValuesWrittenAsTrueOrFalse) {
    TmpFile tmp("boolj.json");
    auto result = makeResult();
    JsonResultExporter::exportToJson(result, tmp.path);
    std::string content = readFile(tmp.path);
    EXPECT_NE(content.find("true"), std::string::npos);
    EXPECT_NE(content.find("false"), std::string::npos);
}

TEST(JsonExporterTest, StringValuesAreJsonEscaped) {
    TmpFile tmp("strj.json");
    SimulationResult r;
    r.timePoints = {0.0};
    r.series["msg"] = {OutputValue{std::string{"say \"hello\""}}};
    JsonResultExporter::exportToJson(r, tmp.path);
    std::string content = readFile(tmp.path);
    // Quotes inside string should be escaped as \"
    EXPECT_NE(content.find("\\\"hello\\\""), std::string::npos);
}

TEST(JsonExporterTest, ThrowsOnUnwritablePath) {
    SimulationResult r;
    EXPECT_THROW(
        JsonResultExporter::exportToJson(r, "/nonexistent_dir/output.json"),
        std::ios_base::failure);
}

TEST(JsonExporterTest, ViaInterfacePolymorphism) {
    TmpFile tmp("poly.json");
    JsonResultExporter exporter;
    SimulationResult r;
    r.timePoints = {0.0, 1.0};
    r.series["v"] = {OutputValue{42.0}, OutputValue{43.0}};
    ASSERT_NO_THROW(exporter.exportResult(r, tmp.path));
    std::string content = readFile(tmp.path);
    EXPECT_NE(content.find("42"), std::string::npos);
}
