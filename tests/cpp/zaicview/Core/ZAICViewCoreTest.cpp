#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "IvPDomain.h"
#include "IvPFunction.h"
#include "PDMap.h"
#include "PopulatorVZAIC.h"
#include "TestFileUtils.h"
#include "ZAIC_HDG_Model.h"
#include "ZAIC_HLEQ_Model.h"
#include "ZAIC_Model.h"
#include "ZAIC_PEAK_Model.h"
#include "ZAIC_SPD_Model.h"
#include "ZAIC_VECT_Model.h"
#include "ZAIC_Vector.h"

#ifdef ZAICVIEW_ENABLE_GUI_TESTS
#include "ZAIC_GUI.h"
#include "ZAIC_HDG_GUI.h"
#include "ZAIC_HLEQ_GUI.h"
#include "ZAIC_PEAK_GUI.h"
#include "ZAIC_SPD_GUI.h"
#include "ZAIC_VECT_GUI.h"
#include "ZAIC_Viewer.h"
#endif

namespace {

std::string writeZaicFixture(const std::string& stem,
                             const std::string& contents)
{
  // Keep the TempFile objects alive so returned paths remain valid, and keep
  // the .zaic suffix expected by vector-ZAIC file tooling.
  static std::vector<std::unique_ptr<TempFile>> files;
  files.emplace_back(new TempFile(stem, contents, ".zaic"));
  return files.back()->path();
}

void expectDomain(const IvPDomain& domain,
                  const std::string& var,
                  double low,
                  double high,
                  unsigned int points)
{
  ASSERT_EQ(domain.size(), 1u);
  EXPECT_EQ(domain.getVarName(0), var);
  EXPECT_DOUBLE_EQ(domain.getVarLow(0), low);
  EXPECT_DOUBLE_EQ(domain.getVarHigh(0), high);
  EXPECT_EQ(domain.getVarPoints(0), points);
}

int g_destroyed_models = 0;

class CountingModel : public ZAIC_Model {
 public:
  ~CountingModel() override { ++g_destroyed_models; }
};

class ExposedHLEQModel : public ZAIC_HLEQ_Model {
 public:
  void setLEQMode(bool value) { m_leq_mode = value; }
  bool leqMode() const { return m_leq_mode; }
};

#ifdef ZAICVIEW_ENABLE_GUI_TESTS
class ExposedZAICViewer : public ZAIC_Viewer {
 public:
  ExposedZAICViewer() : ZAIC_Viewer(0, 0, 700, 460, "zaic-view-test") {}

  double backShade() const { return m_back_shade; }
  double gridShade() const { return m_grid_shade; }
  double lineShade() const { return m_line_shade; }
  bool drawLabels() const { return m_draw_labels; }
  int colorScheme() const { return m_color_scheme; }
  int gridWidth() const { return m_grid_wid; }
  int gridHeight() const { return m_grid_hgt; }
  int cellPixWidth() const { return m_cell_pix_wid; }
  std::string labelColor() const { return m_label_color; }
  std::string lineColor() const { return m_line_color; }
  std::string displayColor() const { return m_display_color; }
  std::string pointColor() const { return m_point_color; }
};

class ExposedHLEQGUI : public ZAIC_HLEQ_GUI {
 public:
  ExposedHLEQGUI() : ZAIC_HLEQ_GUI(700, 460, "zaic-hleq-output-test") {}

  std::string summitValue() const { return m_fld_summit->value(); }
  std::string baseWidthValue() const { return m_fld_bwidth->value(); }
  std::string minUtilValue() const { return m_fld_minutil->value(); }
  std::string maxUtilValue() const { return m_fld_maxutil->value(); }
  std::string summitDeltaValue() const { return m_fld_sumdelta->value(); }
};

bool hasMenuItem(ZAIC_GUI& gui, const std::string& item)
{
  return gui.mbar->find_item(item.c_str()) != nullptr;
}
#endif

}  // namespace

#ifdef ZAICVIEW_ENABLE_MODEL_TESTS
// Covers ZAIC model behavior: base model returns empty domain until subclass provides function.
TEST(ZAICModelTest, BaseModelReturnsEmptyDomainUntilSubclassProvidesFunction)
{
  ZAIC_Model base;
  EXPECT_EQ(base.getIvPDomain().size(), 0u);
  EXPECT_FALSE(base.setParam("summit", "10"));
  base.setDomain(250);
  base.moveX(1);
  base.currMode(2);
  EXPECT_EQ(base.getIvPDomain().size(), 0u);

  ZAIC_SPD_Model speed_model;
  expectDomain(speed_model.getIvPDomain(), "speed", 0, 5, 51);
}
#endif

