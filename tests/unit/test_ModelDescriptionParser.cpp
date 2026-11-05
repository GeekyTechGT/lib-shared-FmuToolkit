#include <gtest/gtest.h>

// Parser lives in src/ — include via private include path exposed to tests
#include "adapter/out/xml/model_description_parser.hpp"
#include "domain/model/fmu_package.hpp"
#include "domain/exception/fmu_exception.hpp"

using namespace libfmu_toolkit;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

// Minimal FMI 2.0 modelDescription with common elements
constexpr const char* kFmi2Minimal = R"(<?xml version="1.0" encoding="UTF-8"?>
<fmiModelDescription
    fmiVersion="2.0"
    modelName="TestModel"
    guid="{abc-123}"
    description="A test model"
    generationTool="MyTool"
    generationDateAndTime="2024-01-01T00:00:00Z"
    variableNamingConvention="flat"
    numberOfEventIndicators="2">

  <CoSimulation modelIdentifier="TestModel"/>

  <UnitDefinitions>
    <Unit name="m/s">
      <BaseUnit factor="1.0" offset="0.0"/>
    </Unit>
    <Unit name="deg">
      <BaseUnit factor="0.017453292519943295" offset="0.0"/>
    </Unit>
  </UnitDefinitions>

  <TypeDefinitions>
    <SimpleType name="Velocity" description="Linear velocity">
      <Real unit="m/s" quantity="Velocity" min="-1e9" max="1e9"/>
    </SimpleType>
    <SimpleType name="Angle">
      <Real unit="deg"/>
    </SimpleType>
  </TypeDefinitions>

  <DefaultExperiment startTime="0.0" stopTime="5.0" stepSize="0.01" tolerance="1e-5"/>

  <LogCategories>
    <Category name="logAll" description="All log messages"/>
  </LogCategories>

  <ModelVariables>
    <ScalarVariable name="x" valueReference="0" causality="output" variability="continuous" initial="exact">
      <Real start="0.0" unit="m/s" declaredType="Velocity"/>
    </ScalarVariable>
    <ScalarVariable name="u" valueReference="1" causality="input" variability="continuous">
      <Real start="0.0"/>
    </ScalarVariable>
    <ScalarVariable name="gain" valueReference="2" causality="parameter" variability="fixed" initial="exact">
      <Real start="2.0"/>
    </ScalarVariable>
    <ScalarVariable name="count" valueReference="3" causality="output" variability="discrete">
      <Integer start="0"/>
    </ScalarVariable>
    <ScalarVariable name="active" valueReference="4" causality="output" variability="discrete">
      <Boolean start="false"/>
    </ScalarVariable>
    <ScalarVariable name="label" valueReference="5" causality="output" variability="discrete">
      <String start="init"/>
    </ScalarVariable>
  </ModelVariables>
</fmiModelDescription>)";

// Minimal FMI 3.0 modelDescription
constexpr const char* kFmi3Minimal = R"(<?xml version="1.0" encoding="UTF-8"?>
<fmiModelDescription
    fmiVersion="3.0"
    modelName="TestModel3"
    instantiationToken="{fmi3-token-xyz}"
    description="FMI 3 test"
    generationTool="FMI3Tool">

  <CoSimulation modelIdentifier="TestModel3"/>

  <UnitDefinitions>
    <Unit name="rad/s">
      <BaseUnit factor="1.0"/>
    </Unit>
  </UnitDefinitions>

  <DefaultExperiment startTime="0.0" stopTime="2.0" stepSize="0.001" tolerance="1e-4"/>

  <ModelVariables>
    <Float64 name="omega" valueReference="0" causality="output" variability="continuous" start="0.0" unit="rad/s"/>
    <Float32 name="temp"  valueReference="1" causality="output" variability="continuous" start="20.0"/>
    <Int32   name="mode"  valueReference="2" causality="parameter" variability="fixed"    start="1"/>
    <Int64   name="ticks" valueReference="3" causality="output"    variability="discrete" start="0"/>
    <Boolean name="flag"  valueReference="4" causality="output"    variability="discrete" start="false"/>
    <String  name="tag"   valueReference="5" causality="output"    variability="discrete" start="none"/>
  </ModelVariables>
