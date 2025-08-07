#include "gtest/gtest.h"
#include "GpxParser.h"
#include <QString>

// Test fixture for GPXParser tests
class GPXParserTest : public ::testing::Test {
protected:
    GPXParser parser;
};

// Test case for successful parsing of basic GPX data
TEST_F(GPXParserTest, BasicParsing) {
    QString gpxData = R"(
        <gpx>
            <trk>
                <trkseg>
                    <trkpt lat="45.0" lon="10.0"><ele>100</ele></trkpt>
                    <trkpt lat="45.1" lon="10.1"><ele>200</ele></trkpt>
                </trkseg>
            </trk>
        </gpx>
    )";

    EXPECT_TRUE(parser.parseData(gpxData));
    EXPECT_EQ(parser.getPoints().size(), 2);
}

// Test case for total distance calculation
TEST_F(GPXParserTest, TotalDistance) {
    QString gpxData = R"(
        <gpx>
            <trk>
                <trkseg>
                    <trkpt lat="0.0" lon="0.0"><ele>10</ele></trkpt>
                    <trkpt lat="0.0" lon="1.0"><ele>12</ele></trkpt>
                </trkseg>
            </trk>
        </gpx>
    )";

    parser.parseData(gpxData);
    // Distance for 1 degree of longitude at the equator is ~111.2 km by WGS84
    EXPECT_NEAR(parser.getTotalDistance(), 111195, 1);
}

// Test case for elevation gain
TEST_F(GPXParserTest, ElevationGain) {
    QString gpxData = R"(
        <gpx>
            <trk>
                <trkseg>
                    <trkpt lat="45.0" lon="10.0"><ele>100</ele></trkpt>
                    <trkpt lat="45.1" lon="10.1"><ele>110</ele></trkpt>
                    <trkpt lat="45.2" lon="10.2"><ele>105</ele></trkpt>
                    <trkpt lat="45.3" lon="10.3"><ele>120</ele></trkpt>
                </trkseg>
            </trk>
        </gpx>
    )";

    parser.parseData(gpxData);
    // Total gain is (110-100) + (120-105) = 10 + 15 = 25
    EXPECT_NEAR(parser.getTotalElevationGain(), 25.0, 0.1);
}

// Test case for handling empty GPX data
TEST_F(GPXParserTest, EmptyData) {
    QString gpxData = "";
    EXPECT_FALSE(parser.parseData(gpxData));
    EXPECT_TRUE(parser.getPoints().empty());
}

// Test case for handling GPX data with no track points
TEST_F(GPXParserTest, NoTrackPoints) {
    QString gpxData = R"(
        <gpx>
            <trk>
                <trkseg></trkseg>
            </trk>
        </gpx>
    )";

    EXPECT_FALSE(parser.parseData(gpxData));
    EXPECT_TRUE(parser.getPoints().empty());
}