#ifdef ZAICVIEW_ENABLE_GUI_TESTS
// Covers ZAIC viewer behavior: defaults match interactive ZAIC tool grid.
TEST(ZAICViewerTest, DefaultsMatchInteractiveZaicToolGrid)
{
  ExposedZAICViewer viewer;

  EXPECT_EQ(viewer.gridWidth(), 600);
  EXPECT_EQ(viewer.gridHeight(), 360);
  EXPECT_EQ(viewer.cellPixWidth(), 50);
  EXPECT_DOUBLE_EQ(viewer.backShade(), 1.0);
  EXPECT_DOUBLE_EQ(viewer.gridShade(), 1.0);
  EXPECT_DOUBLE_EQ(viewer.lineShade(), 1.0);
  EXPECT_TRUE(viewer.drawLabels());
  EXPECT_EQ(viewer.colorScheme(), 1);
  EXPECT_EQ(viewer.labelColor(), "black");
  EXPECT_EQ(viewer.lineColor(), "black");
  EXPECT_EQ(viewer.displayColor(), "hex:df,db,c3");
  EXPECT_EQ(viewer.pointColor(), "red");

  viewer.resize(0, 0, 900, 460);
  EXPECT_EQ(viewer.gridWidth(), 800);
}

// Covers ZAIC viewer behavior: view parameters clamp and toggle without drawing context.
TEST(ZAICViewerTest, ViewParametersClampAndToggleWithoutDrawingContext)
{
  ExposedZAICViewer viewer;

  for(int i = 0; i < 100; ++i)
    viewer.setParam("gridsize", "down");
  EXPECT_EQ(viewer.cellPixWidth(), 20);
  viewer.setParam("gridsize", "up");
  EXPECT_EQ(viewer.cellPixWidth(), 21);
  viewer.setParam("gridsize", "reset");
  EXPECT_EQ(viewer.cellPixWidth(), 50);

  for(int i = 0; i < 1000; ++i) {
    viewer.setParam("backshade", "down");
    viewer.setParam("gridshade", "down");
    viewer.setParam("lineshade", "down");
  }
  EXPECT_DOUBLE_EQ(viewer.backShade(), 0);
  EXPECT_DOUBLE_EQ(viewer.gridShade(), 0);
  EXPECT_DOUBLE_EQ(viewer.lineShade(), 0);

  for(int i = 0; i < 1000; ++i) {
    viewer.setParam("backshade", "up");
    viewer.setParam("gridshade", "up");
    viewer.setParam("lineshade", "up");
  }
  EXPECT_DOUBLE_EQ(viewer.backShade(), 10);
  EXPECT_DOUBLE_EQ(viewer.gridShade(), 10);
  EXPECT_DOUBLE_EQ(viewer.lineShade(), 10);

  viewer.setParam("backshade", "default");
  viewer.setParam("gridshade", "reset");
  viewer.setParam("lineshade", "reset");
  EXPECT_DOUBLE_EQ(viewer.backShade(), 1);
  EXPECT_DOUBLE_EQ(viewer.gridShade(), 1);
  EXPECT_DOUBLE_EQ(viewer.lineShade(), 1);

  viewer.setParam("labels", "false");
  EXPECT_FALSE(viewer.drawLabels());
  viewer.setParam("labels", "toggle");
  EXPECT_TRUE(viewer.drawLabels());
  viewer.setParam("labels", "true");
  EXPECT_TRUE(viewer.drawLabels());

  viewer.setParam("color_scheme", "toggle");
  EXPECT_EQ(viewer.colorScheme(), 0);
  EXPECT_EQ(viewer.labelColor(), "0.0,0.0,0.6");
  EXPECT_EQ(viewer.lineColor(), "white");
  viewer.setParam("color_scheme", "toggle");
  EXPECT_EQ(viewer.colorScheme(), 1);
}

// Covers ZAIC viewer behavior: owns and replaces models used by interactive tools.
TEST(ZAICViewerTest, OwnsAndReplacesModelsUsedByInteractiveTools)
{
  g_destroyed_models = 0;
  {
    ExposedZAICViewer viewer;
    viewer.setModel(new CountingModel());
    EXPECT_EQ(g_destroyed_models, 0);
    viewer.setModel(new CountingModel());
    EXPECT_EQ(g_destroyed_models, 1);
    viewer.setModel(nullptr);
    EXPECT_EQ(g_destroyed_models, 2);
  }
  EXPECT_EQ(g_destroyed_models, 2);
}

