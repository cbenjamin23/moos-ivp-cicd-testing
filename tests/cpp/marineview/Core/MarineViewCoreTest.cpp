#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#if defined(MARINEVIEW_ENABLE_BACKIMG_TESTS) || defined(MARINEVIEW_ENABLE_GUI_TESTS)
#include <tiffio.h>
#endif

#include "BearingLine.h"
#include "LMV_Utils.h"
#include "MOOS/libMOOSGeodesy/MOOSGeodesy.h"
#include "OpAreaSpec.h"
#include "TestFileUtils.h"
#include "VehicleSet.h"

#if defined(MARINEVIEW_ENABLE_BACKIMG_TESTS) || defined(MARINEVIEW_ENABLE_GUI_TESTS)
#include "BackImg.h"
#endif

#ifdef MARINEVIEW_ENABLE_GUI_TESTS
#include "MarineVehiGUI.h"
#include "MarineViewer.h"
#include "MY_Fl_Hold_Browser.h"
#include "MY_Output.h"
#endif

namespace {

void expectColorNear(const std::vector<double>& color,
                     double r,
                     double g,
                     double b)
{
  ASSERT_GE(color.size(), 3u);
  EXPECT_NEAR(color[0], r, 1e-9);
  EXPECT_NEAR(color[1], g, 1e-9);
  EXPECT_NEAR(color[2], b, 1e-9);
}

std::string nodeReport(const std::string& name,
                       const std::string& type,
                       double x,
                       double y,
                       double speed,
                       double heading,
                       double timestamp,
                       const std::string& extra = "")
{
  std::string report = "NAME=" + name + ",TYPE=" + type +
                       ",TIME=" + std::to_string(timestamp) +
                       ",X=" + std::to_string(x) +
                       ",Y=" + std::to_string(y) +
                       ",SPD=" + std::to_string(speed) +
                       ",HDG=" + std::to_string(heading);
  if(!extra.empty())
    report += "," + extra;
  return report;
}

#if defined(MARINEVIEW_ENABLE_BACKIMG_TESTS) || defined(MARINEVIEW_ENABLE_GUI_TESTS)
void writeTinyTiff(const std::string& path)
{
  TIFF* tiff = TIFFOpen(path.c_str(), "w");
  ASSERT_NE(tiff, nullptr);

  TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, 2);
  TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, 2);
  TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 4);
  TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8);
  TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, 2);

  unsigned char row0[8] = {255, 0, 0, 255, 0, 255, 0, 255};
  unsigned char row1[8] = {0, 0, 255, 255, 255, 255, 255, 255};
  ASSERT_NE(TIFFWriteScanline(tiff, row0, 0, 0), -1);
  ASSERT_NE(TIFFWriteScanline(tiff, row1, 1, 0), -1);
  TIFFClose(tiff);
}
#endif

}  // namespace

#ifdef MARINEVIEW_ENABLE_GUI_TESTS
class ExposedMarineViewer : public MarineViewer {
 public:
  ExposedMarineViewer() : MarineViewer(0, 0, 400, 300, "marineview-test") {}

  using MarineViewer::coordInView;
  using MarineViewer::coordInViewX;
  using MarineViewer::img2meters;
  using MarineViewer::img2view;
  using MarineViewer::meters2img;
  using MarineViewer::view2img;

  double hashShade() const { return m_hash_shade; }
  double fillShade() const { return m_fill_shade; }
};
#endif

#ifdef MARINEVIEW_ENABLE_BACKIMG_TESTS
class ExposedBackImg : public BackImg {
 public:
  using BackImg::readTiffInfo;
};
#endif

#ifdef MARINEVIEW_ENABLE_GUI_TESTS
class ExposedMarineVehiGUI : public MarineVehiGUI {
 public:
  ExposedMarineVehiGUI() : MarineVehiGUI(640, 480, "marine-gui-test")
  {
    m_mviewer = new ExposedMarineViewer();
  }

  bool hasMenuItem(const std::string& item) const
  {
    return m_menubar->find_item(item.c_str()) != nullptr;
  }

  bool menuItemSelected(const std::string& item) const
  {
    const Fl_Menu_Item* menu_item = m_menubar->find_item(item.c_str());
    return menu_item && menu_item->value();
  }

  std::string viewerGeoSetting(const std::string& key)
  {
    return m_mviewer->geosetting(key);
  }

  std::string viewerVehiSetting(const std::string& key)
  {
    return m_mviewer->vehisetting(key);
  }

};
#endif

#ifdef MARINEVIEW_ENABLE_CORE_TESTS
// Covers bearing line behavior: defaults to invalid orange absolute vector.
TEST(BearingLineTest, DefaultsToInvalidOrangeAbsoluteVector)
{
  BearingLine line;

  EXPECT_FALSE(line.isValid());
  EXPECT_DOUBLE_EQ(line.getBearing(), 0);
  EXPECT_TRUE(line.isBearingAbsolute());
  EXPECT_DOUBLE_EQ(line.getRange(), 50);
  EXPECT_DOUBLE_EQ(line.getTimeLimit(), -1);
  EXPECT_DOUBLE_EQ(line.getTimeStamp(), -1);
  EXPECT_DOUBLE_EQ(line.getVectorWidth(), 1);
  EXPECT_EQ(line.getVectorColor(), "orange");
  EXPECT_EQ(line.getLabel(), "");
}

// Covers bearing line behavior: stores bearing line metadata used by viewer overlays.
TEST(BearingLineTest, StoresBearingLineMetadataUsedByViewerOverlays)
{
  BearingLine line(270, 125, 30, 1000.5);
  ASSERT_TRUE(line.isValid());
  EXPECT_DOUBLE_EQ(line.getBearing(), 270);
  EXPECT_DOUBLE_EQ(line.getRange(), 125);
  EXPECT_DOUBLE_EQ(line.getTimeLimit(), 30);
  EXPECT_DOUBLE_EQ(line.getTimeStamp(), 1000.5);
  EXPECT_TRUE(line.isBearingAbsolute());

  line.setBearing(45);
  line.setBearingAbsolute(false);
  line.setRange(80);
  line.setTimeLimit(5);
  line.setTimeStamp(1010);
  line.setVectorWidth(3.5);
  line.setVectorColor("dodger_blue");
  line.setLabel("contact-bearing");

  EXPECT_TRUE(line.isValid());
  EXPECT_DOUBLE_EQ(line.getBearing(), 45);
  EXPECT_FALSE(line.isBearingAbsolute());
  EXPECT_DOUBLE_EQ(line.getRange(), 80);
  EXPECT_DOUBLE_EQ(line.getTimeLimit(), 5);
  EXPECT_DOUBLE_EQ(line.getTimeStamp(), 1010);
  EXPECT_DOUBLE_EQ(line.getVectorWidth(), 3.5);
  EXPECT_EQ(line.getVectorColor(), "dodger_blue");
  EXPECT_EQ(line.getLabel(), "contact-bearing");
}

// Covers lmv utils behavior: reads mission entries matching colon separated aliases.
TEST(LMVUtilsTest, ReadsMissionEntriesMatchingColonSeparatedAliases)
{
  TempFile file("marineview_entries",
                "# pMarineViewer visual artifacts\n"
                " polygon = pts={0,0:10,0:10,10},label=survey_box\n"
                "POLY = pts={1,1:2,1:2,2},label=hold_box\n"
                " seglist = pts={0,0:5,5},label=trackline\n"
                "bad_line_without_equals\n"
                " point = x=1,y=2\n",
                ".moos");

  const std::vector<std::string> polys =
      readEntriesFromFile(file.path(), "polygon:poly");
  ASSERT_EQ(polys.size(), 2u);
  EXPECT_EQ(polys[0], "pts={0,0:10,0:10,10},label=survey_box");
  EXPECT_EQ(polys[1], "pts={1,1:2,1:2,2},label=hold_box");

  const std::vector<std::string> lines =
      readEntriesFromFile(file.path(), "seglist:segl");
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_EQ(lines[0], "pts={0,0:5,5},label=trackline");
  EXPECT_TRUE(readEntriesFromFile(file.path(), "circle").empty());
}

// Covers lmv utils behavior: missing files and unmatched parameters return empty vectors.
TEST(LMVUtilsTest, MissingFilesAndUnmatchedParametersReturnEmptyVectors)
{
  TempDir root("marineview_missing");
  EXPECT_TRUE(readEntriesFromFile(root.filePath("no_such_marineview_file.moos"),
                                  "polygon:poly").empty());
}

