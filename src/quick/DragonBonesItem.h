#pragma once

#include "legenddragonbones_global.h"
#include "QtDragonBonesBackend.h"

#include <QtQml/qqmlregistration.h>

#include <QElapsedTimer>
#include <QQuickItem>
#include <QTimer>
#include <QUrl>

class LEGENDDRAGONBONES_EXPORT DragonBonesItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl skeletonSource READ skeletonSource WRITE setSkeletonSource NOTIFY skeletonSourceChanged)
    Q_PROPERTY(QUrl atlasSource READ atlasSource WRITE setAtlasSource NOTIFY atlasSourceChanged)
    Q_PROPERTY(QUrl textureSource READ textureSource WRITE setTextureSource NOTIFY textureSourceChanged)
    Q_PROPERTY(QString dragonBonesName READ dragonBonesName WRITE setDragonBonesName NOTIFY dragonBonesNameChanged)
    Q_PROPERTY(QString armatureName READ armatureName WRITE setArmatureName NOTIFY armatureNameChanged)
    Q_PROPERTY(QString animationName READ animationName WRITE setAnimationName NOTIFY animationNameChanged)
    Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(qreal speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(qreal scaleFactor READ scaleFactor WRITE setScaleFactor NOTIFY scaleFactorChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(QStringList animations READ animations NOTIFY animationsChanged)

public:
    explicit DragonBonesItem(QQuickItem* parent = nullptr);

    QUrl skeletonSource() const { return m_skeletonSource; }
    QUrl atlasSource() const { return m_atlasSource; }
    QUrl textureSource() const { return m_textureSource; }
    QString dragonBonesName() const { return m_dragonBonesName; }
    QString armatureName() const { return m_armatureName; }
    QString animationName() const { return m_animationName; }
    bool playing() const { return m_playing; }
    qreal speed() const { return m_speed; }
    qreal scaleFactor() const { return m_scaleFactor; }
    QString errorString() const { return m_player.errorString(); }
    QStringList animations() const { return m_player.animations(); }

    void setSkeletonSource(const QUrl& value);
    void setAtlasSource(const QUrl& value);
    void setTextureSource(const QUrl& value);
    void setDragonBonesName(const QString& value);
    void setArmatureName(const QString& value);
    void setAnimationName(const QString& value);
    void setPlaying(bool value);
    void setSpeed(qreal value);
    void setScaleFactor(qreal value);

Q_SIGNALS:
    void skeletonSourceChanged();
    void atlasSourceChanged();
    void textureSourceChanged();
    void dragonBonesNameChanged();
    void armatureNameChanged();
    void animationNameChanged();
    void playingChanged();
    void speedChanged();
    void scaleFactorChanged();
    void errorStringChanged();
    void animationsChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;

private:
    void tick();
    void reload();

    QUrl m_skeletonSource;
    QUrl m_atlasSource;
    QUrl m_textureSource;
    QString m_dragonBonesName;
    QString m_armatureName;
    QString m_animationName;
    bool m_playing = true;
    qreal m_speed = 1.0;
    qreal m_scaleFactor = 1.0;

    DragonBonesNativePlayer m_player;
    QTimer m_timer;
    QElapsedTimer m_elapsed;
    qint64 m_lastMs = 0;
};