// Covers ZAICGUI behavior: base menu exposes shared ZAIC editing actions.
TEST(ZAICGUITest, BaseMenuExposesSharedZaicEditingActions)
{
  ZAIC_GUI gui(700, 460, "zaic-gui-test");

  ASSERT_NE(gui.mbar, nullptr);
  EXPECT_NE(gui.mbar->find_item("File/Quit "), nullptr);
  EXPECT_NE(gui.mbar->find_item("View-Options/Grid Size Reset "), nullptr);
  EXPECT_NE(gui.mbar->find_item("View-Options/Toggle ColorScheme "), nullptr);
  EXPECT_NE(gui.mbar->find_item("Modify ZAIC/Move Left  "), nullptr);
  EXPECT_NE(gui.mbar->find_item("Modify ZAIC/Move Right "), nullptr);
  EXPECT_EQ(gui.mbar->find_item("Modify ZAIC/Missing"), nullptr);
}

// Covers ZAICGUI behavior: app specific GUIs expose expected mission editing menus.
TEST(ZAICGUITest, AppSpecificGuisExposeExpectedMissionEditingMenus)
{
  ZAIC_HDG_GUI hdg_gui(700, 460, "zaic-hdg-test");
  EXPECT_TRUE(hasMenuItem(hdg_gui, "Modify Mode/Adjust MedVal "));
  EXPECT_TRUE(hasMenuItem(hdg_gui, "Modify Mode/Adjust HighMinUtil "));
  hdg_gui.updateOutput();
  EXPECT_STREQ(hdg_gui.m_fld_medval->value(), "180.00");
  EXPECT_STREQ(hdg_gui.m_fld_ldelta->value(), "10.00");
  hdg_gui.hide();

  ZAIC_SPD_GUI spd_gui(700, 460, "zaic-spd-test");
  EXPECT_TRUE(hasMenuItem(spd_gui, "Modify Mode/Adjust MedSpd "));
  EXPECT_TRUE(hasMenuItem(spd_gui, "Snap/Snap LowSpd to MinSpd "));
  spd_gui.updateOutput();
  EXPECT_STREQ(spd_gui.m_fld_med_spd->value(), "2.50");
  EXPECT_STREQ(spd_gui.m_fld_hgh_spd->value(), "5.00");
  spd_gui.hide();

  ZAIC_HLEQ_GUI hleq_gui(700, 460, "zaic-hleq-test");
  EXPECT_TRUE(hasMenuItem(hleq_gui, "Modify Mode/Adjust Summit Position "));
  EXPECT_TRUE(hasMenuItem(hleq_gui, "Modify Mode/Toggle HLEQ "));
  hleq_gui.updateOutput();
  hleq_gui.hide();

  ZAIC_PEAK_GUI peak_gui(700, 460, "zaic-peak-test");
  EXPECT_TRUE(hasMenuItem(peak_gui, "ZAIC-Elements-View/Both Elements Maxed    "));
  EXPECT_TRUE(hasMenuItem(peak_gui, "Modify ZAIC/Toggle WRAP "));
  peak_gui.updateOutput();
  EXPECT_STREQ(peak_gui.m_fld_summit1->value(), "125.25");
  EXPECT_STREQ(peak_gui.m_fld_summit2->value(), "325.65");
  peak_gui.hide();

  ZAIC_VECT_GUI vect_gui(700, 460, "zaic-vect-test");
  EXPECT_FALSE(hasMenuItem(vect_gui, "Modify ZAIC/Move Left  "));
  EXPECT_TRUE(hasMenuItem(vect_gui, "Tolerance/Greater + 1.0"));
  EXPECT_TRUE(hasMenuItem(vect_gui, "Tolerance/Lower - 0.1"));
  vect_gui.hide();
}

// Covers ZAICGUI behavior: HLEQ output fields track shared threshold model state.
TEST(ZAICGUITest, HLEQOutputFieldsTrackSharedThresholdModelState)
{
  ExposedHLEQGUI hleq_gui;
  hleq_gui.updateOutput();
  EXPECT_EQ(hleq_gui.summitValue(), "125.25");
  EXPECT_EQ(hleq_gui.baseWidthValue(), "50.10");
  EXPECT_EQ(hleq_gui.minUtilValue(), "0.00");
  EXPECT_EQ(hleq_gui.maxUtilValue(), "100.00");
  EXPECT_EQ(hleq_gui.summitDeltaValue(), "0.00");

  auto* model = static_cast<ZAIC_HLEQ_Model*>(hleq_gui.m_zaic_model);
  model->currMode(0);
  model->moveX(4.75);
  model->currMode(1);
  model->moveX(-0.10);
  model->currMode(3);
  model->moveX(25);
  model->currMode(4);
  model->moveX(12);
  hleq_gui.updateOutput();

  EXPECT_EQ(hleq_gui.summitValue(), "130.00");
  EXPECT_EQ(hleq_gui.baseWidthValue(), "50.00");
  EXPECT_EQ(hleq_gui.minUtilValue(), "0.00");
  EXPECT_EQ(hleq_gui.maxUtilValue(), "125.00");
  EXPECT_EQ(hleq_gui.summitDeltaValue(), "12.00");
  hleq_gui.hide();
}

