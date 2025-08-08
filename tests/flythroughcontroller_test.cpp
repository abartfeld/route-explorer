#include <QtTest/QtTest>
#include <QSignalSpy>
#include <Qt3DRender/QCamera>
#include "FlythroughController.h"
#include "RouteData.h"

class FlythroughControllerTest : public QObject
{
    Q_OBJECT

private:
    std::vector<TrackPoint> createSampleTrack() {
        std::vector<TrackPoint> points;
        points.push_back({QGeoCoordinate(34.0, -118.0, 100.0), 100.0, 0.0});
        points.push_back({QGeoCoordinate(34.0, -117.0, 110.0), 110.0, 100000.0});
        points.push_back({QGeoCoordinate(35.0, -118.0, 120.0), 120.0, 200000.0});
        return points;
    }

private slots:
    void testStartStop();
    void testSignalEmitted();
};

void FlythroughControllerTest::testStartStop()
{
    auto points = createSampleTrack();
    RouteData routeData(points);
    auto camera = new Qt3DRender::QCamera();
    FlythroughController controller(&routeData, camera);

    QVector3D initialPos = camera->position();

    controller.start();
    QTest::qWait(100); // Let it run for a few frames

    QVector3D movedPos = camera->position();
    QVERIFY(movedPos != initialPos); // Verify position changed during animation

    controller.stop();

    // After stop, the camera should be at the start of the route (progress = 0)
    RoutePoint startPoint = routeData.getPointAtProgress(0.0f);
    QVector3D expectedStartPos = startPoint.position + QVector3D(0, 1.8f, 0); // 1.8f is CAMERA_HEIGHT_OFFSET

    // Using QVERIFY with a tolerance for floating point comparisons
    QVERIFY(camera->position().distanceToPoint(expectedStartPos) < 0.001f);
}

void FlythroughControllerTest::testSignalEmitted()
{
    auto points = createSampleTrack();
    RouteData routeData(points);
    auto camera = new Qt3DRender::QCamera();
    FlythroughController controller(&routeData, camera);

    QSignalSpy spy(&controller, &FlythroughController::positionChanged);

    controller.start();
    QTest::qWait(200);
    controller.stop();

    QVERIFY(spy.count() > 0); // At least one signal should have been emitted
}

QTEST_MAIN(FlythroughControllerTest)
#include "flythroughcontroller_test.moc"