</fmiModelDescription>)";

} // anonymous namespace

// ===========================================================================
// FMI 2.0 tests
// ===========================================================================

class Fmi2ParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_NO_THROW(ModelDescriptionParser::parse(kFmi2Minimal, pkg));
    }
    FmuPackage pkg;
};

TEST_F(Fmi2ParserTest, DetectsFmi2Version) {
    EXPECT_EQ(pkg.fmiVersion, FmiVersion::FMI2);
    EXPECT_TRUE(pkg.isFmi2());
    EXPECT_FALSE(pkg.isFmi3());
}

TEST_F(Fmi2ParserTest, ParsesModelAttributes) {
    EXPECT_EQ(pkg.modelName, "TestModel");
    EXPECT_EQ(pkg.guid, "{abc-123}");
    EXPECT_EQ(pkg.description, "A test model");
    EXPECT_EQ(pkg.generationTool, "MyTool");
    EXPECT_EQ(pkg.variableNamingConvention, "flat");
    EXPECT_EQ(pkg.numberOfEventIndicators, 2u);
    EXPECT_EQ(pkg.modelIdentifier, "TestModel");
}

TEST_F(Fmi2ParserTest, ParsesUnitDefinitions) {
    ASSERT_EQ(pkg.unitDefinitions.size(), 2u);
    EXPECT_EQ(pkg.unitDefinitions[0].name, "m/s");
    EXPECT_DOUBLE_EQ(pkg.unitDefinitions[0].factor, 1.0);
    EXPECT_EQ(pkg.unitDefinitions[1].name, "deg");
    EXPECT_NEAR(pkg.unitDefinitions[1].factor, 0.0174532925, 1e-9);
}

TEST_F(Fmi2ParserTest, ParsesTypeDefinitions) {
    ASSERT_EQ(pkg.typeDefinitions.size(), 2u);
    EXPECT_EQ(pkg.typeDefinitions[0].name, "Velocity");
    EXPECT_EQ(pkg.typeDefinitions[0].description, "Linear velocity");
    EXPECT_EQ(pkg.typeDefinitions[0].unit, "m/s");
    EXPECT_EQ(pkg.typeDefinitions[0].quantity, "Velocity");
    EXPECT_EQ(pkg.typeDefinitions[0].dataType, DataType::Real);

    EXPECT_EQ(pkg.typeDefinitions[1].name, "Angle");
    EXPECT_EQ(pkg.typeDefinitions[1].unit, "deg");
}

TEST_F(Fmi2ParserTest, ParsesDefaultExperiment) {
    EXPECT_TRUE(pkg.defaultExperiment.hasStartTime);
    EXPECT_TRUE(pkg.defaultExperiment.hasStopTime);
    EXPECT_TRUE(pkg.defaultExperiment.hasStepSize);
    EXPECT_TRUE(pkg.defaultExperiment.hasTolerance);
    EXPECT_DOUBLE_EQ(pkg.defaultExperiment.startTime, 0.0);
    EXPECT_DOUBLE_EQ(pkg.defaultExperiment.stopTime, 5.0);
    EXPECT_DOUBLE_EQ(pkg.defaultExperiment.stepSize, 0.01);
    EXPECT_DOUBLE_EQ(pkg.defaultExperiment.tolerance, 1e-5);
}

TEST_F(Fmi2ParserTest, ParsesLogCategories) {
    ASSERT_EQ(pkg.logCategories.size(), 1u);
    EXPECT_EQ(pkg.logCategories[0].name, "logAll");
    EXPECT_EQ(pkg.logCategories[0].description, "All log messages");
}

TEST_F(Fmi2ParserTest, ParsesAllSixVariables) {
    ASSERT_EQ(pkg.variables.size(), 6u);
}