// Covers ZAICGUI behavior: update output highlights the active editable field.
TEST(ZAICGUITest, UpdateOutputHighlightsTheActiveEditableField)
{
  const Fl_Color red = fl_rgb_color(220, 140, 140);
  const Fl_Color white = fl_rgb_color(255, 255, 255);
  const Fl_Color blue = fl_rgb_color(140, 140, 220);

  ZAIC_HDG_GUI hdg_gui(700, 460, "zaic-hdg-color-test");
  static_cast<ZAIC_HDG_Model*>(hdg_gui.m_zaic_model)->currMode(2);
  hdg_gui.updateOutput();
  EXPECT_EQ(hdg_gui.m_fld_hdelta->color(), red);
  EXPECT_EQ(hdg_gui.m_fld_medval->color(), white);
  hdg_gui.hide();

  ZAIC_SPD_GUI spd_gui(700, 460, "zaic-spd-color-test");
  static_cast<ZAIC_SPD_Model*>(spd_gui.m_zaic_model)->currMode(5);
  spd_gui.updateOutput();
  EXPECT_EQ(spd_gui.m_fld_min_spd_util->color(), red);
  EXPECT_EQ(spd_gui.m_fld_med_spd->color(), white);
  spd_gui.hide();

  ZAIC_PEAK_GUI peak_gui(700, 460, "zaic-peak-color-test");
  static_cast<ZAIC_PEAK_Model*>(peak_gui.m_zaic_model)->currMode(5);
  peak_gui.updateOutput();
  EXPECT_EQ(peak_gui.m_fld_pwidth2->color(), red);
  EXPECT_EQ(peak_gui.m_fld_summit2->color(), blue);
  EXPECT_EQ(peak_gui.m_fld_summit1->color(), white);
  peak_gui.hide();
}
#endif

#ifdef ZAICVIEW_ENABLE_MODEL_TESTS
// Covers ZAICHDG model behavior: edits heading summit deltas and utility bounds.
TEST(ZAICHDGModelTest, EditsHeadingSummitDeltasAndUtilityBounds)
{
  ZAIC_HDG_Model model;

  EXPECT_DOUBLE_EQ(model.getSummit(), 0);
  model.setDomain(360);
  expectDomain(model.getIvPDomain(), "x", 0, 359, 360);
  EXPECT_DOUBLE_EQ(model.getSummit(), 180);
  EXPECT_DOUBLE_EQ(model.getLowDelta(), 10);
  EXPECT_DOUBLE_EQ(model.getHighDelta(), 40);
  EXPECT_DOUBLE_EQ(model.getLowDeltaUtil(), 50);
  EXPECT_DOUBLE_EQ(model.getHighDeltaUtil(), 75);
  EXPECT_DOUBLE_EQ(model.getLowMinUtil(), 0);
  EXPECT_DOUBLE_EQ(model.getHighMinUtil(), 35);
  EXPECT_DOUBLE_EQ(model.getMaxUtil(), 100);

  model.currMode(0);
  model.moveX(200);
  EXPECT_DOUBLE_EQ(model.getSummit(), 20);
  model.moveX(-30);
  EXPECT_DOUBLE_EQ(model.getSummit(), 350);

  model.currMode(1);
  model.moveX(-500);
  EXPECT_DOUBLE_EQ(model.getLowDelta(), 0);
  model.currMode(2);
  model.moveX(500);
  EXPECT_DOUBLE_EQ(model.getHighDelta(), 180);
  model.currMode(3);
  model.moveX(100);
  EXPECT_DOUBLE_EQ(model.getLowDeltaUtil(), 100);
  model.currMode(4);
  model.moveX(-100);
  EXPECT_DOUBLE_EQ(model.getHighDeltaUtil(), 35);
  model.currMode(5);
  model.moveX(20);
  EXPECT_DOUBLE_EQ(model.getLowMinUtil(), 20);
  model.currMode(6);
  model.moveX(90);
  EXPECT_DOUBLE_EQ(model.getHighMinUtil(), 100);

  model.currMode(7);
  EXPECT_EQ(model.getCurrMode(), 6);

  model.setDomain(5);
  EXPECT_DOUBLE_EQ(model.getSummit(), 350);
  expectDomain(model.getIvPDomain(), "x", 0, 359, 360);
  model.setDomain(2000);
  expectDomain(model.getIvPDomain(), "x", 0, 359, 360);
  EXPECT_DOUBLE_EQ(model.getSummit(), 180);
}

