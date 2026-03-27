#include "DragonBonesItem.h"

#include <QQuickWindow>
#include <QSGGeometryNode>
#include <QSGOpacityNode>
#include <QSGTextureMaterial>
#include <QSGTransformNode>
#include <QHash>

namespace {

class RootNode : public QSGNode
{
public:
    ~RootNode() override
    {
        qDeleteAll(ownedTextures);
        ownedTextures.clear();
    }

    QVector<QSGTexture*> ownedTextures;
};

static QMatrix4x4 centeredMatrix(const QMatrix4x4& m, const QRectF& bounds, const QSizeF& itemSize)
{
    QMatrix4x4 out;
    out.setToIdentity();
    out.translate(itemSize.width() * 0.5f - bounds.center().x(),
                  itemSize.height() * 0.5f - bounds.center().y());
    out *= m;
    return out;
}

} // namespace

DragonBonesItem::DragonBonesItem(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);

    connect(&m_timer, &QTimer::timeout, this, &DragonBonesItem::tick);
    m_timer.start(16);

    m_elapsed.start();
    m_lastMs = m_elapsed.elapsed();
}

void DragonBonesItem::setSkeletonSource(const QUrl& value)
{
    if (m_skeletonSource == value) return;
    m_skeletonSource = value;
    emit skeletonSourceChanged();
    reload();
}

void DragonBonesItem::setAtlasSource(const QUrl& value)
{
    if (m_atlasSource == value) return;
    m_atlasSource = value;
    emit atlasSourceChanged();
    reload();
}

void DragonBonesItem::setTextureSource(const QUrl& value)
{
    if (m_textureSource == value) return;
    m_textureSource = value;
    emit textureSourceChanged();
    reload();
}

void DragonBonesItem::setDragonBonesName(const QString& value)
{
    if (m_dragonBonesName == value) return;
    m_dragonBonesName = value;
    emit dragonBonesNameChanged();
    reload();
}

void DragonBonesItem::setArmatureName(const QString& value)
{
    if (m_armatureName == value) return;
    m_armatureName = value;
    emit armatureNameChanged();
    reload();
}

void DragonBonesItem::setAnimationName(const QString& value)
{
    if (m_animationName == value) return;
    m_animationName = value;
    emit animationNameChanged();
    reload();
}

void DragonBonesItem::setPlaying(bool value)
{
    if (m_playing == value) return;
    m_playing = value;
    emit playingChanged();
}

void DragonBonesItem::setSpeed(qreal value)
{
    if (qFuzzyCompare(m_speed, value)) return;
    m_speed = value;
    emit speedChanged();
}

void DragonBonesItem::setScaleFactor(qreal value)
{
    if (qFuzzyCompare(m_scaleFactor, value)) return;
    m_scaleFactor = value;
    emit scaleFactorChanged();
    reload();
}

void DragonBonesItem::reload()
{
    m_player.setSkeletonSource(m_skeletonSource);
    m_player.setAtlasSource(m_atlasSource);
    m_player.setTextureSource(m_textureSource);
    m_player.setDragonBonesName(m_dragonBonesName);
    m_player.setArmatureName(m_armatureName);
    m_player.setAnimationName(m_animationName);
    m_player.setScaleFactor(m_scaleFactor);
    m_player.reload();

    emit errorStringChanged();
    emit animationsChanged();
    update();
}

void DragonBonesItem::tick()
{
    if (!m_playing) return;

    const qint64 now = m_elapsed.elapsed();
    const double dt = double(now - m_lastMs) / 1000.0;
    m_lastMs = now;

    m_player.advance(dt * m_speed);
    update();
}

