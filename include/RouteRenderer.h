#pragma once

#include "RouteData.h"
#include <Qt3DCore/QEntity>

class RouteRenderer {
public:
    RouteRenderer(RouteData* routeData, Qt3DCore::QEntity* parentEntity);
    ~RouteRenderer();

    Qt3DCore::QEntity* getEntity() const { return m_entity; }

private:
    void generateMesh();

    RouteData* m_routeData;
    Qt3DCore::QEntity* m_entity;
};