// Covers ZAICHDG model behavior: extracts heading IvP function after valid domain initialization.
TEST(ZAICHDGModelTest, ExtractsHeadingIvPFunctionAfterValidDomainInitialization)
{
  ZAIC_HDG_Model model;
  model.setDomain(360);

  std::unique_ptr<IvPFunction> ipf(model.getIvPFunction());
  ASSERT_NE(ipf, nullptr);
  expectDomain(ipf->getPDMap()->getDomain(), "x", 0, 359, 360);
  EXPECT_GT(ipf->size(), 0u);
}

// Covers ZAICSPD model behavior: parses mission style domain and keeps speed triplet ordered.
TEST(ZAICSPDModelTest, ParsesMissionStyleDomainAndKeepsSpeedTripletOrdered)
{
  ZAIC_SPD_Model model;

  expectDomain(model.getIvPDomain(), "speed", 0, 5, 51);
  EXPECT_TRUE(model.setParam("domain", "speed,0,2,21"));
  expectDomain(model.getIvPDomain(), "speed", 0, 2, 21);
  EXPECT_TRUE(model.setParam("domain", "speed,0,2,21:course,0,359,360"));
  expectDomain(model.getIvPDomain(), "speed", 0, 2, 21);
  EXPECT_FALSE(model.setParam("bogus", "1"));

  EXPECT_TRUE(model.setParam("med_spd", "1.0"));
  EXPECT_TRUE(model.setParam("low_spd", "0.6"));
  EXPECT_TRUE(model.setParam("hgh_spd", "1.4"));
  EXPECT_FALSE(model.setParam("min_spd_util", "10"));
  EXPECT_FALSE(model.setParam("max_spd_util", "90"));
  EXPECT_DOUBLE_EQ(model.getMedVal(), 1.0);
  EXPECT_DOUBLE_EQ(model.getLowVal(), 0.6);
  EXPECT_DOUBLE_EQ(model.getHghVal(), 1.4);

  model.currMode(0);
  model.moveX(-100);
  EXPECT_DOUBLE_EQ(model.getMedVal(), 0);
  EXPECT_DOUBLE_EQ(model.getLowVal(), 0);
  model.moveX(100);
  EXPECT_DOUBLE_EQ(model.getMedVal(), 2);
  EXPECT_DOUBLE_EQ(model.getHghVal(), 2);

  model.currMode(3);
  model.moveX(-200);
  EXPECT_DOUBLE_EQ(model.getLowValUtil(), 0);
  model.currMode(4);
  model.moveX(200);
  EXPECT_DOUBLE_EQ(model.getHghValUtil(), 80);
  model.disableLowSpeed();
  model.disableHighSpeed();
  std::unique_ptr<IvPFunction> ipf(model.getIvPFunction());
  ASSERT_NE(ipf, nullptr);
  EXPECT_GT(ipf->size(), 0u);
}

// Covers ZAICSPD model behavior: pins cli utility parameters and atof parsing quirks.
TEST(ZAICSPDModelTest, PinsCliUtilityParametersAndAtofParsingQuirks)
{
  ZAIC_SPD_Model model;
  ASSERT_TRUE(model.setParam("domain", "speed,0,3,31"));

  EXPECT_TRUE(model.setParam("low_spd_util", "25"));
  EXPECT_TRUE(model.setParam("hgh_spd_util", "75"));
  EXPECT_DOUBLE_EQ(model.getLowValUtil(), 25);
  EXPECT_DOUBLE_EQ(model.getHghValUtil(), 75);

  EXPECT_TRUE(model.setParam("low_spd_util", "bad"));
  EXPECT_DOUBLE_EQ(model.getLowValUtil(), 0);
  EXPECT_TRUE(model.setParam("hgh_spd_util", "88 knots"));
  EXPECT_DOUBLE_EQ(model.getHghValUtil(), 88);

  EXPECT_TRUE(model.setParam("med_spd", "2.5 extra"));
  EXPECT_DOUBLE_EQ(model.getMedVal(), 2.5);
  EXPECT_TRUE(model.setParam("low_spd", "-10"));
  EXPECT_DOUBLE_EQ(model.getLowVal(), 0);
  EXPECT_TRUE(model.setParam("hgh_spd", "99"));
  EXPECT_DOUBLE_EQ(model.getHghVal(), 3);

  std::unique_ptr<IvPFunction> ipf(model.getIvPFunction());
  ASSERT_NE(ipf, nullptr);
  EXPECT_GT(ipf->size(), 0u);
}