QSGNode* DragonBonesItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*)
{
    auto* root = static_cast<RootNode*>(oldNode);
    if (!root) {
        root = new RootNode();
    }

    // 清理上一帧的临时节点
    while (auto* child = root->firstChild()) {
        root->removeChildNode(child);
        delete child;
    }

    // 注意：
    // 这里“缓存纹理”的前提是 QSGTexture 由当前 window 创建且跨帧复用。
    // 但为了保持稳定，不做复杂的纹理字典常驻，只保留当前 root 生命周期内 ownedTextures。
    // 如果你后面还要继续优化，可以再把纹理字典提到成员级。
    qDeleteAll(root->ownedTextures);
    root->ownedTextures.clear();

    if (!window()) {
        return root;
    }

    const auto& frame = m_player.frame();
    const QRectF sceneBounds = frame.bounds.isNull()
                                   ? QRectF(-64, -64, 128, 128)
                                   : frame.bounds;

    // 简单纹理缓存：同一帧内同一个 QImage* 只创建一次 texture
    QHash<const QImage*, QSGTexture*> textureCache;

    for (const auto& item : frame.items) {
        if (!item.image || item.image->isNull()) {
            continue;
        }

        QSGTexture* texture = textureCache.value(item.image, nullptr);
        if (!texture) {
            texture = window()->createTextureFromImage(*item.image);
            if (!texture) {
                continue;
            }
            texture->setFiltering(QSGTexture::Linear);
            texture->setMipmapFiltering(QSGTexture::None);
            texture->setHorizontalWrapMode(QSGTexture::ClampToEdge);
            texture->setVerticalWrapMode(QSGTexture::ClampToEdge);

            textureCache.insert(item.image, texture);
            root->ownedTextures.push_back(texture);
        }

        auto* transformNode = new QSGTransformNode();
        transformNode->setMatrix(centeredMatrix(item.transform, sceneBounds, QSizeF(width(), height())));

        auto* opacityNode = new QSGOpacityNode();
        opacityNode->setOpacity(float(item.opacity));
        transformNode->appendChildNode(opacityNode);

        auto* geomNode = new QSGGeometryNode();
        auto* material = new QSGTextureMaterial();
        material->setTexture(texture);
        material->setFiltering(QSGTexture::Linear);
        geomNode->setMaterial(material);
        geomNode->setFlag(QSGNode::OwnsMaterial, true);

        if (item.kind == RenderKind::Quad) {
            auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4, 6);
            geometry->setDrawingMode(QSGGeometry::DrawTriangles);

            auto* v = geometry->vertexDataAsTexturedPoint2D();
            const QRectF r = item.targetRect;
            const qreal tw = item.image->width();
            const qreal th = item.image->height();
            const QRectF s(item.sourceRect.x() / tw,
                           item.sourceRect.y() / th,
                           item.sourceRect.width() / tw,
                           item.sourceRect.height() / th);

            v[0].set(float(r.left()),  float(r.top()),    float(s.left()),  float(s.top()));
            v[1].set(float(r.right()), float(r.top()),    float(s.right()), float(s.top()));
            v[2].set(float(r.left()),  float(r.bottom()), float(s.left()),  float(s.bottom()));
            v[3].set(float(r.right()), float(r.bottom()), float(s.right()), float(s.bottom()));

            auto* idx = geometry->indexDataAsUShort();
            idx[0] = 0; idx[1] = 1; idx[2] = 2;
            idx[3] = 2; idx[4] = 1; idx[5] = 3;

            geomNode->setGeometry(geometry);
            geomNode->setFlag(QSGNode::OwnsGeometry, true);
        } else {
            const int vc = item.vertices.size();
            const int ic = item.indices.size();

            if (vc <= 0 || ic <= 0) {
                delete material;
                delete geomNode;
                delete opacityNode;
                delete transformNode;
                continue;
            }

            auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), vc, ic);
            geometry->setDrawingMode(QSGGeometry::DrawTriangles);

            auto* v = geometry->vertexDataAsTexturedPoint2D();
            for (int i = 0; i < vc; ++i) {
                const auto p = item.vertices[i];
                const auto uv = item.uvs.value(i);
                v[i].set(float(p.x()), float(p.y()), float(uv.x()), float(uv.y()));
            }

            auto* idx = geometry->indexDataAsUShort();
            for (int i = 0; i < ic; ++i) {
                idx[i] = item.indices[i];
            }

            geomNode->setGeometry(geometry);
            geomNode->setFlag(QSGNode::OwnsGeometry, true);
        }

        opacityNode->appendChildNode(geomNode);
        root->appendChildNode(transformNode);
    }

    return root;
}
