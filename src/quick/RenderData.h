#pragma once

#include <QImage>
#include <QMatrix4x4>
#include <QPointF>
#include <QRectF>
#include <QVector>

#include <cstdint>

enum class RenderKind {
    Quad,
    Mesh,
};

struct RenderItem {
    RenderKind kind = RenderKind::Quad;
    const QImage* image = nullptr;
    QRectF sourceRect;
    QRectF targetRect;
    QMatrix4x4 transform;
    qreal opacity = 1.0;
    int zOrder = 0;
    QVector<QPointF> vertices;
    QVector<QPointF> uvs;
    QVector<std::uint16_t> indices;
};

struct RenderFrame {
    QVector<RenderItem> items;
    QRectF bounds;
};