// Covers ZAICSPD model behavior: exercises all edit modes and no ops invalid modes.
TEST(ZAICSPDModelTest, ExercisesAllEditModesAndNoOpsInvalidModes)
{
  ZAIC_SPD_Model model;
  ASSERT_TRUE(model.setParam("domain", "speed,0,4,41"));
  ASSERT_TRUE(model.setParam("med_spd", "2.0"));
  ASSERT_TRUE(model.setParam("low_spd", "1.0"));
  ASSERT_TRUE(model.setParam("hgh_spd", "3.0"));

  model.currMode(1);
  model.moveX(100);
  EXPECT_DOUBLE_EQ(model.getLowVal(), 2.0);
  model.moveX(-100);
  EXPECT_DOUBLE_EQ(model.getLowVal(), 0.0);

  model.currMode(2);
  model.moveX(-100);
  EXPECT_DOUBLE_EQ(model.getHghVal(), 2.0);
  model.moveX(100);
  EXPECT_DOUBLE_EQ(model.getHghVal(), 4.0);

  model.currMode(5);
  model.moveX(100);
  EXPECT_DOUBLE_EQ(model.getLowMinUtil(), 100);
  model.moveX(-200);
  EXPECT_DOUBLE_EQ(model.getLowMinUtil(), 0);

  model.currMode(6);
  model.moveX(100);
  EXPECT_DOUBLE_EQ(model.getHighMinUtil(), 100);
  model.moveX(-200);
  EXPECT_DOUBLE_EQ(model.getHighMinUtil(), 0);

  const double high_before = model.getHighMinUtil();
  model.currMode(7);
  EXPECT_EQ(model.getCurrMode(), 6);
  model.moveX(5);
  EXPECT_DOUBLE_EQ(model.getHighMinUtil(), high_before + 5);
}

// Covers ZAICHLEQ model behavior: shared LEQHEQ model edits threshold utility shape.
TEST(ZAICHLEQModelTest, SharedLEQHEQModelEditsThresholdUtilityShape)
{
  ZAIC_HLEQ_Model model;

  EXPECT_DOUBLE_EQ(model.getSummit(), 25);
  EXPECT_DOUBLE_EQ(model.getBaseWidth(), 10);
  EXPECT_DOUBLE_EQ(model.getMinUtil(), 0);
  EXPECT_DOUBLE_EQ(model.getMaxUtil(), 100);

  model.currMode(0);
  model.moveX(5);
  EXPECT_DOUBLE_EQ(model.getSummit(), 30);
  model.currMode(1);
  model.moveX(-50);
  EXPECT_DOUBLE_EQ(model.getBaseWidth(), 0);
  model.currMode(2);
  model.moveX(250);
  EXPECT_DOUBLE_EQ(model.getMinUtil(), 0);
  model.currMode(3);
  model.moveX(250);
  EXPECT_DOUBLE_EQ(model.getMaxUtil(), 200);
  model.currMode(4);
  model.moveX(12);
  EXPECT_DOUBLE_EQ(model.getSummitDelta(), 12);

  model.currMode(99);
  model.moveX(1);
  EXPECT_DOUBLE_EQ(model.getSummitDelta(), 13);
}

// Covers ZAICHLEQ model behavior: extracts both LEQ and HEQ functions when mode is initialized.
TEST(ZAICHLEQModelTest, ExtractsBothLEQAndHEQFunctionsWhenModeIsInitialized)
{
  ExposedHLEQModel model;

  model.setLEQMode(true);
  EXPECT_TRUE(model.leqMode());
  {
    std::unique_ptr<IvPFunction> ipf(model.getIvPFunction());
    ASSERT_NE(ipf, nullptr);
    expectDomain(ipf->getPDMap()->getDomain(), "x", 0, 99, 100);
    EXPECT_GT(ipf->size(), 0u);
  }

  model.toggleHLEQ();
  EXPECT_FALSE(model.leqMode());
  {
    std::unique_ptr<IvPFunction> ipf(model.getIvPFunction());
    ASSERT_NE(ipf, nullptr);
    expectDomain(ipf->getPDMap()->getDomain(), "x", 0, 99, 100);
    EXPECT_GT(ipf->size(), 0u);
  }

  model.setDomain(5);
  EXPECT_DOUBLE_EQ(model.getSummit(), 25);
  model.setDomain(2000);
  EXPECT_DOUBLE_EQ(model.getSummit(), 250.25);
}