// Covers lmv utils behavior: preserves payloads with embedded equals and requires exact aliases.
TEST(LMVUtilsTest, PreservesPayloadsWithEmbeddedEqualsAndRequiresExactAliases)
{
  TempFile file("marineview_payloads",
                "VIEW_POLYGON = pts={0,0:10,0},label=survey=alpha\n"
                " view_polygon = pts={1,1:2,2},msg={status=hot}\n"
                "view_range_pulse=x=1,y=2,radius=40 # inline comments persist\n",
                ".moos");

  EXPECT_TRUE(readEntriesFromFile(file.path(), "VIEW_POLYGON").empty());

  const std::vector<std::string> polygons =
      readEntriesFromFile(file.path(), "view_polygon:polygon");
  ASSERT_EQ(polygons.size(), 2u);
  EXPECT_EQ(polygons[0], "pts={0,0:10,0},label=survey=alpha");
  EXPECT_EQ(polygons[1], "pts={1,1:2,2},msg={status=hot}");

  const std::vector<std::string> pulses =
      readEntriesFromFile(file.path(), "view_range_pulse");
  ASSERT_EQ(pulses.size(), 1u);
  EXPECT_EQ(pulses[0], "x=1,y=2,radius=40 # inline comments persist");
}
#endif

#ifdef MARINEVIEW_ENABLE_BACKIMG_TESTS
// Covers back img behavior: starts empty and converts pixels to fractions after info load.
TEST(BackImgTest, StartsEmptyAndConvertsPixelsToFractionsAfterInfoLoad)
{
  BackImg image;
  EXPECT_EQ(image.get_img_data(), nullptr);
  EXPECT_EQ(image.get_img_pix_width(), 0u);
  EXPECT_EQ(image.get_img_pix_height(), 0u);
  EXPECT_EQ(image.get_img_mtr_width(), 0u);
  EXPECT_EQ(image.get_img_mtr_height(), 0u);
  EXPECT_EQ(image.getTiffFile(), "");
  EXPECT_EQ(image.getInfoFile(), "");

  ASSERT_TRUE(image.readTiffInfoEmpty(42.36, 42.35, -70.98, -71.00));
  EXPECT_EQ(image.get_img_pix_width(), 1000u);
  EXPECT_EQ(image.get_img_pix_height(), 1000u);
  EXPECT_GT(image.get_img_mtr_width(), 0u);
  EXPECT_GT(image.get_img_mtr_height(), 0u);
  EXPECT_NEAR(image.pixToPctX(250), 0.25, 1e-9);
  EXPECT_NEAR(image.pixToPctY(750), 0.75, 1e-9);
  EXPECT_LT(image.get_x_at_img_left(), image.get_x_at_img_right());
  EXPECT_LT(image.get_y_at_img_bottom(), image.get_y_at_img_top());
  EXPECT_NEAR(image.get_x_at_img_ctr(), 0, 1e-6);
  EXPECT_NEAR(image.get_y_at_img_ctr(), 0, 1e-6);
  EXPECT_NEAR(image.get_pix_per_mtr_x() * image.get_img_mtr_width(),
              1000.0, 1.0);
  EXPECT_NEAR(image.get_pix_per_mtr_y() * image.get_img_mtr_height(),
              1000.0, 1.0);
}

// Covers back img behavior: datum overrides shift local image center within boundary map.
TEST(BackImgTest, DatumOverridesShiftLocalImageCenterWithinBoundaryMap)
{
  BackImg image;
  ASSERT_TRUE(image.readTiffInfoEmpty(42.36, 42.35, -70.98, -71.00));
  const double default_x_center = image.get_x_at_img_ctr();
  const double default_y_center = image.get_y_at_img_ctr();

  image.setDatumLatLon(42.355, -70.99);
  EXPECT_NEAR(image.get_x_at_img_ctr(), default_x_center, 1e-3);
  EXPECT_NEAR(image.get_y_at_img_ctr(), default_y_center, 1e-3);

  image.setDatumLatLon(42.358, -70.992);
  EXPECT_NE(image.get_x_at_img_ctr(), default_x_center);
  EXPECT_NE(image.get_y_at_img_ctr(), default_y_center);
  EXPECT_LT(image.get_x_at_img_left(), image.get_x_at_img_right());
  EXPECT_LT(image.get_y_at_img_bottom(), image.get_y_at_img_top());
}

// Covers back img behavior: copies derived geometry and can locate matching TIFF info pair.
TEST(BackImgTest, CopiesDerivedGeometryAndCanLocateMatchingTiffInfoPair)
{
  BackImg image;
  ASSERT_TRUE(image.readTiffInfoEmpty(42.36, 42.35, -70.98, -71.00));
  image.setDatumLatLon(42.358, -70.992);

  BackImg copy;
  copy.copy(image);
  EXPECT_EQ(copy.get_img_pix_width(), image.get_img_pix_width());
  EXPECT_EQ(copy.get_img_pix_height(), image.get_img_pix_height());
  EXPECT_DOUBLE_EQ(copy.get_x_at_img_left(), image.get_x_at_img_left());
  EXPECT_DOUBLE_EQ(copy.get_x_at_img_right(), image.get_x_at_img_right());
  EXPECT_DOUBLE_EQ(copy.get_y_at_img_bottom(), image.get_y_at_img_bottom());
  EXPECT_DOUBLE_EQ(copy.get_y_at_img_top(), image.get_y_at_img_top());
  copy.clearData();
  EXPECT_EQ(copy.get_img_data(), nullptr);

  TempDir root("viewer_backdrop_pair");
  const std::string base = root.filePath("viewer_backdrop.tif");
  const std::string info_path = root.filePath("viewer_backdrop.info");
  {
    std::ofstream tiff(base.c_str());
    std::ofstream info(info_path.c_str());
    ASSERT_TRUE(tiff.is_open());
    ASSERT_TRUE(info.is_open());
  }

  BackImg locator;
  EXPECT_TRUE(locator.locateTiffAndInfoFiles(base));
  EXPECT_EQ(locator.getTiffFile(), base);
  EXPECT_EQ(locator.getInfoFile(), info_path);
  EXPECT_FALSE(locator.locateTiffAndInfoFiles(""));
}

// Covers back img behavior: reads tiny TIFF and old style info pair for viewer backdrop.
TEST(BackImgTest, ReadsTinyTiffAndOldStyleInfoPairForViewerBackdrop)
{
  TempDir root("tiny_viewer_backdrop");
  const std::string tiff_path = root.filePath("tiny_viewer_backdrop.tif");
  const std::string info_path = root.filePath("tiny_viewer_backdrop.info");
  writeTinyTiff(tiff_path);

  std::ofstream info(info_path.c_str());
  ASSERT_TRUE(info.is_open());
  info << "img_centx = 0.5\n";
  info << "img_centy = 0.5\n";
  info << "img_meters = 10\n";
  info.close();

  BackImg image;
  ASSERT_TRUE(image.readTiff(tiff_path));
  EXPECT_NE(image.get_img_data(), nullptr);
  EXPECT_EQ(image.get_img_pix_width(), 2u);
  EXPECT_EQ(image.get_img_pix_height(), 2u);
  EXPECT_DOUBLE_EQ(image.get_img_meters(), 10);
  EXPECT_DOUBLE_EQ(image.get_x_at_img_left(), -5);
  EXPECT_DOUBLE_EQ(image.get_x_at_img_right(), 5);
  EXPECT_DOUBLE_EQ(image.get_y_at_img_bottom(), -5);
  EXPECT_DOUBLE_EQ(image.get_y_at_img_top(), 5);
  EXPECT_EQ(image.getTiffFile(), tiff_path);
  EXPECT_EQ(image.getInfoFile(), info_path);

  image.clearData();
  EXPECT_EQ(image.get_img_data(), nullptr);
  EXPECT_FALSE(image.readTiff(""));
}

// Covers back img behavior: reads old style info with slash comments and rejects incomplete info.
TEST(BackImgTest, ReadsOldStyleInfoWithSlashCommentsAndRejectsIncompleteInfo)
{
  TempDir root("old_style_backdrop_info");
  const std::string dir = root.path();
  const std::string info_path = dir + "/commented_old_style.info";
  std::ofstream info(info_path.c_str());
  ASSERT_TRUE(info.is_open());
  info << "// pMarineViewer historical image-center format\n";
  info << "img_centx = 0.25 // datum is one quarter from west edge\n";
  info << "img_centy = 0.75\n";
  info << "img_meters = 20\n";
  info.close();

  ExposedBackImg image;
  ASSERT_TRUE(image.readTiffInfo(info_path));
  EXPECT_DOUBLE_EQ(image.get_img_centx(), 0.25);
  EXPECT_DOUBLE_EQ(image.get_img_centy(), 0.75);
  EXPECT_DOUBLE_EQ(image.get_img_meters(), 20);
  EXPECT_DOUBLE_EQ(image.get_x_at_img_left(), -1.25);
  EXPECT_DOUBLE_EQ(image.get_x_at_img_right(), 3.75);
  EXPECT_DOUBLE_EQ(image.get_y_at_img_bottom(), -3.75);
  EXPECT_DOUBLE_EQ(image.get_y_at_img_top(), 1.25);

  const std::string missing_path = dir + "/missing_meters.info";
  std::ofstream missing(missing_path.c_str());
  ASSERT_TRUE(missing.is_open());
  missing << "img_centx = 0.5\n";
  missing << "img_centy = 0.5\n";
  missing.close();

  ExposedBackImg incomplete;
  EXPECT_FALSE(incomplete.readTiffInfo(missing_path));
}