TEST_F(Fmi2ParserTest, ParsesRealOutputVariable) {
    const auto* v = pkg.findVariable("x");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->valueReference, 0u);
    EXPECT_EQ(v->causality, Causality::Output);
    EXPECT_EQ(v->variability, Variability::Continuous);
    EXPECT_EQ(v->initial, Initial::Exact);
    EXPECT_EQ(v->dataType, DataType::Real);
    EXPECT_EQ(v->unit, "m/s");
    EXPECT_EQ(v->typeName, "Velocity");
    ASSERT_TRUE(v->startValue.has_value());
    EXPECT_DOUBLE_EQ(std::get<double>(*v->startValue), 0.0);
}

TEST_F(Fmi2ParserTest, ParsesInputVariable) {
    const auto* v = pkg.findVariable("u");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->causality, Causality::Input);
}

TEST_F(Fmi2ParserTest, ParsesParameterVariable) {
    const auto* v = pkg.findVariable("gain");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->causality, Causality::Parameter);
    EXPECT_EQ(v->variability, Variability::Fixed);
    ASSERT_TRUE(v->startValue.has_value());
    EXPECT_DOUBLE_EQ(std::get<double>(*v->startValue), 2.0);
}

TEST_F(Fmi2ParserTest, ParsesIntegerVariable) {
    const auto* v = pkg.findVariable("count");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->dataType, DataType::Integer);
    ASSERT_TRUE(v->startValue.has_value());
    EXPECT_EQ(std::get<int64_t>(*v->startValue), 0);
}

TEST_F(Fmi2ParserTest, ParsesBooleanVariable) {
    const auto* v = pkg.findVariable("active");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->dataType, DataType::Boolean);
    ASSERT_TRUE(v->startValue.has_value());
    EXPECT_FALSE(std::get<bool>(*v->startValue));
}

TEST_F(Fmi2ParserTest, ParsesStringVariable) {
    const auto* v = pkg.findVariable("label");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->dataType, DataType::String);
    ASSERT_TRUE(v->startValue.has_value());
    EXPECT_EQ(std::get<std::string>(*v->startValue), "init");
}

TEST_F(Fmi2ParserTest, FindVariableReturnsNullptrForUnknownName) {
    EXPECT_EQ(pkg.findVariable("does_not_exist"), nullptr);
}

TEST_F(Fmi2ParserTest, InputsOutputsParametersFilters) {
    EXPECT_EQ(pkg.inputs().size(), 1u);
    EXPECT_EQ(pkg.outputs().size(), 4u);
    EXPECT_EQ(pkg.parameters().size(), 1u);
}

TEST_F(Fmi2ParserTest, RawModelDescriptionIsStored) {
    EXPECT_FALSE(pkg.rawModelDescription.empty());
    EXPECT_NE(pkg.rawModelDescription.find("TestModel"), std::string::npos);
}

// ===========================================================================
// FMI 3.0 tests
// ===========================================================================

class Fmi3ParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_NO_THROW(ModelDescriptionParser::parse(kFmi3Minimal, pkg));
    }
    FmuPackage pkg;
};

TEST_F(Fmi3ParserTest, DetectsFmi3Version) {
    EXPECT_EQ(pkg.fmiVersion, FmiVersion::FMI3);
    EXPECT_TRUE(pkg.isFmi3());
    EXPECT_FALSE(pkg.isFmi2());
}

TEST_F(Fmi3ParserTest, ParsesInstantiationToken) {
    EXPECT_EQ(pkg.modelName, "TestModel3");
    EXPECT_EQ(pkg.guid, "{fmi3-token-xyz}");
}

TEST_F(Fmi3ParserTest, ParsesDefaultExperiment) {
    EXPECT_DOUBLE_EQ(pkg.defaultExperiment.stopTime, 2.0);
    EXPECT_DOUBLE_EQ(pkg.defaultExperiment.stepSize, 0.001);
    EXPECT_DOUBLE_EQ(pkg.defaultExperiment.tolerance, 1e-4);
    EXPECT_TRUE(pkg.defaultExperiment.hasStopTime);
}