// Covers ZAICpeak model behavior: edits two component peak and draw mode extraction.
TEST(ZAICPEAKModelTest, EditsTwoComponentPeakAndDrawModeExtraction)
{
  ZAIC_PEAK_Model model;

  expectDomain(model.getIvPDomain(), "x", 0, 99, 100);
  EXPECT_DOUBLE_EQ(model.getSummit1(), 25);
  EXPECT_DOUBLE_EQ(model.getPeakWidth1(), 10);
  EXPECT_DOUBLE_EQ(model.getBaseWidth1(), 25);
  EXPECT_DOUBLE_EQ(model.getSummitDelta1(), 80);
  EXPECT_DOUBLE_EQ(model.getSummit2(), 65);
  EXPECT_DOUBLE_EQ(model.getPeakWidth2(), 5);
  EXPECT_DOUBLE_EQ(model.getBaseWidth2(), 30);
  EXPECT_DOUBLE_EQ(model.getSummitDelta2(), 45);

  model.currMode(0);
  model.moveX(-1000);
  EXPECT_DOUBLE_EQ(model.getSummit1(), 0);
  model.currMode(4);
  model.moveX(1000);
  EXPECT_DOUBLE_EQ(model.getSummit2(), 100);
  model.currMode(1);
  model.moveX(-50);
  EXPECT_DOUBLE_EQ(model.getPeakWidth1(), 0);
  model.currMode(2);
  model.moveX(-50);
  EXPECT_DOUBLE_EQ(model.getBaseWidth1(), 0);
  model.currMode(3);
  model.moveX(500);
  EXPECT_DOUBLE_EQ(model.getSummitDelta1(), 100);
  model.currMode(6);
  model.moveX(-50);
  EXPECT_DOUBLE_EQ(model.getBaseWidth2(), 0);
  model.currMode(5);
  model.moveX(-50);
  EXPECT_DOUBLE_EQ(model.getPeakWidth2(), 0);
  model.currMode(7);
  model.moveX(500);
  EXPECT_DOUBLE_EQ(model.getSummitDelta2(), 100);
  model.currMode(8);
  model.moveX(-200);
  EXPECT_DOUBLE_EQ(model.getMaxUtil(), 1);
  model.currMode(9);
  model.moveX(200);
  EXPECT_DOUBLE_EQ(model.getMinUtil(), 0);

  for(int mode = 1; mode <= 4; ++mode) {
    model.drawMode(mode);
    std::unique_ptr<IvPFunction> ipf(model.getIvPFunction());
    ASSERT_NE(ipf, nullptr);
    EXPECT_GT(ipf->size(), 0u);
  }
  model.toggleValueWrap();
  model.drawMode(0);
  std::unique_ptr<IvPFunction> toggled(model.getIvPFunction());
  ASSERT_NE(toggled, nullptr);

  model.setDomain(5);
  expectDomain(model.getIvPDomain(), "x", 0, 99, 100);
  model.setDomain(2000);
  expectDomain(model.getIvPDomain(), "x", 0, 1000, 1001);
}

// Covers ZAICpeak model behavior: invalid modes do not change peak state.
TEST(ZAICPEAKModelTest, InvalidModesDoNotChangePeakState)
{
  ZAIC_PEAK_Model model;
  model.currMode(0);
  model.drawMode(1);
  const double summit = model.getSummit1();
  const double maxutil = model.getMaxUtil();

  model.currMode(10);
  EXPECT_EQ(model.getCurrMode(), 0);
  model.moveX(1);
  EXPECT_DOUBLE_EQ(model.getSummit1(), summit + 1);

  model.drawMode(99);
  std::unique_ptr<IvPFunction> ipf(model.getIvPFunction());
  ASSERT_NE(ipf, nullptr);
  EXPECT_DOUBLE_EQ(model.getMaxUtil(), maxutil);
}

// Covers ZAIC vect model behavior: populates vector ZAIC from file and applies tolerance.
TEST(ZAICVectModelTest, PopulatesVectorZaicFromFileAndAppliesTolerance)
{
  const std::string path = writeZaicFixture(
      "mission_vector",
      "// app_zaic_vect style utility vector\n"
      "ivpdomain = speed,0,4,5\n"
      "domain = 0,1,2,3,4\n"
      "range = 0,30,100,30,0\n"
      "minutil = 0\n"
      "maxutil = 100\n");

  PopulatorVZAIC populator;
  EXPECT_FALSE(populator.readFile("/tmp/missing_vector_zaic_file"));
  EXPECT_EQ(populator.buildZAIC(), nullptr);
  ASSERT_TRUE(populator.readFile(path));
  ZAIC_Vector* zaic = populator.buildZAIC();
  ASSERT_NE(zaic, nullptr);

  ZAIC_VECT_Model model;
  EXPECT_EQ(model.getIvPFunction(), nullptr);
  EXPECT_DOUBLE_EQ(model.getTolerance(), 0);
  model.setZAIC(zaic);
  EXPECT_DOUBLE_EQ(model.getMinUtil(), 0);
  EXPECT_DOUBLE_EQ(model.getMaxUtil(), 100);

  model.setTolerance(-5);
  EXPECT_DOUBLE_EQ(model.getTolerance(), 0);
  model.modTolerance(1.5);
  EXPECT_DOUBLE_EQ(model.getTolerance(), 1.5);
  model.modTolerance(-3);
  EXPECT_DOUBLE_EQ(model.getTolerance(), 0);

  std::unique_ptr<IvPFunction> ipf(model.getIvPFunction());
  ASSERT_NE(ipf, nullptr);
  EXPECT_EQ(model.getTotalPieces(), 0u);
  EXPECT_GT(ipf->size(), 0u);
  expectDomain(ipf->getPDMap()->getDomain(), "speed", 0, 4, 5);
}

