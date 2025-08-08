#pragma once

#include "GpxParser.h"
#include <vector>
#include <QVector3D>

struct RoutePoint {
    QVector3D position;
    QVector3D direction;
    float distance; // Cumulative distance from the start
};

class RouteData {
public:
    RouteData(const std::vector<TrackPoint>& trackPoints, float elevationScale = 1.0f);
    ~RouteData();

    const std::vector<QVector3D>& getPositions() const { return m_positions; }
    RoutePoint getPointAtProgress(float progress) const;
    size_t getIndexAtProgress(float progress) const;

private:
    void processPoints(const std::vector<TrackPoint>& trackPoints, float elevationScale);

    std::vector<QVector3D> m_positions; // Raw positions, for renderer
    std::vector<RoutePoint> m_routePoints; // Enriched points for controller
    float m_totalDistance = 0.0f;
};
