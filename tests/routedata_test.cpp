#include <QtTest/QtTest>
#include "RouteData.h"
#include "GpxParser.h" // For TrackPoint definition

class RouteDataTest : public QObject
{
    Q_OBJECT

private slots:
    void testEmptyData();
    void testProcessPoints();
};

void RouteDataTest::testEmptyData()
{
    std::vector<TrackPoint> points;
    RouteData data(points);

    QVERIFY(data.getPositions().empty());
}

void RouteDataTest::testProcessPoints()
{
    // Create a sample track
    std::vector<TrackPoint> points;
    // Point 1: Origin
    points.push_back({QGeoCoordinate(34.0, -118.0, 100.0), 100.0, 0.0});
    // Point 2: About 1 degree east
    points.push_back({QGeoCoordinate(34.0, -117.0, 110.0), 110.0, 100000.0});
    // Point 3: About 1 degree north
    points.push_back({QGeoCoordinate(35.0, -118.0, 120.0), 120.0, 200000.0});


    RouteData data(points);

    const auto& positions = data.getPositions();
    QCOMPARE(positions.size(), (size_t)3);

    // Check origin point (should be at 0, 100, 0)
    QCOMPARE(positions[0].x(), 0.0f);
    QCOMPARE(positions[0].y(), 100.0f);
    QCOMPARE(positions[0].z(), 0.0f);

    // Check second point (roughly)
    // Mercator projection is not linear, so we check for approximate values.
    // At 34 degrees latitude, 1 degree of longitude is roughly 92.4km.
    QVERIFY(positions[1].x() > 92000.0f && positions[1].x() < 93000.0f);
    QCOMPARE(positions[1].y(), 110.0f);
    QVERIFY(std::abs(positions[1].z()) < 1.0f); // Should be very close to 0 on Z axis

    // Check third point (roughly)
    // 1 degree of latitude is roughly 111.32km.
    QVERIFY(std::abs(positions[2].x()) < 1.0f); // Should be very close to 0 on X axis
    QCOMPARE(positions[2].y(), 120.0f);
    QVERIFY(positions[2].z() > 111000.0f && positions[2].z() < 112000.0f);
}


QTEST_MAIN(RouteDataTest)
#include "routedata_test.moc"