// Covers back img behavior: reads new style info rejects malformed files and uses env lookup.
TEST(BackImgTest, ReadsNewStyleInfoRejectsMalformedFilesAndUsesEnvLookup)
{
  TempDir root("new_style_backdrop_info");
  const std::string dir = root.path();

  const std::string good_info_path = dir + "/new_style_backdrop.info";
  std::ofstream good_info(good_info_path.c_str());
  ASSERT_TRUE(good_info.is_open());
  good_info << "lat_north = 42.360\n";
  good_info << "lat_south = 42.350\n";
  good_info << "lon_west = -71.000\n";
  good_info << "lon_east = -70.980\n";
  good_info << "datum_lat = 42.358\n";
  good_info << "datum_lon = -70.992\n";
  good_info.close();

  ExposedBackImg good;
  ASSERT_TRUE(good.readTiffInfo(good_info_path));
  EXPECT_GT(good.get_img_mtr_width(), 0u);
  EXPECT_GT(good.get_img_mtr_height(), 0u);
  EXPECT_NE(good.get_x_at_img_ctr(), 0);
  EXPECT_NE(good.get_y_at_img_ctr(), 0);
  EXPECT_LT(good.get_x_at_img_left(), good.get_x_at_img_right());
  EXPECT_LT(good.get_y_at_img_bottom(), good.get_y_at_img_top());

  const std::string bad_key_path = dir + "/bad_key.info";
  std::ofstream bad_key(bad_key_path.c_str());
  ASSERT_TRUE(bad_key.is_open());
  bad_key << "img_centx = 0.5\n";
  bad_key << "img_centy = 0.5\n";
  bad_key << "img_meters = 10\n";
  bad_key << "unexpected = value\n";
  bad_key.close();
  ExposedBackImg bad_key_img;
  EXPECT_FALSE(bad_key_img.readTiffInfo(bad_key_path));

  const std::string bad_boundary_path = dir + "/bad_boundary.info";
  std::ofstream bad_boundary(bad_boundary_path.c_str());
  ASSERT_TRUE(bad_boundary.is_open());
  bad_boundary << "lat_north = 42.350\n";
  bad_boundary << "lat_south = 42.360\n";
  bad_boundary << "lon_west = -71.000\n";
  bad_boundary << "lon_east = -70.980\n";
  bad_boundary.close();
  ExposedBackImg bad_boundary_img;
  EXPECT_FALSE(bad_boundary_img.readTiffInfo(bad_boundary_path));

  const std::string env_tiff = dir + "/env_backdrop.tif";
  const std::string env_info = dir + "/env_backdrop.info";
  {
    std::ofstream tiff(env_tiff.c_str());
    std::ofstream info(env_info.c_str());
    ASSERT_TRUE(tiff.is_open());
    ASSERT_TRUE(info.is_open());
  }
  // IVP_IMAGE_DIRS is process-global, so scope it tightly around the lookup
  // that needs image-directory discovery.
  ScopedEnv image_dirs("IVP_IMAGE_DIRS", dir);
  BackImg locator;
  EXPECT_TRUE(locator.locateTiffAndInfoFiles("env_backdrop.tif"));
  EXPECT_EQ(locator.getTiffFile(), env_tiff);
  EXPECT_EQ(locator.getInfoFile(), env_info);
}
#endif

#ifdef MARINEVIEW_ENABLE_GUI_TESTS
// Covers marine viewer behavior: starts with default viewport state and rejects bad geodesy.
TEST(MarineViewerTest, StartsWithDefaultViewportStateAndRejectsBadGeodesy)
{
  ExposedMarineViewer viewer;

  EXPECT_DOUBLE_EQ(viewer.getZoom(), 1.0);
  EXPECT_DOUBLE_EQ(viewer.getPanX(), 0);
  EXPECT_DOUBLE_EQ(viewer.getPanY(), 0);
  EXPECT_EQ(viewer.getTiffFileCount(), 0u);
  EXPECT_EQ(viewer.getTiffFileCurrent(), "");
  EXPECT_EQ(viewer.getTiffFiles(), std::vector<std::string>{});
  EXPECT_EQ(viewer.getInfoFiles(), std::vector<std::string>{});
  EXPECT_EQ(viewer.getTiffFileA(), "");
  EXPECT_EQ(viewer.getInfoFileA(), "");
  EXPECT_EQ(viewer.getTiffFileB(), "");
  EXPECT_EQ(viewer.getInfoFileB(), "");
  EXPECT_DOUBLE_EQ(viewer.getImgWidthMtrs(), 0);

  EXPECT_FALSE(viewer.initGeodesy(91, -71));
  EXPECT_FALSE(viewer.initGeodesy(42, -181));
  EXPECT_FALSE(viewer.initGeodesy("42.0"));
  EXPECT_FALSE(viewer.initGeodesy("lat,-71"));
  EXPECT_TRUE(viewer.initGeodesy("42.0, -71.0"));
}

// Covers marine viewer behavior: routes parameters to viewer geo vehicle and drop settings.
TEST(MarineViewerTest, RoutesParametersToViewerGeoVehicleAndDropSettings)
{
  ExposedMarineViewer viewer;

  EXPECT_TRUE(viewer.setParam("zoom", 2.0));
  EXPECT_DOUBLE_EQ(viewer.getZoom(), 2.0);
  EXPECT_TRUE(viewer.setParam("zoom", 0.0));
  EXPECT_DOUBLE_EQ(viewer.getZoom(), 0.00001);
  EXPECT_TRUE(viewer.setParam("zoom", "reset"));
  EXPECT_DOUBLE_EQ(viewer.getZoom(), 1.0);

  EXPECT_TRUE(viewer.setParam("pan_x", 15.0));
  EXPECT_TRUE(viewer.setParam("pan_y", -5.0));
  EXPECT_DOUBLE_EQ(viewer.getPanX(), 15);
  EXPECT_DOUBLE_EQ(viewer.getPanY(), -5);
  EXPECT_TRUE(viewer.setParam("set_pan_x", -20.0));
  EXPECT_TRUE(viewer.setParam("set_pan_y", 40.0));
  EXPECT_DOUBLE_EQ(viewer.getPanX(), -20);
  EXPECT_DOUBLE_EQ(viewer.getPanY(), 40);

  EXPECT_TRUE(viewer.setParam("hash_delta", "75"));
  EXPECT_EQ(viewer.geosetting("hash_delta"), "50");
  EXPECT_TRUE(viewer.setParam("hash_delta", "49"));
  EXPECT_EQ(viewer.geosetting("hash_delta"), "10");
  EXPECT_TRUE(viewer.setParam("hash_delta", "999"));
  EXPECT_EQ(viewer.geosetting("hash_delta"), "500");
  EXPECT_TRUE(viewer.setParam("hash_delta", "10000"));
  EXPECT_EQ(viewer.geosetting("hash_delta"), "10000");
  EXPECT_TRUE(viewer.setParam("hash_delta", "1000000"));
  EXPECT_EQ(viewer.geosetting("hash_delta"), "1000000");
  EXPECT_TRUE(viewer.setParam("vehicles_name_mode", "names+mode"));
  EXPECT_EQ(viewer.vehisetting("vehicles_name_mode"), "names+mode");
  EXPECT_TRUE(viewer.setParam("vehicles_name_mode", "toggle"));
  EXPECT_EQ(viewer.vehisetting("vehicles_name_mode"), "names+shortmode");
  EXPECT_TRUE(viewer.setParam("vehicles_name_viewable", "false"));
  EXPECT_EQ(viewer.vehisetting("vehicles_name_mode"), "off");
  EXPECT_TRUE(viewer.setParam("vehicle_names_color", "cyan"));
  EXPECT_EQ(viewer.vehisetting("vehicles_name_color"), "cyan");
  EXPECT_FALSE(viewer.setParam("vehicles_name_color", "not_a_color"));
  EXPECT_TRUE(viewer.setParam("trails_point_size", "bigger"));
  EXPECT_EQ(viewer.vehisetting("trails_point_size"), "1.25000");
  EXPECT_TRUE(viewer.setParam("trails_point_size", "smaller"));
  EXPECT_EQ(viewer.vehisetting("trails_point_size"), "1.00000");
  EXPECT_TRUE(viewer.setParam("trails_length", "longer"));
  EXPECT_EQ(viewer.vehisetting("trails_length"), "125");
  EXPECT_TRUE(viewer.setParam("trails_length", "shorter"));
  EXPECT_EQ(viewer.vehisetting("trails_length"), "100");
  EXPECT_TRUE(viewer.setParam("vehicles_shape_scale", "4"));
  EXPECT_EQ(viewer.vehisetting("vehicles_shape_scale"), "4.00000");
  EXPECT_TRUE(viewer.setParam("vehicle_shape_scale", "reset"));
  EXPECT_EQ(viewer.vehisetting("vehicles_shape_scale"), "1.00000");
  EXPECT_TRUE(viewer.setParam("stale_report_thresh", "1"));
  EXPECT_EQ(viewer.vehisetting("stale_report_thresh"), "2.00000");
  EXPECT_DOUBLE_EQ(viewer.getStaleReportThresh(), 2);
  EXPECT_TRUE(viewer.setParam("stale_remove_thresh", "0"));
  EXPECT_EQ(viewer.vehisetting("stale_remove_thresh"), "0.00000");
  EXPECT_DOUBLE_EQ(viewer.getStaleRemoveThresh(), 0);
  EXPECT_FALSE(viewer.setParam("stale_report_thresh", "-1"));

  EXPECT_TRUE(viewer.setParam("tiff_view", "false"));
  EXPECT_EQ(viewer.geosetting("tiff_view"), "false");
  EXPECT_TRUE(viewer.setParam("tiff_viewable", "toggle"));
  EXPECT_EQ(viewer.geosetting("tiff_viewable"), "true");
  EXPECT_TRUE(viewer.setParam("datum_size", "scale:5"));
  EXPECT_EQ(viewer.geosetting("datum_size"), "10");
  EXPECT_TRUE(viewer.setParam("datum_size", "delta:-50"));
  EXPECT_EQ(viewer.geosetting("datum_size"), "1");
  EXPECT_TRUE(viewer.setParam("grid_opaqueness", "more"));
  EXPECT_EQ(viewer.geosetting("grid_opaqueness"), "0.4");
  EXPECT_TRUE(viewer.setParam("drop_point_coords", "lat-lon"));
  EXPECT_EQ(viewer.geosetting("drop_point_coords"), "lat-lon");
  EXPECT_FALSE(viewer.setParam("datum_color", "not_a_color"));

  EXPECT_FALSE(viewer.setParam("unknown_viewer_param", "true"));
  EXPECT_FALSE(viewer.setParam("zoom", "fast"));
  EXPECT_FALSE(viewer.setParam("set_zoom", "2"));
}

