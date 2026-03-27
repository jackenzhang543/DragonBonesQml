#pragma once

#include "RenderData.h"

#include <QObject>
#include <QImage>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>
#include <QRectF>
#include <QSizeF>
#include <QDateTime>
#include <cstdint>
#include <map>
#include <vector>
#include <functional>

#ifdef signals
#undef signals
#endif

#ifdef slots
#undef slots
#endif

#include "dragonBones/DragonBonesHeaders.h"

class QtTextureData final : public dragonBones::TextureData
{
    BIND_CLASS_TYPE_A(QtTextureData);
public:
    QImage* atlasImage = nullptr;

protected:
    void _onClear() override;
};

class QtTextureAtlasData final : public dragonBones::TextureAtlasData
{
    BIND_CLASS_TYPE_A(QtTextureAtlasData);
public:
    QImage image;

protected:
    void _onClear() override;

public:
    dragonBones::TextureData* createTexture() const override;
};

class QtArmatureProxy final : public dragonBones::IArmatureProxy
{
public:
    QtArmatureProxy();
    ~QtArmatureProxy() override;

    void dbInit(dragonBones::Armature* armature) override;
    void dbClear() override;
    void dbUpdate() override;
    void dispose(bool disposeProxy) override;
    dragonBones::Armature* getArmature() const override;
    dragonBones::Animation* getAnimation() const override;

    bool hasDBEventListener(const std::string& type) const override;
    void dispatchDBEvent(const std::string& type, dragonBones::EventObject* value) override;
    void addDBEventListener(const std::string& type, const std::function<void(dragonBones::EventObject*)>& listener) override;
    void removeDBEventListener(const std::string& type, const std::function<void(dragonBones::EventObject*)>& listener) override;

private:
    dragonBones::Armature* m_armature = nullptr;
    std::map<std::string, std::vector<std::function<void(dragonBones::EventObject*)>>> m_listeners;
};

struct QtDisplayObject
{
    dragonBones::DisplayType displayType = dragonBones::DisplayType::Image;
};

class QtSlot final : public dragonBones::Slot
{
    BIND_CLASS_TYPE_A(QtSlot);
public:
    void appendRenderItems(QVector<RenderItem>& out,
                           const dragonBones::Matrix& parentMatrix,
                           qreal parentOpacity) const;

    bool buildRenderItem(RenderItem& out,
                         const dragonBones::Matrix& parentMatrix,
                         qreal parentOpacity) const;
    void updateRenderItem(RenderItem& item,
                          const dragonBones::Matrix& parentMatrix,
                          qreal parentOpacity) const;

    bool hasRenderableDisplayForCache() const;
    int displayIndexForCache() const;

    qreal alphaMultiplier() const { return _colorTransform.alphaMultiplier; }

protected:
    void _onClear() override;
    void _initDisplay(void* value, bool isRetain) override;
    void _disposeDisplay(void* value, bool isRelease) override;
    void _onUpdateDisplay() override;
    void _addDisplay() override;
    void _replaceDisplay(void* value, bool isArmatureDisplay) override;
    void _removeDisplay() override;
    void _updateZOrder() override;
    void _updateFrame() override;
    void _updateMesh() override;
    void _updateTransform() override;
    void _identityTransform() override;
    void _updateVisible() override;
    void _updateBlendMode() override;
    void _updateColor() override;

private:
    dragonBones::Matrix combinedMatrix(const dragonBones::Matrix& parentMatrix) const;

    QtDisplayObject* m_displayObject = nullptr;
    const QImage* m_currentImage = nullptr;
    QRectF m_sourceRect;
    QSizeF m_size;
    bool m_rotated = false;
    bool m_isMesh = false;
    bool m_skinnedMesh = false;
    QVector<QPointF> m_vertices;
    QVector<QPointF> m_uvs;
    QVector<std::uint16_t> m_indices;
};

class QtFactory final : public dragonBones::BaseFactory
{
public:
    QtFactory();
    ~QtFactory() override;

    dragonBones::DragonBones* dragonBonesInstance() { return m_dragonBonesInstance; }

    dragonBones::Armature* buildArmatureNoAdvance(const std::string& armatureName,
                                                  const std::string& dragonBonesName,
                                                  const std::string& skinName = "",
                                                  const std::string& textureAtlasName = "") const;

protected:
    dragonBones::TextureAtlasData* _buildTextureAtlasData(dragonBones::TextureAtlasData* textureAtlasData,
                                                          void* textureAtlas) const override;
    dragonBones::Armature* _buildArmature(const dragonBones::BuildArmaturePackage& dataPackage) const override;
    dragonBones::Slot* _buildSlot(const dragonBones::BuildArmaturePackage& dataPackage,
                                  const dragonBones::SlotData* slotData,
                                  dragonBones::Armature* armature) const override;

private:
    QtArmatureProxy* m_eventManager = nullptr;
    dragonBones::DragonBones* m_dragonBonesInstance = nullptr;
};

struct CachedRenderEntry
{
    QtSlot* slot = nullptr;
    int itemIndex = -1;
    bool wasRenderable = false;
    int lastDisplayIndex = -1;
};

class DragonBonesNativePlayer : public QObject
{
    Q_OBJECT
public:
    explicit DragonBonesNativePlayer(QObject* parent = nullptr);
    ~DragonBonesNativePlayer() override;

    void setSkeletonSource(const QUrl& url);
    void setAtlasSource(const QUrl& url);
    void setTextureSource(const QUrl& url);
    void setDragonBonesName(const QString& value);
    void setArmatureName(const QString& value);
    void setAnimationName(const QString& value);
    void setScaleFactor(qreal value);

    bool reload();
    void advance(double dtSeconds);

    QString errorString() const;
    QStringList animations() const;
    const RenderFrame& frame() const;

private:
    QString resolvePath(const QUrl& url) const;
    void buildRenderCache();
    void updateRenderCache();
    bool needsRebuildRenderCache() const;
    void clearArmature();

    QUrl m_skeletonSource;
    QUrl m_atlasSource;
    QUrl m_textureSource;
    QString m_dragonBonesName;
    QString m_armatureName;
    QString m_animationName;
    qreal m_scaleFactor = 1.0;

    QString m_error;
    QStringList m_animations;
    QImage m_texture;
    RenderFrame m_frame;
    QVector<CachedRenderEntry> m_renderCache;

    bool m_renderCacheDirty = false;
    qint64 m_lastRenderCacheRebuildMs = 0;

    QtFactory* m_factory = nullptr;
    dragonBones::Armature* m_armature = nullptr;
};
