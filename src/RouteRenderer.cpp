#include "RouteRenderer.h"
#include <cmath>

#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DExtras/QPhongMaterial>

namespace {
    // Configuration for the route mesh
    const float ROUTE_RADIUS = 0.5f; // meters
    const int ROUTE_SEGMENT_SIDES = 8; // An octagon cross-section is a good balance
}

RouteRenderer::RouteRenderer(RouteData* routeData, Qt3DCore::QEntity* parentEntity)
    : m_routeData(routeData), m_entity(new Qt3DCore::QEntity(parentEntity))
{
    logDebug("RouteRenderer", "Creating renderer and generating mesh.");
    generateMesh();
}

RouteRenderer::~RouteRenderer()
{
    logDebug("RouteRenderer", "Destroying renderer.");
    // The entity will be cleaned up by its parent.
}

void RouteRenderer::generateMesh()
{
    if (!m_routeData || m_routeData->getPositions().size() < 2) {
        logWarning("RouteRenderer", "Not enough data to generate a mesh.");
        return;
    }

    auto geometry = new Qt3DRender::QGeometry(m_entity);

    QByteArray vertexBufferData;
    QByteArray indexBufferData;

    const auto& positions = m_routeData->getPositions();
    const int numPoints = positions.size();
    const int numVertices = numPoints * ROUTE_SEGMENT_SIDES;
    const int numIndices = (numPoints - 1) * ROUTE_SEGMENT_SIDES * 2 * 3; // (num segments) * (sides) * (2 triangles per side) * (3 vertices per triangle)
    const size_t numPoints = positions.size();
    const size_t numVertices = numPoints * ROUTE_SEGMENT_SIDES;
    const size_t numIndices = (numPoints > 1) ? (numPoints - 1) * ROUTE_SEGMENT_SIDES * 2 * 3 : 0; // (num segments) * (sides) * (2 triangles per side) * (3 vertices per triangle)

    // Bounds checking to prevent overflow
    if (numVertices > (std::numeric_limits<size_t>::max() / ((3 + 3) * sizeof(float))) ||
        numIndices > (std::numeric_limits<size_t>::max() / sizeof(unsigned int))) {
        logWarning("RouteRenderer", "Route too large to render safely (would overflow buffer sizes).");
        return;
    }
    vertexBufferData.resize(numVertices * (3 + 3) * sizeof(float)); // 3 for pos, 3 for normal
    float* p = reinterpret_cast<float*>(vertexBufferData.data());

    indexBufferData.resize(numIndices * sizeof(unsigned int));
    unsigned int* indices = reinterpret_cast<unsigned int*>(indexBufferData.data());

    QVector3D lastUpVector(0.0f, 1.0f, 0.0f);

    // Generate vertices
    for (int i = 0; i < numPoints; ++i) {
        QVector3D currentPoint = positions[i];
        QVector3D direction;

        if (i < numPoints - 1) {
            direction = (positions[i+1] - currentPoint).normalized();
        } else {
            direction = (currentPoint - positions[i-1]).normalized();
        }

        // Create a coordinate system (tangent, bitangent, normal) for the current point
        QVector3D right = QVector3D::crossProduct(direction, lastUpVector).normalized();
        QVector3D up = QVector3D::crossProduct(right, direction).normalized();
        lastUpVector = up;

        // Generate a circle of vertices around the point
        for (int j = 0; j < ROUTE_SEGMENT_SIDES; ++j) {
            float angle = (float)j / ROUTE_SEGMENT_SIDES * 2.0f * M_PI;
            QVector3D normal = (right * std::cos(angle) + up * std::sin(angle)).normalized();
            QVector3D vertexPos = currentPoint + normal * ROUTE_RADIUS;

            // Position
            *p++ = vertexPos.x();
            *p++ = vertexPos.y();
            *p++ = vertexPos.z();

            // Normal
            *p++ = normal.x();
            *p++ = normal.y();
            *p++ = normal.z();
        }
    }

    // Generate indices
    for (int i = 0; i < numPoints - 1; ++i) {
        for (int j = 0; j < ROUTE_SEGMENT_SIDES; ++j) {
            unsigned int i0 = i * ROUTE_SEGMENT_SIDES + j;
            unsigned int i1 = i * ROUTE_SEGMENT_SIDES + ((j + 1) % ROUTE_SEGMENT_SIDES);
            unsigned int i2 = (i + 1) * ROUTE_SEGMENT_SIDES + j;
            unsigned int i3 = (i + 1) * ROUTE_SEGMENT_SIDES + ((j + 1) % ROUTE_SEGMENT_SIDES);

            // Triangle 1
            *indices++ = i0;
            *indices++ = i2;
            *indices++ = i1;

            // Triangle 2
            *indices++ = i1;
            *indices++ = i2;
            *indices++ = i3;
        }
    }

    auto* vertexBuffer = new Qt3DRender::QBuffer(geometry);
    vertexBuffer->setData(vertexBufferData);

    auto* indexBuffer = new Qt3DRender::QBuffer(geometry);
    indexBuffer->setData(indexBufferData);

    // Position attribute
    auto* posAttr = new Qt3DRender::QAttribute(geometry);
    posAttr->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
    posAttr->setVertexBaseType(Qt3DRender::QAttribute::Float);
    posAttr->setVertexSize(3);
    posAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    posAttr->setBuffer(vertexBuffer);
    posAttr->setByteStride(6 * sizeof(float));
    posAttr->setCount(numVertices);
    geometry->addAttribute(posAttr);

    // Normal attribute
    auto* normAttr = new Qt3DRender::QAttribute(geometry);
    normAttr->setName(Qt3DRender::QAttribute::defaultNormalAttributeName());
    normAttr->setVertexBaseType(Qt3DRender::QAttribute::Float);
    normAttr->setVertexSize(3);
    normAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    normAttr->setBuffer(vertexBuffer);
    normAttr->setByteOffset(3 * sizeof(float));
    normAttr->setByteStride(6 * sizeof(float));
    normAttr->setCount(numVertices);
    geometry->addAttribute(normAttr);

    // Index attribute
    auto* indexAttr = new Qt3DRender::QAttribute(geometry);
    indexAttr->setAttributeType(Qt3DRender::QAttribute::IndexAttribute);
    indexAttr->setBuffer(indexBuffer);
    indexAttr->setVertexBaseType(Qt3DRender::QAttribute::UnsignedInt);
    indexAttr->setCount(numIndices);
    geometry->addAttribute(indexAttr);

    auto* renderer = new Qt3DRender::QGeometryRenderer();
    renderer->setGeometry(geometry);
    renderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);

    auto* material = new Qt3DExtras::QPhongMaterial();
    material->setDiffuse(QColor(QRgb(0x4CAF50))); // A nice green color

    m_entity->addComponent(renderer);
    m_entity->addComponent(material);
}