// Covers marine viewer behavior: numeric viewport shade and datum controls clamp like viewer hotkeys.
TEST(MarineViewerTest, NumericViewportShadeAndDatumControlsClampLikeViewerHotkeys)
{
  ExposedMarineViewer viewer;

  EXPECT_TRUE(viewer.setParam("set_zoom", 3.0));
  EXPECT_DOUBLE_EQ(viewer.getZoom(), 3.0);
  EXPECT_TRUE(viewer.setParam("set_zoom", -10.0));
  EXPECT_DOUBLE_EQ(viewer.getZoom(), 0.00001);

  EXPECT_TRUE(viewer.setParam("hash_shade", 2.0));
  EXPECT_DOUBLE_EQ(viewer.hashShade(), 1.0);
  EXPECT_TRUE(viewer.setParam("hash_shade_mod", -5.0));
  EXPECT_DOUBLE_EQ(viewer.hashShade(), 0.0);
  EXPECT_TRUE(viewer.setParam("hash_shade_mod", 0.25));
  EXPECT_DOUBLE_EQ(viewer.hashShade(), 0.25);

  EXPECT_TRUE(viewer.setParam("back_shade", 0.75));
  EXPECT_DOUBLE_EQ(viewer.fillShade(), 0.75);
  EXPECT_TRUE(viewer.setParam("back_shade", 2.0));
  EXPECT_DOUBLE_EQ(viewer.fillShade(), 0.75);
  EXPECT_TRUE(viewer.setParam("tiff_viewable", "false"));
  EXPECT_TRUE(viewer.setParam("back_shade_delta", 0.20));
  EXPECT_DOUBLE_EQ(viewer.fillShade(), 0.95);
  EXPECT_TRUE(viewer.setParam("back_shade_delta", 0.20));
  EXPECT_DOUBLE_EQ(viewer.fillShade(), 0.95);
  EXPECT_TRUE(viewer.setParam("tiff_viewable", "true"));
  EXPECT_TRUE(viewer.setParam("back_shade_delta", -0.20));
  EXPECT_DOUBLE_EQ(viewer.fillShade(), 0.95);

  EXPECT_TRUE(viewer.initGeodesy(42.0, -71.0));
  EXPECT_TRUE(viewer.setParam("tiff_file", "null.tif"));
  const double x_before = viewer.img2meters('x', 0.5);
  EXPECT_TRUE(viewer.setParam("datum", "42.0005,-71.0005"));
  EXPECT_NE(viewer.img2meters('x', 0.5), x_before);
  EXPECT_FALSE(viewer.setParam("datum", "42.0"));
  EXPECT_FALSE(viewer.setParam("datum", "north,-71"));
}

// Covers marine viewer behavior: queues TIFF files and handles null backdrop after geodesy init.
TEST(MarineViewerTest, QueuesTiffFilesAndHandlesNullBackdropAfterGeodesyInit)
{
  ExposedMarineViewer viewer;

  EXPECT_FALSE(viewer.setParam("tiff_file", "mission backdrop.tif"));
  EXPECT_FALSE(viewer.setParam("tiff_file", "=bad.tif"));
  EXPECT_FALSE(viewer.setParam("tiff_file", "mission.png"));
  EXPECT_TRUE(viewer.setParam("tiff_file", "alpha.tif"));
  EXPECT_TRUE(viewer.setParam("tiff_file_b", "bravo.tif"));
  EXPECT_EQ(viewer.getTiffFileCount(), 2u);
  EXPECT_EQ(viewer.getTiffFileCurrent(), "alpha.tif");
  EXPECT_EQ(viewer.getTiffFiles(),
            (std::vector<std::string>{"alpha.tif", "bravo.tif"}));

  EXPECT_FALSE(viewer.setParam("tiff_type", "next"));
  EXPECT_FALSE(viewer.setParam("tiff_type", "toggle"));
  EXPECT_EQ(viewer.getTiffFileCurrent(), "alpha.tif");
  EXPECT_FALSE(viewer.setParam("tiff_file", "null.tif"));

  ExposedMarineViewer null_viewer;
  ASSERT_TRUE(null_viewer.initGeodesy(42.0, -71.0));
  EXPECT_TRUE(null_viewer.setParam("tiff_file", "null.tif"));
  EXPECT_EQ(null_viewer.getTiffFileCount(), 1u);
  EXPECT_EQ(null_viewer.getTiffFileCurrent(), "null.tif");
  EXPECT_EQ(null_viewer.getImgWidthMtrs(), 0);
  EXPECT_NEAR(null_viewer.meters2img('x', 0), 0.5, 1e-9);
  EXPECT_FALSE(null_viewer.applyTiffFiles());
}

// Covers marine viewer behavior: converts between meters image fractions and view pixels.
TEST(MarineViewerTest, ConvertsBetweenMetersImageFractionsAndViewPixels)
{
  ExposedMarineViewer viewer;
  ASSERT_TRUE(viewer.initGeodesy(42.0, -71.0));
  ASSERT_TRUE(viewer.setParam("tiff_file", "null.tif"));

  EXPECT_NEAR(viewer.meters2img('x', 0), 0.5, 1e-9);
  EXPECT_NEAR(viewer.meters2img('y', 0), 0.5, 1e-9);
  EXPECT_NEAR(viewer.img2meters('x', 0.5), 0, 1e-9);
  EXPECT_NEAR(viewer.img2meters('y', 0.5), 0, 1e-9);
  EXPECT_DOUBLE_EQ(viewer.meters2img('z', 10), 0);
  EXPECT_DOUBLE_EQ(viewer.img2meters('z', 0.5), 0);

  EXPECT_NEAR(viewer.img2view('x', 0.5), 700, 1e-9);
  EXPECT_NEAR(viewer.img2view('y', 0.5), 650, 1e-9);
  EXPECT_NEAR(viewer.view2img('x', 700), 0.5, 1e-9);
  EXPECT_NEAR(viewer.view2img('y', 650), 0.5, 1e-9);

  EXPECT_FALSE(viewer.coordInView(0, 0));
  EXPECT_FALSE(viewer.coordInView(10000, 10000));
  EXPECT_TRUE(viewer.coordInViewX(10, 10));
  EXPECT_FALSE(viewer.coordInViewX(-1, 10));
  EXPECT_FALSE(viewer.coordInViewX(401, 10));

  viewer.setCtrView(100, -50);
  EXPECT_LT(viewer.getPanX(), 0);
  EXPECT_GT(viewer.getPanY(), 0);
  const double pan_x_before = viewer.getPanX();
  const double pan_y_before = viewer.getPanY();
  viewer.setAutoZoom(0, 0);
  viewer.autoZoom();
  EXPECT_NEAR(viewer.getPanX(), pan_x_before * 0.92, 1e-6);
  EXPECT_NEAR(viewer.getPanY(), pan_y_before * 0.92, 1e-6);
}