TEST_F(Fmi3ParserTest, ParsesFloat64Variable) {
    const auto* v = pkg.findVariable("omega");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->dataType, DataType::Float64);
    EXPECT_EQ(v->causality, Causality::Output);
    EXPECT_EQ(v->unit, "rad/s");
}

TEST_F(Fmi3ParserTest, ParsesFloat32Variable) {
    const auto* v = pkg.findVariable("temp");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->dataType, DataType::Float32);
    ASSERT_TRUE(v->startValue.has_value());
    EXPECT_DOUBLE_EQ(std::get<double>(*v->startValue), 20.0);
}

TEST_F(Fmi3ParserTest, ParsesInt32Parameter) {
    const auto* v = pkg.findVariable("mode");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->dataType, DataType::Int32);
    EXPECT_EQ(v->causality, Causality::Parameter);
}

TEST_F(Fmi3ParserTest, ParsesInt64Variable) {
    const auto* v = pkg.findVariable("ticks");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->dataType, DataType::Int64);
}

TEST_F(Fmi3ParserTest, ParsesBooleanFmi3) {
    const auto* v = pkg.findVariable("flag");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->dataType, DataType::Boolean);
}

TEST_F(Fmi3ParserTest, ParsesUnitDefinitions) {
    ASSERT_EQ(pkg.unitDefinitions.size(), 1u);
    EXPECT_EQ(pkg.unitDefinitions[0].name, "rad/s");
}

// ===========================================================================
// Error / edge-case tests
// ===========================================================================

TEST(ParserErrorTest, ThrowsOnEmptyXml) {
    FmuPackage pkg;
    EXPECT_THROW(ModelDescriptionParser::parse("", pkg), FmuLoadException);
}

TEST(ParserErrorTest, ThrowsOnMalformedXml) {
    FmuPackage pkg;
    EXPECT_THROW(
        ModelDescriptionParser::parse("<unclosed>", pkg),
        FmuLoadException);
}

TEST(ParserErrorTest, ThrowsOnMissingFmiVersion) {
    FmuPackage pkg;
    const std::string xml = R"(<?xml version="1.0"?><fmiModelDescription/>)";
    EXPECT_THROW(ModelDescriptionParser::parse(xml, pkg), FmuLoadException);
}

TEST(ParserErrorTest, ThrowsOnUnsupportedFmiVersion) {
    FmuPackage pkg;
    const std::string xml =
        R"(<?xml version="1.0"?><fmiModelDescription fmiVersion="1.0" modelName="X" guid="Y"/>)";
    EXPECT_THROW(ModelDescriptionParser::parse(xml, pkg), FmuLoadException);
}

TEST(ParserEdgeTest, ModelWithNoVariablesParsesSuccessfully) {
    FmuPackage pkg;
    const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<fmiModelDescription fmiVersion="2.0" modelName="Empty" guid="{00}">
  <CoSimulation modelIdentifier="Empty"/>
  <ModelVariables/>
</fmiModelDescription>)";
    ASSERT_NO_THROW(ModelDescriptionParser::parse(xml, pkg));
    EXPECT_TRUE(pkg.variables.empty());
}

TEST(ParserEdgeTest, ModelWithNoDefaultExperimentHasNoFlags) {
    FmuPackage pkg;
    const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<fmiModelDescription fmiVersion="2.0" modelName="NoExp" guid="{00}">
  <CoSimulation modelIdentifier="NoExp"/>
  <ModelVariables/>
</fmiModelDescription>)";
    ASSERT_NO_THROW(ModelDescriptionParser::parse(xml, pkg));
    EXPECT_FALSE(pkg.defaultExperiment.hasStopTime);
    EXPECT_FALSE(pkg.defaultExperiment.hasStepSize);
}

// ---------------------------------------------------------------------------
// FMI 3.0: Event Mode, Binary / Clock attributes and terminals (LS-BUS building blocks)
// ---------------------------------------------------------------------------