// Covers ZAIC vect model behavior: rejects mismatched vector ZAIC payloads at build time.
TEST(ZAICVectModelTest, RejectsMismatchedVectorZaicPayloadsAtBuildTime)
{
  const std::string path = writeZaicFixture(
      "mismatched_vector",
      "ivpdomain = depth,0,10,11\n"
      "domain = 0,5,10\n"
      "range = 0,100\n");

  PopulatorVZAIC populator;
  ASSERT_TRUE(populator.readFile(path));
  EXPECT_EQ(populator.buildZAIC(), nullptr);

  ZAIC_VECT_Model empty_model;
  empty_model.setTolerance(10);
  empty_model.modTolerance(10);
  EXPECT_DOUBLE_EQ(empty_model.getTolerance(), 0);
  EXPECT_DOUBLE_EQ(empty_model.getMinUtil(), 0);
  EXPECT_DOUBLE_EQ(empty_model.getMaxUtil(), 0);
  EXPECT_EQ(empty_model.getTotalPieces(), 0u);
}

// Covers ZAIC vect model behavior: empty files fail and repeated reads accumulate vector rows.
TEST(ZAICVectModelTest, EmptyFilesFailAndRepeatedReadsAccumulateVectorRows)
{
  const std::string empty_path = writeZaicFixture("empty_vector", "");

  PopulatorVZAIC empty_populator;
  EXPECT_TRUE(empty_populator.readFile(empty_path));
  EXPECT_EQ(empty_populator.buildZAIC(), nullptr);

  const std::string first_path = writeZaicFixture(
      "first_vector",
      "ivpdomain = speed,0,2,3\n"
      "domain = 0,1,2\n"
      "range = 0,50,100\n");

  const std::string second_path = writeZaicFixture(
      "second_vector",
      "ivpdomain = speed,0,2,3\n"
      "domain = 0,1,2\n"
      "range = 100,50,0\n");

  PopulatorVZAIC reused_populator;
  ASSERT_TRUE(reused_populator.readFile(first_path));
  // Reusing a PopulatorVZAIC accumulates rows from repeated reads instead of
  // clearing the first file's vector data.
  ASSERT_TRUE(reused_populator.readFile(second_path));
  ZAIC_Vector* zaic = reused_populator.buildZAIC();
  ASSERT_NE(zaic, nullptr);

  ZAIC_VECT_Model model;
  model.setZAIC(zaic);
  EXPECT_DOUBLE_EQ(model.getMinUtil(), 0);
  EXPECT_DOUBLE_EQ(model.getMaxUtil(), 100);
  std::unique_ptr<IvPFunction> ipf(model.getIvPFunction());
  ASSERT_NE(ipf, nullptr);
  expectDomain(ipf->getPDMap()->getDomain(), "speed", 0, 2, 3);
}

// Covers ZAIC vect model behavior: pins parser quirks for comments nonnumeric values and unused min max.
TEST(ZAICVectModelTest, PinsParserQuirksForCommentsNonnumericValuesAndUnusedMinMax)
{
  // This pins the current parser quirks: only slash comments are ignored,
  // atof-style conversion accepts "bad" as zero, and min/max lines are not used.
  const std::string path = writeZaicFixture(
      "quirky_vector",
      "// full-line slash comments are ignored\n"
      "# hash comments are treated as unknown keys and ignored\n"
      "ivpdomain = depth,0,3,4\n"
      "domain = 0,1,bad,3\n"
      "range = 10,20,30,40\n"
      "minutil = 15\n"
      "maxutil = 85\n");

  PopulatorVZAIC populator;
  ASSERT_TRUE(populator.readFile(path));
  ZAIC_Vector* zaic = populator.buildZAIC();
  ASSERT_NE(zaic, nullptr);

  ZAIC_VECT_Model model;
  model.setZAIC(zaic);
  EXPECT_DOUBLE_EQ(model.getMinUtil(), 10);
  EXPECT_DOUBLE_EQ(model.getMaxUtil(), 40);

  std::unique_ptr<IvPFunction> ipf(model.getIvPFunction());
  ASSERT_NE(ipf, nullptr);
  expectDomain(ipf->getPDMap()->getDomain(), "depth", 0, 3, 4);
}
#endif