// Covers marine GUI behavior: base menu starts with back view items and requires viewer for sync.
TEST(MarineGUITest, BaseMenuStartsWithBackViewItemsAndRequiresViewerForSync)
{
  MarineGUI gui(640, 480, "base-marine-gui");

  EXPECT_FALSE(gui.setMenuAttrib("BackView", "hash_viewable", "true"));
  EXPECT_TRUE(gui.removeMenuItem("BackView/Zoom In"));
  EXPECT_FALSE(gui.removeMenuItem("BackView/Zoom In"));
  EXPECT_FALSE(gui.removeMenuItem("BackView/Missing Item"));
  EXPECT_EQ(gui.handle(FL_KEYDOWN), 1);
}

// Covers marine GUI behavior: custom FLTK widgets keep viewer keyboard focus stable.
TEST(MarineGUITest, CustomFltkWidgetsKeepViewerKeyboardFocusStable)
{
  MY_Output output(0, 0, 80, 20, "read-only-output");
  EXPECT_EQ(output.handle(FL_PUSH), 0);
  EXPECT_EQ(output.handle(FL_KEYBOARD), 0);
  EXPECT_EQ(output.handle(FL_RELEASE), 0);

  MY_Fl_Hold_Browser browser(10, 20, 140, 90, "contact-browser");
  EXPECT_EQ(browser.textfont(), FL_COURIER);
  EXPECT_EQ(browser.box(), FL_DOWN_BOX);
  EXPECT_EQ(browser.textcolor(), FL_BLACK);
  EXPECT_EQ(browser.handle(FL_KEYBOARD), 0);
  EXPECT_EQ(browser.handle(FL_MOVE), 0);
}

// Covers marine vehi GUI behavior: adds geo and vehicle menus for viewer controls.
TEST(MarineVehiGUITest, AddsGeoAndVehicleMenusForViewerControls)
{
  ExposedMarineVehiGUI gui;

  EXPECT_TRUE(gui.hasMenuItem("GeoAttr/Polygons/polygon_viewable_all=true"));
  EXPECT_TRUE(gui.hasMenuItem("GeoAttr/SegLists/seglist_viewable_labels=false"));
  EXPECT_TRUE(gui.hasMenuItem("GeoAttr/DropPoints/drop_point_coords=local-grid"));
  EXPECT_TRUE(gui.hasMenuItem("Vehicles/vehicles_viewable=true"));
  EXPECT_TRUE(gui.hasMenuItem("Vehicles/vehicles_name_mode=names+shortmode"));
  EXPECT_TRUE(gui.hasMenuItem("Vehicles/trails_color=green"));
  EXPECT_TRUE(gui.hasMenuItem("Vehicles/bearing_lines_viewable=false"));
  EXPECT_FALSE(gui.hasMenuItem("Vehicles/missing"));
}

// Covers marine vehi GUI behavior: syncs geometry radio menus through marine viewer settings.
TEST(MarineVehiGUITest, SyncsGeometryRadioMenusThroughMarineViewerSettings)
{
  ExposedMarineVehiGUI gui;

  EXPECT_TRUE(gui.setMenuAttrib("GeoAttr/Polygons", "polygon_viewable_all", "false"));
  EXPECT_EQ(gui.viewerGeoSetting("polygon_viewable_all"), "false");
  EXPECT_TRUE(gui.menuItemSelected("GeoAttr/Polygons/polygon_viewable_all=false"));

  EXPECT_TRUE(gui.setMenuAttrib("GeoAttr/Polygons", "polygon_viewable_all", "toggle"));
  EXPECT_EQ(gui.viewerGeoSetting("polygon_viewable_all"), "true");
  EXPECT_TRUE(gui.menuItemSelected("GeoAttr/Polygons/polygon_viewable_all=true"));

  EXPECT_TRUE(gui.setMenuAttrib("GeoAttr/Datum", "datum_size", "4"));
  EXPECT_EQ(gui.viewerGeoSetting("datum_size"), "4");
  EXPECT_TRUE(gui.menuItemSelected("GeoAttr/Datum/datum_size=4"));

  EXPECT_FALSE(gui.setMenuAttrib("GeoAttr/Datum", "datum_size", "huge"));
  EXPECT_FALSE(gui.setMenuAttrib("GeoAttr/Polygons", "unknown_attr", "true"));
  gui.updateRadios();
  gui.setMenuItemColors();
}

// Covers marine vehi GUI behavior: syncs vehicle radio menus through marine viewer settings.
TEST(MarineVehiGUITest, SyncsVehicleRadioMenusThroughMarineViewerSettings)
{
  ExposedMarineVehiGUI gui;

  EXPECT_TRUE(gui.setRadioVehiAttrib("vehicles_viewable", "false"));
  EXPECT_EQ(gui.viewerVehiSetting("vehicles_viewable"), "false");
  EXPECT_TRUE(gui.menuItemSelected("Vehicles/vehicles_viewable=false"));

  EXPECT_TRUE(gui.setRadioVehiAttrib("vehicles_viewable", "toggle"));
  EXPECT_EQ(gui.viewerVehiSetting("vehicles_viewable"), "true");
  EXPECT_TRUE(gui.menuItemSelected("Vehicles/vehicles_viewable=true"));

  EXPECT_TRUE(gui.setRadioVehiAttrib("vehicles_name_mode", "names+depth"));
  EXPECT_EQ(gui.viewerVehiSetting("vehicles_name_mode"), "names+depth");
  EXPECT_TRUE(gui.menuItemSelected("Vehicles/vehicles_name_mode=names+depth"));

  EXPECT_TRUE(gui.setRadioVehiAttrib("vehicles_active_color", "green"));
  EXPECT_EQ(gui.viewerVehiSetting("vehicles_active_color"), "green");
  EXPECT_TRUE(gui.menuItemSelected("Vehicles/vehicles_active_color=green"));

  EXPECT_TRUE(gui.setRadioVehiAttrib("trails_color", "yellow"));
  EXPECT_EQ(gui.viewerVehiSetting("trails_color"), "yellow");
  EXPECT_TRUE(gui.menuItemSelected("Vehicles/trails_color=yellow"));

  EXPECT_FALSE(gui.setRadioVehiAttrib("vehicles_name_mode", "names+unknown"));
  EXPECT_FALSE(gui.setRadioVehiAttrib("unknown_vehicle_attr", "true"));
  gui.updateRadios();
  gui.setMenuItemColors();
}

// Covers marine vehi GUI behavior: syncs additional vehicle radio menus.
TEST(MarineVehiGUITest, SyncsAdditionalVehicleRadioMenus)
{
  ExposedMarineVehiGUI gui;

  EXPECT_TRUE(gui.setRadioVehiAttrib("trails_connect_viewable", "true"));
  EXPECT_EQ(gui.viewerVehiSetting("trails_connect_viewable"), "true");
  EXPECT_TRUE(gui.menuItemSelected("Vehicles/trails_connect_viewable=true"));
  EXPECT_TRUE(gui.setRadioVehiAttrib("trails_connect_viewable", "toggle"));
  EXPECT_EQ(gui.viewerVehiSetting("trails_connect_viewable"), "false");
  EXPECT_TRUE(gui.menuItemSelected("Vehicles/trails_connect_viewable=false"));

  EXPECT_TRUE(gui.setRadioVehiAttrib("bearing_lines_viewable", "toggle"));
  EXPECT_EQ(gui.viewerVehiSetting("bearing_lines_viewable"), "false");
  EXPECT_TRUE(gui.menuItemSelected("Vehicles/bearing_lines_viewable=false"));
  EXPECT_TRUE(gui.setRadioVehiAttrib("vehicles_inactive_color", "blue"));
  EXPECT_EQ(gui.viewerVehiSetting("vehicles_inactive_color"), "blue");
  EXPECT_TRUE(gui.menuItemSelected("Vehicles/vehicles_inactive_color=blue"));
}