namespace {
constexpr const char* kFmi3Bus = R"(<?xml version="1.0" encoding="UTF-8"?>
<fmiModelDescription fmiVersion="3.0" modelName="Node" instantiationToken="{t}">
  <CoSimulation modelIdentifier="Node" hasEventMode="true" mightReturnEarlyFromDoStep="false" canGetAndSetFMUState="true"/>
  <ModelVariables>
    <Clock name="Bus.Tx_Clock" valueReference="1" causality="output" intervalVariability="triggered"/>
    <Binary name="Bus.Tx_Data" valueReference="2" causality="output" variability="discrete" initial="calculated" clocks="1" mimeType="application/org.fmi-standard.fmi-ls-bus.can;version=&quot;1.0.0&quot;" maxSize="2048"/>
    <Clock name="Bus.Rx_Clock" valueReference="3" causality="input" variability="discrete" intervalVariability="triggered"/>
    <Binary name="Bus.Rx_Data" valueReference="4" causality="input" variability="discrete" clocks="3 1"/>
  </ModelVariables>
</fmiModelDescription>)";

constexpr const char* kTerminals = R"(<?xml version="1.0" encoding="UTF-8"?>
<fmiTerminalsAndIcons fmiVersion="3.0">
  <Terminals>
    <Terminal name="Bus" terminalKind="org.fmi-ls-bus.network-terminal" matchingRule="org.fmi-ls-bus.transceiver">
      <TerminalMemberVariable variableName="Bus.Tx_Data" memberName="Tx_Data" variableKind="signal"/>
      <TerminalMemberVariable variableName="Bus.Rx_Clock" memberName="Rx_Clock" variableKind="signal"/>
      <Terminal name="Inner" matchingRule="plug"/>
    </Terminal>
  </Terminals>
</fmiTerminalsAndIcons>)";
}  // namespace

TEST(ParserFmi3BusTest, ReadsEventModeBinaryAndClockAttributes) {
    FmuPackage pkg;
    ModelDescriptionParser::parse(kFmi3Bus, pkg);
    EXPECT_TRUE(pkg.coSimulation.hasEventMode);
    EXPECT_FALSE(pkg.coSimulation.mightReturnEarlyFromDoStep);
    EXPECT_TRUE(pkg.coSimulation.canGetAndSetFMUState);
    ASSERT_EQ(pkg.variables.size(), 4u);
    EXPECT_EQ(pkg.variables[0].dataType, DataType::Clock);
    EXPECT_EQ(pkg.variables[0].intervalVariability, "triggered");
    EXPECT_EQ(pkg.variables[1].dataType, DataType::Binary);
    EXPECT_EQ(pkg.variables[1].mimeType, "application/org.fmi-standard.fmi-ls-bus.can;version=\"1.0.0\"");
    EXPECT_EQ(pkg.variables[1].maxSize, 2048u);
    ASSERT_EQ(pkg.variables[1].clocks.size(), 1u);
    EXPECT_EQ(pkg.variables[1].clocks[0], 1u);
    ASSERT_EQ(pkg.variables[3].clocks.size(), 2u);
    EXPECT_EQ(pkg.variables[3].clocks[1], 1u);
}

TEST(ParserFmi3BusTest, ReadsTerminalsIncludingNestedOnes) {
    FmuPackage pkg;
    ModelDescriptionParser::parseTerminals(kTerminals, pkg);
    ASSERT_EQ(pkg.terminals.size(), 2u);
    EXPECT_EQ(pkg.terminals[0].name, "Bus");
    EXPECT_EQ(pkg.terminals[0].terminalKind, "org.fmi-ls-bus.network-terminal");
    ASSERT_EQ(pkg.terminals[0].members.size(), 2u);
    EXPECT_EQ(pkg.terminals[0].members[0].memberName, "Tx_Data");
    EXPECT_EQ(pkg.terminals[1].name, "Bus.Inner");
}

TEST(ParserFmi3BusTest, MalformedTerminalsAreRejected) {
    FmuPackage pkg;
    EXPECT_THROW(ModelDescriptionParser::parseTerminals("<fmiTerminalsAndIcons><Terminals>", pkg), FmuLoadException);
}