// Covers marine vehi GUI behavior: syncs back view and high volume geometry menus.
TEST(MarineVehiGUITest, SyncsBackViewAndHighVolumeGeometryMenus)
{
  ExposedMarineVehiGUI gui;

  EXPECT_TRUE(gui.setMenuAttrib("BackView", "hash_viewable", "true"));
  EXPECT_EQ(gui.viewerGeoSetting("hash_viewable"), "true");
  EXPECT_TRUE(gui.menuItemSelected("BackView/hash_viewable=true"));
  EXPECT_TRUE(gui.setMenuAttrib("BackView", "hash_viewable", "toggle"));
  EXPECT_EQ(gui.viewerGeoSetting("hash_viewable"), "false");
  EXPECT_TRUE(gui.menuItemSelected("BackView/hash_viewable=false"));

  EXPECT_TRUE(gui.setMenuAttrib("BackView", "tiff_viewable", "false"));
  EXPECT_EQ(gui.viewerGeoSetting("tiff_viewable"), "false");
  EXPECT_TRUE(gui.menuItemSelected("BackView/tiff_viewable=false"));

  EXPECT_TRUE(gui.setMenuAttrib("GeoAttr/Vectors", "vector_viewable_all", "false"));
  EXPECT_EQ(gui.viewerGeoSetting("vector_viewable_all"), "false");
  EXPECT_TRUE(gui.menuItemSelected("GeoAttr/Vectors/vector_viewable_all=false"));
  EXPECT_TRUE(gui.setMenuAttrib("GeoAttr/Circles", "circle_viewable_labels", "false"));
  EXPECT_EQ(gui.viewerGeoSetting("circle_viewable_labels"), "false");
  EXPECT_TRUE(gui.menuItemSelected("GeoAttr/Circles/circle_viewable_labels=false"));
  EXPECT_TRUE(gui.setMenuAttrib("GeoAttr/Markers", "marker_scale", "scale:1.1"));
  EXPECT_EQ(gui.viewerGeoSetting("marker_scale"), "");
  EXPECT_TRUE(gui.setMenuAttrib("GeoAttr/DropPoints", "drop_point_vertex_size", "10"));
  EXPECT_EQ(gui.viewerGeoSetting("drop_point_vertex_size"), "10");
  EXPECT_TRUE(gui.menuItemSelected("GeoAttr/DropPoints/drop_point_vertex_size=10"));

  EXPECT_TRUE(gui.setMenuAttrib("GeoAttr/RangePulses",
                                "range_pulse_viewable_all", "false"));
  EXPECT_EQ(gui.viewerGeoSetting("range_pulse_viewable_all"), "false");
  EXPECT_TRUE(gui.setMenuAttrib("GeoAttr/CommsPulses",
                                "comms_pulse_viewable_all", "false"));
  EXPECT_EQ(gui.viewerGeoSetting("comms_pulse_viewable_all"), "false");
  EXPECT_TRUE(gui.setMenuAttrib("GeoAttr/NodePulses",
                                "node_pulse_viewable_all", "false"));
  EXPECT_EQ(gui.viewerGeoSetting("node_pulse_viewable_all"), "false");
}
#endif

#ifdef MARINEVIEW_ENABLE_CORE_TESTS
// Covers op area spec behavior: starts with visible defaults and out of range fallbacks.
TEST(OpAreaSpecTest, StartsWithVisibleDefaultsAndOutOfRangeFallbacks)
{
  OpAreaSpec spec;

  EXPECT_TRUE(spec.viewable());
  EXPECT_TRUE(spec.viewable("ALL"));
  EXPECT_TRUE(spec.viewable("labels"));
  EXPECT_FALSE(spec.viewable("vertices"));
  EXPECT_DOUBLE_EQ(spec.getLineShade(), 1.0);
  EXPECT_DOUBLE_EQ(spec.getLabelShade(), 1.0);
  EXPECT_EQ(spec.size(), 0u);

  EXPECT_DOUBLE_EQ(spec.getXPos(0), 0);
  EXPECT_DOUBLE_EQ(spec.getYPos(0), 0);
  EXPECT_DOUBLE_EQ(spec.getLWidth(0), 0);
  EXPECT_EQ(spec.getGroup(0), "");
  EXPECT_EQ(spec.getLabel(0), "");
  EXPECT_FALSE(spec.getDashed(0));
  EXPECT_FALSE(spec.getLooped(0));
  expectColorNear(spec.getLColor(0), 0.5, 0.5, 0.5);
  expectColorNear(spec.getVColor(0), 0.5, 0.5, 0.5);
}

// Covers op area spec behavior: adds local XY vertices with viewer style metadata.
TEST(OpAreaSpecTest, AddsLocalXYVerticesWithViewerStyleMetadata)
{
  CMOOSGeodesy geodesy;
  OpAreaSpec spec;

  EXPECT_TRUE(spec.addVertex("x=10, y=-5, lwidth=0.25, group=alpha, "
                             "label=gate_start, lcolor=red, vcolor=green, "
                             "dashed=true, looped=true",
                             geodesy));
  EXPECT_TRUE(spec.addVertex("xpos=20,ypos=30,lwidth=3,group=alpha,"
                             "label=gate_end,lcolor=blue",
                             geodesy));

  ASSERT_EQ(spec.size(), 2u);
  EXPECT_DOUBLE_EQ(spec.getXPos(0), 10);
  EXPECT_DOUBLE_EQ(spec.getYPos(0), -5);
  EXPECT_DOUBLE_EQ(spec.getLWidth(0), 1);
  EXPECT_EQ(spec.getGroup(0), "alpha");
  EXPECT_EQ(spec.getLabel(0), "gate_start");
  EXPECT_TRUE(spec.getDashed(0));
  EXPECT_TRUE(spec.getLooped(0));
  expectColorNear(spec.getLColor(0), 1, 0, 0);
  expectColorNear(spec.getVColor(0), 0, 128.0 / 255.0, 0);

  EXPECT_DOUBLE_EQ(spec.getXPos(1), 20);
  EXPECT_DOUBLE_EQ(spec.getYPos(1), 30);
  EXPECT_DOUBLE_EQ(spec.getLWidth(1), 3);
  EXPECT_FALSE(spec.getDashed(1));
  EXPECT_FALSE(spec.getLooped(1));
  expectColorNear(spec.getLColor(1), 0, 0, 1);
  expectColorNear(spec.getVColor(1), 0, 0, 1);
}

// Covers op area spec behavior: rejects malformed vertices without changing stored geometry.
TEST(OpAreaSpecTest, RejectsMalformedVerticesWithoutChangingStoredGeometry)
{
  CMOOSGeodesy geodesy;
  OpAreaSpec spec;
  ASSERT_TRUE(spec.addVertex("x=1,y=2,lwidth=2,label=ok", geodesy));

  EXPECT_FALSE(spec.addVertex("x=1,y=2,badtoken", geodesy));
  EXPECT_FALSE(spec.addVertex("x=1", geodesy));
  EXPECT_FALSE(spec.addVertex("x=1,y=abc", geodesy));
  EXPECT_FALSE(spec.addVertex("x=1,y=2,lwidth=-1", geodesy));
  EXPECT_FALSE(spec.addVertex("lat=42.0", geodesy));

  ASSERT_EQ(spec.size(), 1u);
  EXPECT_DOUBLE_EQ(spec.getXPos(0), 1);
  EXPECT_DOUBLE_EQ(spec.getYPos(0), 2);
}

// Covers op area spec behavior: converts lat lon vertices using configured geodesy.
TEST(OpAreaSpecTest, ConvertsLatLonVerticesUsingConfiguredGeodesy)
{
  CMOOSGeodesy geodesy;
  ASSERT_TRUE(geodesy.Initialise(42.0, -71.0));
  OpAreaSpec spec;

  ASSERT_TRUE(spec.addVertex("lat=42.0,lon=-71.0,label=datum", geodesy));
  ASSERT_TRUE(spec.addVertex("lat=42.0001,lon=-70.9999,group=survey",
                             geodesy));
  ASSERT_EQ(spec.size(), 2u);
  EXPECT_NEAR(spec.getXPos(0), 0, 1e-5);
  EXPECT_NEAR(spec.getYPos(0), 0, 1e-5);
  EXPECT_GT(spec.getXPos(1), 0);
  EXPECT_GT(spec.getYPos(1), 0);
  EXPECT_EQ(spec.getLabel(0), "datum");
  EXPECT_EQ(spec.getGroup(1), "survey");
}

// Covers op area spec behavior: view toggles and shade mods clamp for viewer controls.
TEST(OpAreaSpecTest, ViewTogglesAndShadeModsClampForViewerControls)
{
  OpAreaSpec spec;

  EXPECT_TRUE(spec.setParam("op_area_viewable_all", "false"));
  EXPECT_TRUE(spec.setParam("op_area_viewable_labels", "false"));
  EXPECT_FALSE(spec.viewable("all"));
  EXPECT_FALSE(spec.viewable("labels"));

  EXPECT_TRUE(spec.setParam("op_area_viewable_all", "true"));
  EXPECT_TRUE(spec.setParam("op_area_viewable_labels", "true"));
  EXPECT_TRUE(spec.viewable("all"));
  EXPECT_TRUE(spec.viewable("labels"));

  EXPECT_TRUE(spec.setParam("op_area_line_shade", "1.5"));
  EXPECT_DOUBLE_EQ(spec.getLineShade(), 1.0);
  EXPECT_TRUE(spec.setParam("op_area_line_shade_mod", "0.25"));
  EXPECT_DOUBLE_EQ(spec.getLineShade(), 0.25);
  EXPECT_TRUE(spec.setParam("op_area_line_shade_mod", "10"));
  EXPECT_DOUBLE_EQ(spec.getLineShade(), 1.0);

  EXPECT_TRUE(spec.setParam("op_area_label_shade", "-2"));
  EXPECT_DOUBLE_EQ(spec.getLabelShade(), 0.0);
  EXPECT_TRUE(spec.setParam("op_area_label_shade", "0.8"));
  EXPECT_DOUBLE_EQ(spec.getLabelShade(), 0.8);
  EXPECT_TRUE(spec.setParam("op_area_label_shade_mod", "0.5"));
  EXPECT_DOUBLE_EQ(spec.getLabelShade(), 0.4);

  EXPECT_FALSE(spec.setParam("op_area_line_shade", "bright"));
  EXPECT_DOUBLE_EQ(spec.getLineShade(), 1.0);
  EXPECT_FALSE(spec.setParam("op_area_label_shade_mod", "dim"));
  EXPECT_DOUBLE_EQ(spec.getLabelShade(), 0.4);
  EXPECT_FALSE(spec.setParam("op_area_viewable_all", "1"));
  EXPECT_TRUE(spec.viewable("all"));
  EXPECT_FALSE(spec.setParam("unknown", "true"));
}

// Covers op area spec behavior: pins boolean case sensitivity and color fallbacks.
TEST(OpAreaSpecTest, PinsBooleanCaseSensitivityAndColorFallbacks)
{
  CMOOSGeodesy geodesy;
  OpAreaSpec spec;

  EXPECT_TRUE(spec.addVertex("x=0,y=0,lcolor=not_a_color,vcolor=cyan",
                             geodesy));
  EXPECT_EQ(spec.size(), 1u);
  expectColorNear(spec.getLColor(0), 0, 0, 0);
  expectColorNear(spec.getVColor(0), 0, 1, 1);

  EXPECT_TRUE(spec.addVertex("x=1,y=2,dashed=TRUE,looped=True", geodesy));
  ASSERT_EQ(spec.size(), 2u);
  EXPECT_TRUE(spec.getDashed(1));
  EXPECT_TRUE(spec.getLooped(1));

  EXPECT_TRUE(spec.setParam("op_area_viewable_all", "TRUE"));
  EXPECT_TRUE(spec.viewable("all"));
  EXPECT_TRUE(spec.setParam("op_area_line_shade", "0.5"));
  EXPECT_TRUE(spec.setParam("op_area_line_shade_mod", "-2"));
  EXPECT_DOUBLE_EQ(spec.getLineShade(), 0.0);
}

// Covers vehicle set behavior: pins control param whitespace and numeric case fallback.
TEST(VehicleSetTest, PinsControlParamWhitespaceAndNumericCaseFallback)
{
  VehicleSet vehicles;
  std::string whynot;

  ASSERT_TRUE(vehicles.handleNodeReport(
      10, nodeReport("ABE", "heron", 1, 2, 3, 45, 10,
                     "MODE=LOITER,MODE_AUX=STATION,ALLSTOP=clear"),
      whynot));

  EXPECT_FALSE(vehicles.setParam(" active_vehicle_name ", " ABE "));
  EXPECT_TRUE(vehicles.setParam("active_vehicle_name", " ABE "));
  EXPECT_EQ(vehicles.getActiveVehicle(), "ABE");
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("abe", "xpos"), 1);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("ABE", "meters_y"), 2);
  EXPECT_EQ(vehicles.getStringInfo("ABE", "helm_mode_aux"), "");
  EXPECT_EQ(vehicles.getStringInfo("active_vehicle_name"), "ABE");
  EXPECT_EQ(vehicles.getClosestVehicle(100, 100), "ABE");
}

// Covers vehicle set behavior: rejects incomplete or unsafe NODE_REPORTs.
TEST(VehicleSetTest, RejectsIncompleteOrUnsafeNodeReports)
{
  VehicleSet vehicles;
  std::string whynot;

  EXPECT_FALSE(vehicles.handleNodeReport(
      "NAME=abe,TYPE=kayak,TIME=1,SPD=1,HDG=90", whynot));
  EXPECT_EQ(whynot, "must have either x,y or lat,lon");

  whynot.clear();
  EXPECT_FALSE(vehicles.handleNodeReport(
      "NAME=abe,TYPE=kayak,TIME=1,X=0,Y=0,HDG=90", whynot));
  EXPECT_EQ(whynot, "missing:speed");

  whynot.clear();
  EXPECT_FALSE(vehicles.handleNodeReport(
      "NAME=abe,TYPE=uuv,TIME=1,X=0,Y=0,SPD=1,HDG=90", whynot));
  EXPECT_EQ(whynot, "underwater vehicle type with no depth reported");
}

// Covers vehicle set behavior: stores NODE_REPORTs normalizes types and defaults lengths.
TEST(VehicleSetTest, StoresNodeReportsNormalizesTypesAndDefaultsLengths)
{
  VehicleSet vehicles;
  std::string whynot;

  EXPECT_TRUE(vehicles.handleNodeReport(
      10, nodeReport("abe", "KAYAK", 0, 0, 2, 90, 10,
                     "MODE=TRANSIT,GROUP=alpha,ALLSTOP=false"),
      whynot));
  EXPECT_TRUE(vehicles.handleNodeReport(
      11, nodeReport("ben", "uuv", 10, 0, 1, 180, 11, "DEPTH=4"),
      whynot));
  EXPECT_TRUE(vehicles.handleNodeReport(
      12, nodeReport("cal", "unknown_robot", -5, 5, 0.5, 270, 12),
      whynot));

  EXPECT_EQ(vehicles.getVehiNames(),
            (std::vector<std::string>{"abe", "ben", "cal"}));
  EXPECT_EQ(vehicles.getActiveVehicle(), "abe");
  EXPECT_EQ(vehicles.getClosestVehicle(9, 1), "ben");

  EXPECT_EQ(vehicles.getStringInfo("abe", "type"), "kayak");
  EXPECT_EQ(vehicles.getStringInfo("ben", "type"), "auv");
  EXPECT_EQ(vehicles.getStringInfo("cal", "type"), "ship");
  EXPECT_EQ(vehicles.getStringInfo("abe", "helm_mode"), "TRANSIT");
  EXPECT_EQ(vehicles.getStringInfo("abe", "helm_allstop_mode"), "false");
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("abe", "vlength"), 3);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("ben", "vlength"), 3);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("cal", "vlength"), 10);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("ben", "depth"), 4);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("abe", "course"), 90);
  EXPECT_EQ(vehicles.getStringInfo("abe", "helm_mode_aux"), "");
  double center_x = 0;
  double center_y = 0;
  EXPECT_FALSE(vehicles.getStringInfo("abe", "unknown_info", whynot));
  EXPECT_FALSE(vehicles.getDoubleInfo("abe", "unknown_info", center_x));

  ASSERT_TRUE(vehicles.getWeightedCenter(center_x, center_y));
  EXPECT_NEAR(center_x, 5.0 / 3.0, 1e-9);
  EXPECT_NEAR(center_y, 5.0 / 3.0, 1e-9);
}

// Covers vehicle set behavior: pins viewer body defaults for additional MOOS vehicle types.
TEST(VehicleSetTest, PinsViewerBodyDefaultsForAdditionalMoosVehicleTypes)
{
  VehicleSet vehicles;
  std::string whynot;

  ASSERT_TRUE(vehicles.handleNodeReport(
      1, nodeReport("long", "longship", 0, 0, 0, 0, 1), whynot));
  ASSERT_TRUE(vehicles.handleNodeReport(
      2, nodeReport("sam", "smr", 1, 0, 0, 0, 2), whynot));
  ASSERT_TRUE(vehicles.handleNodeReport(
      3, nodeReport("mok", "mokai", 2, 0, 0, 0, 3), whynot));
  ASSERT_TRUE(vehicles.handleNodeReport(
      4, nodeReport("kay", "kayak", 3, 0, 0, 0, 4, "LENGTH=4.2"),
      whynot));
  ASSERT_TRUE(vehicles.handleNodeReport(
      5, nodeReport("slo", "slocum", 4, 0, 0, 0, 5, "DEPTH=8"),
      whynot));

  EXPECT_EQ(vehicles.getStringInfo("long", "type"), "longship");
  EXPECT_EQ(vehicles.getStringInfo("sam", "type"), "smr");
  EXPECT_EQ(vehicles.getStringInfo("mok", "type"), "mokai");
  EXPECT_EQ(vehicles.getStringInfo("slo", "type"), "glider");
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("long", "vlength"), 10);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("sam", "vlength"), 6);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("mok", "vlength"), 0);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("kay", "vlength"), 4.2);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("slo", "vlength"), 2);
}

// Covers vehicle set behavior: tracks active center age and extrapolated viewer positions.
TEST(VehicleSetTest, TracksActiveCenterAgeAndExtrapolatedViewerPositions)
{
  VehicleSet vehicles;
  std::string whynot;
  ASSERT_TRUE(vehicles.handleNodeReport(
      10, nodeReport("abe", "kayak", 0, 0, 2, 90, 10), whynot));
  ASSERT_TRUE(vehicles.handleNodeReport(
      11, nodeReport("ben", "ship", 10, -5, 1, 0, 11, "LENGTH=20"), whynot));

  EXPECT_TRUE(vehicles.setParam("curr_time", 12.0));
  EXPECT_FALSE(vehicles.setParam("curr_time", 9.0));
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("curr_time"), 12);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("abe", "age_ais"), 2);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("ben", "age_ais"), 1);

  EXPECT_TRUE(vehicles.setParam("active_vehicle_name", "ben"));
  EXPECT_FALSE(vehicles.setParam("active_vehicle_name", "missing"));
  EXPECT_EQ(vehicles.getActiveVehicle(), "ben");
  EXPECT_EQ(vehicles.getStringInfo("active_vehicle_name"), "ben");
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("speed"), 1);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("active", "vlength"), 20);

  EXPECT_FALSE(vehicles.setParam("center_vehicle_name", "abe"));
  EXPECT_EQ(vehicles.getCenterVehicle(), "abe");
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("center_vehicle", "heading"), 90);

  EXPECT_TRUE(vehicles.setParam("cycle_active"));
  EXPECT_EQ(vehicles.getActiveVehicle(), "abe");

  double extrapolated_x = 0;
  ASSERT_TRUE(vehicles.getDoubleInfo("abe", "xpos", extrapolated_x));
  EXPECT_NEAR(extrapolated_x, 4.0, 1e-9);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("unknown", "speed"), 0);
  EXPECT_EQ(vehicles.getStringInfo("unknown", "type"), "");
}

// Covers vehicle set behavior: supports legacy active aliases and clear all sentinel.
TEST(VehicleSetTest, SupportsLegacyActiveAliasesAndClearAllSentinel)
{
  VehicleSet vehicles;
  std::string whynot;
  EXPECT_TRUE(vehicles.setParam("cycle_active"));

  ASSERT_TRUE(vehicles.handleNodeReport(
      1, nodeReport("abe", "kayak", 0, 0, 1, 0, 1), whynot));
  ASSERT_TRUE(vehicles.handleNodeReport(
      2, nodeReport("ben", "ship", 10, 0, 1, 0, 2), whynot));

  EXPECT_TRUE(vehicles.setParam("vehicles_active_name", "ben"));
  EXPECT_EQ(vehicles.getActiveVehicle(), "ben");
  EXPECT_FALSE(vehicles.setParam("vehicles_center_name", "abe"));
  EXPECT_EQ(vehicles.getCenterVehicle(), "abe");
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("center_vehicle", "meters_y"), 0);

  vehicles.clear("missing");
  EXPECT_EQ(vehicles.getVehiNames(), (std::vector<std::string>{"abe", "ben"}));
  vehicles.clear("all");
  EXPECT_TRUE(vehicles.getVehiNames().empty());
  double center_x = 5;
  double center_y = 5;
  EXPECT_FALSE(vehicles.getWeightedCenter(center_x, center_y));
  EXPECT_DOUBLE_EQ(center_x, 0);
  EXPECT_DOUBLE_EQ(center_y, 0);
}

// Covers vehicle set behavior: maintains track history and supports targeted clears.
TEST(VehicleSetTest, MaintainsTrackHistoryAndSupportsTargetedClears)
{
  VehicleSet vehicles;
  std::string whynot;
  double x = 99;
  double y = 99;
  EXPECT_FALSE(vehicles.getWeightedCenter(x, y));
  EXPECT_DOUBLE_EQ(x, 0);
  EXPECT_DOUBLE_EQ(y, 0);

  ASSERT_TRUE(vehicles.handleNodeReport(
      1, nodeReport("abe", "kayak", 0, 0, 1, 45, 1), whynot));
  ASSERT_TRUE(vehicles.handleNodeReport(
      2, nodeReport("abe", "kayak", 1, 1, 1, 45, 2), whynot));
  ASSERT_TRUE(vehicles.handleNodeReport(
      3, nodeReport("ben", "ship", 5, 5, 0, 0, 3), whynot));

  EXPECT_EQ(vehicles.getVehiHist("abe").size(), 2u);
  EXPECT_EQ(vehicles.getVehiHist("active").size(), 2u);
  EXPECT_EQ(vehicles.getVehiHist("ben").size(), 1u);
  EXPECT_TRUE(vehicles.getVehiHist("missing").empty());

  vehicles.clear("abe");
  EXPECT_EQ(vehicles.getVehiNames(), (std::vector<std::string>{"ben"}));
  EXPECT_TRUE(vehicles.getVehiHist("abe").empty());
  EXPECT_FALSE(vehicles.getVehiHist("ben").empty());

  vehicles.clear();
  EXPECT_TRUE(vehicles.getVehiNames().empty());
  EXPECT_TRUE(vehicles.getVehiHist("ben").empty());
}

// Covers vehicle set behavior: converts lat lon only reports using configured geodesy.
TEST(VehicleSetTest, ConvertsLatLonOnlyReportsUsingConfiguredGeodesy)
{
  CMOOSGeodesy geodesy;
  ASSERT_TRUE(geodesy.Initialise(42.0, -71.0));

  VehicleSet vehicles;
  vehicles.setMOOSGeodesy(geodesy);

  std::string whynot;
  ASSERT_TRUE(vehicles.handleNodeReport(
      100, "NAME=abe,TYPE=kayak,TIME=100,LAT=42.0,LON=-71.0,"
           "SPD=1.5,HDG=45",
      whynot));

  EXPECT_NEAR(vehicles.getDoubleInfo("abe", "xpos"), 0, 1e-6);
  EXPECT_NEAR(vehicles.getDoubleInfo("abe", "ypos"), 0, 1e-6);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("abe", "lat"), 42.0);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("abe", "lon"), -71.0);
}

// Covers vehicle set behavior: accepts JSON reports and pins case lookup differences.
TEST(VehicleSetTest, AcceptsJsonReportsAndPinsCaseLookupDifferences)
{
  VehicleSet vehicles;
  std::string whynot;

  ASSERT_TRUE(vehicles.handleNodeReport(
      20, "{\"NAME\":\"ABE\",\"TYPE\":\"HERON\",\"TIME\":\"20\","
          "\"X\":\"3\",\"Y\":\"4\",\"SPD\":\"1.2\",\"HDG\":\"270\","
          "\"MODE\":\"RETURN\",\"ALLSTOP\":\"clear\"}",
      whynot));

  EXPECT_EQ(vehicles.getVehiNames(), (std::vector<std::string>{"ABE"}));
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("ABE", "speed"), 1.2);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("abe", "speed"), 1.2);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("meters_x"), 3);
  EXPECT_EQ(vehicles.getStringInfo("ABE", "helm_mode"), "RETURN");
  EXPECT_EQ(vehicles.getStringInfo("ABE", "helm_allstop_mode"), "clear");
  EXPECT_EQ(vehicles.getStringInfo("abe", "helm_mode"), "");
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("ABE", "vlength"), 3);
}

// Covers vehicle set behavior: normalizes glider aliases and uses report time when local time is zero.
TEST(VehicleSetTest, NormalizesGliderAliasesAndUsesReportTimeWhenLocalTimeIsZero)
{
  VehicleSet vehicles;
  std::string whynot;

  ASSERT_TRUE(vehicles.handleNodeReport(
      0, nodeReport("gilda", "seaglider", -2, 6, 0.7, 10, 30, "DEPTH=12"),
      whynot));
  ASSERT_TRUE(vehicles.setParam("curr_time", 35.0));

  EXPECT_EQ(vehicles.getStringInfo("gilda", "type"), "glider");
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("gilda", "vlength"), 2);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("gilda", "depth"), 12);
  EXPECT_DOUBLE_EQ(vehicles.getDoubleInfo("gilda", "age_ais"), 5);
}
#endif
