#include "QtDragonBonesBackend.h"

#include <QFile>
#include <QImageReader>
#include <QPainter>
#include <QVector4D>
#include <algorithm>

using namespace dragonBones;

namespace
{
static QMatrix4x4 toQMatrix(const Matrix& m, qreal scaleFactor = 1.0)
{
    QMatrix4x4 out;
    out.setToIdentity();
    out(0, 0) = m.a * scaleFactor;
    out(0, 1) = m.c * scaleFactor;
    out(1, 0) = m.b * scaleFactor;
    out(1, 1) = m.d * scaleFactor;
    out(0, 3) = m.tx * scaleFactor;
    out(1, 3) = m.ty * scaleFactor;
    return out;
}

static QRectF makeLocalRect(const QSizeF& size, float pivotX, float pivotY)
{
    return QRectF(-pivotX, -pivotY, size.width(), size.height());
}
}

void QtTextureData::_onClear()
{
    TextureData::_onClear();
    atlasImage = nullptr;
}

void QtTextureAtlasData::_onClear()
{
    TextureAtlasData::_onClear();
    image = QImage();
}

TextureData* QtTextureAtlasData::createTexture() const
{
    return BaseObject::borrowObject<QtTextureData>();
}

QtArmatureProxy::QtArmatureProxy() = default;
QtArmatureProxy::~QtArmatureProxy() = default;

void QtArmatureProxy::dbInit(Armature* armature)
{
    m_armature = armature;
}

void QtArmatureProxy::dbClear()
{
    m_armature = nullptr;
    m_listeners.clear();
}

void QtArmatureProxy::dbUpdate()
{
}

void QtArmatureProxy::dispose(bool)
{
    if (m_armature != nullptr) {
        m_armature->dispose();
        m_armature = nullptr;
    }
}

Armature* QtArmatureProxy::getArmature() const
{
    return m_armature;
}

Animation* QtArmatureProxy::getAnimation() const
{
    return m_armature ? m_armature->getAnimation() : nullptr;
}

bool QtArmatureProxy::hasDBEventListener(const std::string& type) const
{
    const auto it = m_listeners.find(type);
    return it != m_listeners.end() && !it->second.empty();
}

void QtArmatureProxy::dispatchDBEvent(const std::string& type, EventObject* value)
{
    const auto it = m_listeners.find(type);
    if (it == m_listeners.end()) {
        return;
    }

    for (const auto& fn : it->second) {
        fn(value);
    }
}

void QtArmatureProxy::addDBEventListener(const std::string& type,
                                         const std::function<void(EventObject*)>& listener)
{
    m_listeners[type].push_back(listener);
}

void QtArmatureProxy::removeDBEventListener(const std::string& type,
                                            const std::function<void(EventObject*)>&)
{
    m_listeners.erase(type);
}

void QtSlot::_onClear()
{
    Slot::_onClear();
    m_displayObject = nullptr;
    m_currentImage = nullptr;
    m_sourceRect = QRectF();
    m_size = QSizeF();
    m_rotated = false;
    m_isMesh = false;
    m_skinnedMesh = false;
    m_vertices.clear();
    m_uvs.clear();
    m_indices.clear();
}

void QtSlot::_initDisplay(void* value, bool)
{
    Q_UNUSED(value);
}

void QtSlot::_disposeDisplay(void* value, bool)
{
    delete static_cast<QtDisplayObject*>(value);
}

void QtSlot::_onUpdateDisplay()
{
    m_displayObject = static_cast<QtDisplayObject*>(_display);
}

void QtSlot::_addDisplay()
{
}

void QtSlot::_replaceDisplay(void*, bool)
{
    _onUpdateDisplay();
}

void QtSlot::_removeDisplay()
{
}

void QtSlot::_updateZOrder()
{
}

void QtSlot::_updateTransform()
{
}

void QtSlot::_identityTransform()
{
}

void QtSlot::_updateVisible()
{
}

void QtSlot::_updateBlendMode()
{
}

void QtSlot::_updateColor()
{
}

void QtSlot::_updateFrame()
{
    m_currentImage = nullptr;
    m_sourceRect = QRectF();
    m_size = QSizeF();
    m_rotated = false;
    m_isMesh = false;
    m_skinnedMesh = false;

    const auto* displayData = _displayData;
    if (!displayData || !_textureData) {
        return;
    }

    if (displayData->type == DisplayType::Image) {
        const auto* imageData = static_cast<const ImageDisplayData*>(displayData);
        const auto* qtTexture = static_cast<QtTextureData*>(imageData->texture);
        if (!qtTexture || !qtTexture->atlasImage) {
            return;
        }

        m_currentImage = qtTexture->atlasImage;
        m_sourceRect = QRectF(qtTexture->region.x,
                              qtTexture->region.y,
                              qtTexture->region.width,
                              qtTexture->region.height);

        const auto width = qtTexture->frame ? qtTexture->frame->width : qtTexture->region.width;
        const auto height = qtTexture->frame ? qtTexture->frame->height : qtTexture->region.height;
        m_size = QSizeF(width, height);
        m_rotated = qtTexture->rotated;

        _pivotX = imageData->pivot.x * width + (qtTexture->frame ? qtTexture->frame->x : 0.0f);
        _pivotY = imageData->pivot.y * height + (qtTexture->frame ? qtTexture->frame->y : 0.0f);
        return;
    }

    if (displayData->type == DisplayType::Mesh) {
        const auto* meshData = static_cast<const MeshDisplayData*>(displayData);
        const auto* qtTexture = static_cast<QtTextureData*>(meshData->texture);
        if (!qtTexture || !qtTexture->atlasImage) {
            return;
        }

        m_currentImage = qtTexture->atlasImage;
        m_sourceRect = QRectF(qtTexture->region.x,
                              qtTexture->region.y,
                              qtTexture->region.width,
                              qtTexture->region.height);
        m_size = QSizeF(qtTexture->region.width, qtTexture->region.height);
        m_rotated = qtTexture->rotated;
        m_isMesh = true;

        _pivotX = 0.0f;
        _pivotY = 0.0f;

        _updateMesh();
        return;
    }
}

// void QtSlot::_updateMesh()
// {
//     m_vertices.clear();
//     m_uvs.clear();
//     m_indices.clear();
//     m_skinnedMesh = false;

//     const auto hasFFD = (_deformVertices != nullptr && !_deformVertices->vertices.empty());

//     const auto* verticesData =
//         (_deformVertices != nullptr && _display == _meshDisplay)
//             ? _deformVertices->verticesData
//             : nullptr;

//     if (verticesData == nullptr) {
//         const auto* meshDisplay = dynamic_cast<const MeshDisplayData*>(_displayData);
//         if (!meshDisplay) {
//             return;
//         }
//         verticesData = &meshDisplay->vertices;
//     }

//     if (!verticesData || !verticesData->data) {
//         return;
//     }

//     const auto* data = verticesData->data;
//     const auto* intArray = data->intArray;
//     const auto* floatArray = data->floatArray;
//     if (!intArray || !floatArray) {
//         return;
//     }

//     const auto scale = _armature->_armatureData->scale;

//     const unsigned vertexCount =
//         intArray[verticesData->offset + (unsigned)BinaryOffset::MeshVertexCount];
//     const unsigned triangleCount =
//         intArray[verticesData->offset + (unsigned)BinaryOffset::MeshTriangleCount];

//     int vertexOffset =
//         intArray[verticesData->offset + (unsigned)BinaryOffset::MeshFloatOffset];
//     if (vertexOffset < 0) {
//         vertexOffset += 65536;
//     }

//     const unsigned uvOffset = vertexOffset + vertexCount * 2;
//     const unsigned indexOffset = verticesData->offset + (unsigned)BinaryOffset::MeshVertexIndices;

//     if (vertexCount == 0 || triangleCount == 0) {
//         return;
//     }

//     m_vertices.resize(int(vertexCount));
//     m_uvs.resize(int(vertexCount));
//     m_indices.resize(int(triangleCount * 3));

//     const qreal texW = m_currentImage ? m_currentImage->width() : 1.0;
//     const qreal texH = m_currentImage ? m_currentImage->height() : 1.0;

//     for (unsigned i = 0; i < vertexCount; ++i) {
//         const auto i2 = i * 2;
//         const float u = floatArray[uvOffset + i2];
//         const float v = floatArray[uvOffset + i2 + 1];

//         qreal px, py;
//         if (m_rotated) {
//             px = m_sourceRect.x() + (1.0f - v) * m_sourceRect.width();
//             py = m_sourceRect.y() + u * m_sourceRect.height();
//         } else {
//             px = m_sourceRect.x() + u * m_sourceRect.width();
//             py = m_sourceRect.y() + v * m_sourceRect.height();
//         }

//         m_uvs[int(i)] = QPointF(px / texW, py / texH);
//     }

//     for (unsigned i = 0; i < triangleCount * 3; ++i) {
//         m_indices[int(i)] = static_cast<std::uint16_t>(intArray[indexOffset + i]);
//     }

//     const auto* weightData = verticesData->weight;

//     if (weightData != nullptr && _deformVertices != nullptr) {
//         m_skinnedMesh = true;
//         const auto& deformVertices = _deformVertices->vertices;
//         const auto& bones = _deformVertices->bones;

//         int weightFloatOffset =
//             intArray[weightData->offset + (unsigned)BinaryOffset::WeigthFloatOffset];
//         if (weightFloatOffset < 0) {
//             weightFloatOffset += 65536;
//         }

//         for (std::size_t i = 0,
//                          iB = weightData->offset + (unsigned)BinaryOffset::WeigthBoneIndices + bones.size(),
//                          iV = (std::size_t)weightFloatOffset,
//                          iF = 0;
//              i < vertexCount;
//              ++i)
//         {
//             const auto boneCount = (std::size_t)intArray[iB++];
//             qreal xG = 0.0;
//             qreal yG = 0.0;

//             for (std::size_t j = 0; j < boneCount; ++j) {
//                 const auto boneIndex = (unsigned)intArray[iB++];
//                 Bone* bone = (boneIndex < bones.size()) ? bones[boneIndex] : nullptr;
//                 if (bone == nullptr) {
//                     ++iV;
//                     iV += 2;
//                     if (hasFFD) {
//                         iF += 2;
//                     }
//                     continue;
//                 }

//                 const auto& matrix = bone->globalTransformMatrix;
//                 const qreal weight = floatArray[iV++];
//                 qreal xL = floatArray[iV++] * scale;
//                 qreal yL = floatArray[iV++] * scale;

//                 if (hasFFD) {
//                     xL += deformVertices[iF++];
//                     yL += deformVertices[iF++];
//                 }

//                 xG += (matrix.a * xL + matrix.c * yL + matrix.tx) * weight;
//                 yG += (matrix.b * xL + matrix.d * yL + matrix.ty) * weight;
//             }

//             m_vertices[int(i)] = QPointF(xG, yG);
//         }

//         return;
//     }

//     if (hasFFD) {
//         const auto& deformVertices = _deformVertices->vertices;
//         for (std::size_t i = 0, l = vertexCount * 2; i < l; i += 2) {
//             const auto iH = i / 2;
//             const qreal x = floatArray[vertexOffset + i] * scale + deformVertices[i];
//             const qreal y = floatArray[vertexOffset + i + 1] * scale + deformVertices[i + 1];
//             m_vertices[int(iH)] = QPointF(x, y);
//         }
//     } else {
//         for (std::size_t i = 0, l = vertexCount * 2; i < l; i += 2) {
//             const auto iH = i / 2;
//             const qreal x = floatArray[vertexOffset + i] * scale;
//             const qreal y = floatArray[vertexOffset + i + 1] * scale;
//             m_vertices[int(iH)] = QPointF(x, y);
//         }
//     }
// }
void QtSlot::_updateMesh()
{
    m_vertices.clear();
    m_uvs.clear();
    m_indices.clear();
    m_skinnedMesh = false;

    const bool hasFFD = (_deformVertices != nullptr && !_deformVertices->vertices.empty());

    const auto* verticesData =
        (_deformVertices != nullptr && _display == _meshDisplay)
            ? _deformVertices->verticesData
            : nullptr;

    if (verticesData == nullptr) {
        const auto* meshDisplay = dynamic_cast<const MeshDisplayData*>(_displayData);
        if (meshDisplay == nullptr) {
            return;
        }
        verticesData = &meshDisplay->vertices;
    }

    if (verticesData == nullptr || verticesData->data == nullptr) {
        return;
    }

    const auto* data = verticesData->data;
    const auto* intArray = data->intArray;
    const auto* floatArray = data->floatArray;
    if (intArray == nullptr || floatArray == nullptr) {
        return;
    }

    const float scale = _armature->_armatureData->scale;

    const unsigned vertexCount =
        (unsigned)intArray[verticesData->offset + (unsigned)BinaryOffset::MeshVertexCount];
    const unsigned triangleCount =
        (unsigned)intArray[verticesData->offset + (unsigned)BinaryOffset::MeshTriangleCount];

    int vertexOffset =
        intArray[verticesData->offset + (unsigned)BinaryOffset::MeshFloatOffset];
    if (vertexOffset < 0) {
        vertexOffset += 65536;
    }

    const unsigned uvOffset = (unsigned)vertexOffset + vertexCount * 2;
    const unsigned indexOffset =
        verticesData->offset + (unsigned)BinaryOffset::MeshVertexIndices;

    if (vertexCount == 0 || triangleCount == 0) {
        return;
    }

    m_vertices.resize((int)vertexCount);
    m_uvs.resize((int)vertexCount);
    m_indices.resize((int)(triangleCount * 3));

    const qreal texW = m_currentImage ? m_currentImage->width() : 1.0;
    const qreal texH = m_currentImage ? m_currentImage->height() : 1.0;

    const QString slotName =
        (_slotData != nullptr) ? QString::fromStdString(_slotData->name) : QStringLiteral("<null-slot>");

    const bool debugThisSlot = (slotName == QStringLiteral("ganzi1qizi"));

    if (debugThisSlot) {
        qDebug().noquote()
        << "[DB][Mesh][BEGIN]"
        << "slot=" << slotName
        << " vertexCount=" << vertexCount
        << " triangleCount=" << triangleCount
        << " hasFFD=" << hasFFD
        << " displayIsMesh=" << (_display == _meshDisplay)
        << " verticesOffset=" << verticesData->offset
        << " vertexOffset=" << vertexOffset
        << " uvOffset=" << uvOffset
        << " indexOffset=" << indexOffset;
    }

    for (unsigned i = 0; i < vertexCount; ++i) {
        const unsigned i2 = i * 2;
        const float u = floatArray[uvOffset + i2];
        const float v = floatArray[uvOffset + i2 + 1];

        qreal px = 0.0;
        qreal py = 0.0;

        if (m_rotated) {
            px = m_sourceRect.x() + (1.0f - v) * m_sourceRect.width();
            py = m_sourceRect.y() + u * m_sourceRect.height();
        } else {
            px = m_sourceRect.x() + u * m_sourceRect.width();
            py = m_sourceRect.y() + v * m_sourceRect.height();
        }

        m_uvs[(int)i] = QPointF(px / texW, py / texH);
    }

    for (unsigned i = 0; i < triangleCount * 3; ++i) {
        m_indices[(int)i] = (std::uint16_t)intArray[indexOffset + i];
    }

    const auto* weightData = verticesData->weight;

    // -------- skinned mesh --------
    if (weightData != nullptr && _deformVertices != nullptr) {
        m_skinnedMesh = true;

        const auto& deformVertices = _deformVertices->vertices;
        const auto& bones = _deformVertices->bones;

        int weightFloatOffset =
            intArray[weightData->offset + (unsigned)BinaryOffset::WeightFloatOffset];
        if (weightFloatOffset < 0) {
            weightFloatOffset += 65536;
        }

        std::size_t iB =
            (std::size_t)weightData->offset +
            (std::size_t)BinaryOffset::WeightBoneIndices +
            bones.size();
        std::size_t iV = (std::size_t)weightFloatOffset;
        std::size_t iF = 0;

        if (debugThisSlot) {
            qDebug().noquote()
            << "[DB][Mesh][SkinInfo]"
            << "slot=" << slotName
            << " weightOffset=" << weightData->offset
            << " weightFloatOffset=" << weightFloatOffset
            << " bones.size=" << bones.size()
            << " deform.size=" << deformVertices.size()
            << " iB.begin=" << iB
            << " iV.begin=" << iV
            << " iF.begin=" << iF;
        }

        for (std::size_t i = 0; i < vertexCount; ++i) {
            const std::size_t boneCount = (std::size_t)intArray[iB++];
            qreal xG = 0.0;
            qreal yG = 0.0;

            if (debugThisSlot && (i < 12 || i % 20 == 0)) {
                qDebug().noquote()
                << "[DB][Mesh][VertexBegin]"
                << "slot=" << slotName
                << " v=" << i
                << " boneCount=" << boneCount
                << " iB=" << iB
                << " iV=" << iV
                << " iF=" << iF;
            }

            for (std::size_t j = 0; j < boneCount; ++j) {
                const unsigned boneIndex = (unsigned)intArray[iB++];
                Bone* bone = (boneIndex < bones.size()) ? bones[boneIndex] : nullptr;

                if (bone == nullptr) {
                    ++iV;       // skip weight
                    iV += 2;    // skip xL, yL
                    if (hasFFD) {
                        iF += 2; // skip deform x,y for this influence
                    }

                    if (debugThisSlot) {
                        qDebug().noquote()
                        << "[DB][Mesh][BoneNull]"
                        << "slot=" << slotName
                        << " v=" << i
                        << " j=" << j
                        << " boneIndex=" << boneIndex
                        << " iB=" << iB
                        << " iV=" << iV
                        << " iF=" << iF;
                    }
                    continue;
                }

                const auto& matrix = bone->globalTransformMatrix;

                const qreal weight = floatArray[iV++];
                qreal xL = floatArray[iV++] * scale;
                qreal yL = floatArray[iV++] * scale;

                qreal deformX = 0.0;
                qreal deformY = 0.0;

                if (hasFFD) {
                    // 注意：官方 JS 语义是 skinned mesh 的 deform 按 influence 走，不是按 logical vertex 走
                    deformX = deformVertices[iF++];
                    deformY = deformVertices[iF++];
                    xL += deformX;
                    yL += deformY;
                }

                const qreal xW = matrix.a * xL + matrix.c * yL + matrix.tx;
                const qreal yW = matrix.b * xL + matrix.d * yL + matrix.ty;

                xG += xW * weight;
                yG += yW * weight;

                if (debugThisSlot && (i < 12 || i % 20 == 0)) {
                    QString boneName = QStringLiteral("<null-bone-data>");
                    const auto* boneData = bone->getBoneData();
                    if (boneData != nullptr) {
                        boneName = QString::fromStdString(boneData->name);
                    }

                    qDebug().noquote()
                        << "[DB][Mesh][Influence]"
                        << "slot=" << slotName
                        << " v=" << i
                        << " j=" << j
                        << " boneIndex=" << boneIndex
                        << " bone=" << boneName
                        << " weight=" << weight
                        << " xL=" << xL
                        << " yL=" << yL
                        << " deformX=" << deformX
                        << " deformY=" << deformY
                        << " xW=" << xW
                        << " yW=" << yW
                        << " iB=" << iB
                        << " iV=" << iV
                        << " iF=" << iF;
                }
            }

            m_vertices[(int)i] = QPointF(xG, yG);

            if (debugThisSlot && (i < 12 || i % 20 == 0)) {
                qDebug().noquote()
                << "[DB][Mesh][VertexEnd]"
                << "slot=" << slotName
                << " v=" << i
                << " xG=" << xG
                << " yG=" << yG
                << " iB=" << iB
                << " iV=" << iV
                << " iF=" << iF;
            }
        }

        if (debugThisSlot) {
            qDebug().noquote()
            << "[DB][Mesh][END-SKIN]"
            << "slot=" << slotName
            << " finalVertexCount=" << m_vertices.size()
            << " finalUvCount=" << m_uvs.size()
            << " finalIndexCount=" << m_indices.size();
        }

        return;
    }

    // -------- unskinned mesh --------
    if (hasFFD) {
        const auto& deformVertices = _deformVertices->vertices;

        for (std::size_t i = 0, l = (std::size_t)vertexCount * 2; i < l; i += 2) {
            const std::size_t iH = i / 2;
            const qreal x = floatArray[vertexOffset + i] * scale + deformVertices[i];
            const qreal y = floatArray[vertexOffset + i + 1] * scale + deformVertices[i + 1];
            m_vertices[(int)iH] = QPointF(x, y);

            if (debugThisSlot && (iH < 12 || iH % 20 == 0)) {
                qDebug().noquote()
                << "[DB][Mesh][UnskinnedFFD]"
                << "slot=" << slotName
                << " v=" << iH
                << " x=" << x
                << " y=" << y
                << " baseX=" << floatArray[vertexOffset + i] * scale
                << " baseY=" << floatArray[vertexOffset + i + 1] * scale
                << " deformX=" << deformVertices[i]
                << " deformY=" << deformVertices[i + 1];
            }
        }
    } else {
        for (std::size_t i = 0, l = (std::size_t)vertexCount * 2; i < l; i += 2) {
            const std::size_t iH = i / 2;
            const qreal x = floatArray[vertexOffset + i] * scale;
            const qreal y = floatArray[vertexOffset + i + 1] * scale;
            m_vertices[(int)iH] = QPointF(x, y);

            if (debugThisSlot && (iH < 12 || iH % 20 == 0)) {
                qDebug().noquote()
                << "[DB][Mesh][Unskinned]"
                << "slot=" << slotName
                << " v=" << iH
                << " x=" << x
                << " y=" << y;
            }
        }
    }

    if (debugThisSlot) {
        qDebug().noquote()
        << "[DB][Mesh][END-PLAIN]"
        << "slot=" << slotName
        << " finalVertexCount=" << m_vertices.size()
        << " finalUvCount=" << m_uvs.size()
        << " finalIndexCount=" << m_indices.size();
    }
}

Matrix QtSlot::combinedMatrix(const Matrix& parentMatrix) const
{
    Matrix out = globalTransformMatrix;
    out.concat(parentMatrix);
    return out;
}

bool QtSlot::buildRenderItem(RenderItem& item,
                             const Matrix& parentMatrix,
                             qreal parentOpacity) const
{
    if (!_display || !_visible || !m_currentImage) {
        return false;
    }

    if (m_sourceRect.width() <= 0 || m_sourceRect.height() <= 0) {
        return false;
    }

    const qreal opacity = parentOpacity * _colorTransform.alphaMultiplier;
    const auto matrix = combinedMatrix(parentMatrix);

    if (m_isMesh && !m_vertices.isEmpty() && !m_indices.isEmpty()) {
        item.kind = RenderKind::Mesh;
        item.image = m_currentImage;
        item.sourceRect = m_sourceRect;
        item.opacity = opacity;
        item.zOrder = _zOrder;
        item.vertices = m_vertices;
        item.uvs = m_uvs;
        item.indices = m_indices;

        if (m_skinnedMesh) {
            item.transform.setToIdentity();
        } else {
            item.transform = toQMatrix(matrix);
        }
        return true;
    }

    if (m_size.width() <= 0 || m_size.height() <= 0) {
        return false;
    }

    item.kind = RenderKind::Quad;
    item.image = m_currentImage;
    item.sourceRect = m_sourceRect;
    item.targetRect = makeLocalRect(m_size, _pivotX, _pivotY);
    item.transform = toQMatrix(matrix);
    item.opacity = opacity;
    item.zOrder = _zOrder;
    return true;
}

void QtSlot::updateRenderItem(RenderItem& item,
                              const Matrix& parentMatrix,
                              qreal parentOpacity) const
{
    if (!_display || !_visible || !m_currentImage) {
        item.opacity = 0.0;
        return;
    }

    const qreal opacity = parentOpacity * _colorTransform.alphaMultiplier;
    const auto matrix = combinedMatrix(parentMatrix);

    item.opacity = opacity;
    item.image = m_currentImage;
    item.sourceRect = m_sourceRect;
    item.zOrder = _zOrder;

    if (item.kind == RenderKind::Mesh) {
        item.vertices = m_vertices;
        if (m_skinnedMesh) {
            item.transform.setToIdentity();
        } else {
            item.transform = toQMatrix(matrix);
        }
    } else {
        item.targetRect = makeLocalRect(m_size, _pivotX, _pivotY);
        item.transform = toQMatrix(matrix);
    }
}

void QtSlot::appendRenderItems(QVector<RenderItem>& out,
                               const Matrix& parentMatrix,
                               qreal parentOpacity) const
{
    RenderItem item;
    if (buildRenderItem(item, parentMatrix, parentOpacity)) {
        out.push_back(item);
    }
}

bool QtSlot::hasRenderableDisplayForCache() const
{
    return _display != nullptr && _displayIndex >= 0 && _visible && m_currentImage != nullptr;
}

int QtSlot::displayIndexForCache() const
{
    return _displayIndex;
}

QtFactory::QtFactory()
    : BaseFactory()
{
    m_eventManager = new QtArmatureProxy();
    m_dragonBonesInstance = new DragonBones(m_eventManager);
    _dragonBones = m_dragonBonesInstance;
}

QtFactory::~QtFactory()
{
    delete m_dragonBonesInstance;
    m_dragonBonesInstance = nullptr;
    delete m_eventManager;
    m_eventManager = nullptr;
    _dragonBones = nullptr;
}

dragonBones::Armature* QtFactory::buildArmatureNoAdvance(const std::string& armatureName,
                                                         const std::string& dragonBonesName,
                                                         const std::string& skinName,
                                                         const std::string& textureAtlasName) const
{
    dragonBones::BuildArmaturePackage dataPackage;
    if (!_fillBuildArmaturePackage(dataPackage, dragonBonesName, armatureName, skinName, textureAtlasName)) {
        return nullptr;
    }

    const auto armature = _buildArmature(dataPackage);
    _buildBones(dataPackage, armature);
    _buildSlots(dataPackage, armature);
    return armature;
}

TextureAtlasData* QtFactory::_buildTextureAtlasData(TextureAtlasData* textureAtlasData,
                                                    void* textureAtlas) const
{
    auto* atlas = static_cast<QtTextureAtlasData*>(textureAtlasData);
    if (!atlas) {
        atlas = BaseObject::borrowObject<QtTextureAtlasData>();
    }

    if (textureAtlas) {
        atlas->image = *static_cast<QImage*>(textureAtlas);
        for (const auto& pair : atlas->textures) {
            if (auto* tex = dynamic_cast<QtTextureData*>(pair.second)) {
                tex->atlasImage = &atlas->image;
            }
        }
    }

    return atlas;
}

Armature* QtFactory::_buildArmature(const BuildArmaturePackage& dataPackage) const
{
    auto* armature = BaseObject::borrowObject<Armature>();
    auto* proxy = new QtArmatureProxy();
    armature->init(dataPackage.armature, proxy, proxy, _dragonBones);
    return armature;
}

Slot* QtFactory::_buildSlot(const BuildArmaturePackage& dataPackage,
                            const SlotData* slotData,
                            Armature* armature) const
{
    Q_UNUSED(dataPackage);
    auto* slot = BaseObject::borrowObject<QtSlot>();
    slot->init(slotData,
               armature,
               new QtDisplayObject{DisplayType::Image},
               new QtDisplayObject{DisplayType::Mesh});
    return slot;
}

DragonBonesNativePlayer::DragonBonesNativePlayer(QObject* parent)
    : QObject(parent)
{
}

DragonBonesNativePlayer::~DragonBonesNativePlayer()
{
    clearArmature();
    delete m_factory;
}

void DragonBonesNativePlayer::setSkeletonSource(const QUrl& url)
{
    m_skeletonSource = url;
}

void DragonBonesNativePlayer::setAtlasSource(const QUrl& url)
{
    m_atlasSource = url;
}

void DragonBonesNativePlayer::setTextureSource(const QUrl& url)
{
    m_textureSource = url;
}

void DragonBonesNativePlayer::setDragonBonesName(const QString& value)
{
    m_dragonBonesName = value;
}

void DragonBonesNativePlayer::setArmatureName(const QString& value)
{
    m_armatureName = value;
}

void DragonBonesNativePlayer::setAnimationName(const QString& value)
{
    m_animationName = value;
}

void DragonBonesNativePlayer::setScaleFactor(qreal value)
{
    m_scaleFactor = value;
}

QString DragonBonesNativePlayer::errorString() const
{
    return m_error;
}

QStringList DragonBonesNativePlayer::animations() const
{
    return m_animations;
}

const RenderFrame& DragonBonesNativePlayer::frame() const
{
    return m_frame;
}

QString DragonBonesNativePlayer::resolvePath(const QUrl& url) const
{
    if (url.scheme() == "qrc") {
        return ":" + url.path();
    }
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    return url.toString();
}

void DragonBonesNativePlayer::clearArmature()
{
    if (m_armature && m_factory && m_factory->getClock()) {
        m_factory->getClock()->remove(m_armature);
        m_armature->dispose();
        m_armature = nullptr;
    }
}

bool DragonBonesNativePlayer::reload()
{
    m_error.clear();
    m_animations.clear();
    m_frame = {};
    m_renderCache.clear();
    m_renderCacheDirty = false;
    m_lastRenderCacheRebuildMs = 0;

    clearArmature();
    delete m_factory;
    m_factory = new QtFactory();

    const QString texturePath = resolvePath(m_textureSource);
    if (!texturePath.isEmpty()) {
        QImageReader reader(texturePath);
        m_texture = reader.read();
    }

    const QString skeletonPath = resolvePath(m_skeletonSource);
    const QString atlasPath = resolvePath(m_atlasSource);
    if (skeletonPath.isEmpty() || atlasPath.isEmpty() || m_texture.isNull()) {
        m_texture = QImage(128, 128, QImage::Format_RGBA8888);
        m_texture.fill(QColor(90, 20, 20));
        QPainter p(&m_texture);
        p.setPen(Qt::white);
        p.drawRect(0, 0, 127, 127);
        p.drawText(m_texture.rect(), Qt::AlignCenter, QStringLiteral("No asset"));

        RenderItem item;
        item.kind = RenderKind::Quad;
        item.image = &m_texture;
        item.sourceRect = QRectF(0, 0, 128, 128);
        item.targetRect = QRectF(-64, -64, 128, 128);
        item.opacity = 1.0;
        item.transform = toQMatrix(Matrix(), m_scaleFactor);
        m_frame.items = {item};
        m_frame.bounds = item.targetRect;
        m_error = QStringLiteral("Set skeletonSource/atlasSource/textureSource to load DragonBones assets.");
        return false;
    }

    QFile skeletonFile(skeletonPath);
    QFile atlasFile(atlasPath);
    if (!skeletonFile.open(QIODevice::ReadOnly) || !atlasFile.open(QIODevice::ReadOnly)) {
        m_error = QStringLiteral("Failed to open skeleton or atlas JSON.");
        return false;
    }

    const QByteArray skeletonBytes = skeletonFile.readAll();
    const QByteArray atlasBytes = atlasFile.readAll();
    const std::string dbName = m_dragonBonesName.isEmpty()
        ? std::string()
        : m_dragonBonesName.toStdString();

    auto* dbData = m_factory->parseDragonBonesData(skeletonBytes.constData(), dbName);
    if (!dbData) {
        m_error = QStringLiteral("parseDragonBonesData failed.");
        return false;
    }

    auto* atlasData = m_factory->parseTextureAtlasData(atlasBytes.constData(), &m_texture, dbName);
    if (!atlasData) {
        m_error = QStringLiteral("parseTextureAtlasData failed.");
        return false;
    }

    const std::string armName = m_armatureName.toStdString();
    m_armature = m_factory->buildArmatureNoAdvance(armName, dbName);
    if (!m_armature) {
        m_error = QStringLiteral("buildArmature failed for '%1'.").arg(m_armatureName);
        return false;
    }

    m_armature->invalidUpdate("", true);
    m_armature->advanceTime(0.0f);

    m_factory->getClock()->add(m_armature);

    for (const auto& name : m_armature->getAnimation()->getAnimationNames()) {
        m_animations.push_back(QString::fromStdString(name));
    }

    if (!m_animationName.isEmpty()) {
        m_armature->getAnimation()->fadeIn(m_animationName.toStdString());
    } else if (!m_animations.isEmpty()) {
        m_armature->getAnimation()->play(m_animations.first().toStdString());
    }

    buildRenderCache();
    updateRenderCache();
    return true;
}

void DragonBonesNativePlayer::buildRenderCache()
{
    m_frame.items.clear();
    m_renderCache.clear();
    m_frame.bounds = QRectF();

    if (!m_armature) {
        return;
    }

    const Matrix identity;
    const qreal opacity = 1.0;

    for (auto* slotBase : m_armature->getSlots()) {
        auto* slot = dynamic_cast<QtSlot*>(slotBase);
        if (!slot) {
            continue;
        }

        RenderItem item;
        if (!slot->buildRenderItem(item, identity, opacity)) {
            continue;
        }

        CachedRenderEntry entry;
        entry.slot = slot;
        entry.itemIndex = m_frame.items.size();
        entry.wasRenderable = true;
        entry.lastDisplayIndex = slot->displayIndexForCache();

        m_renderCache.push_back(entry);
        m_frame.items.push_back(item);
    }

    std::sort(m_frame.items.begin(), m_frame.items.end(),
              [](const RenderItem& a, const RenderItem& b) {
                  return a.zOrder < b.zOrder;
              });

    m_lastRenderCacheRebuildMs = QDateTime::currentMSecsSinceEpoch();
    m_renderCacheDirty = false;
}

void DragonBonesNativePlayer::updateRenderCache()
{
    if (!m_armature) {
        return;
    }

    const Matrix identity;
    const qreal opacity = 1.0;

    for (const auto& entry : m_renderCache) {
        if (!entry.slot || entry.itemIndex < 0 || entry.itemIndex >= m_frame.items.size()) {
            continue;
        }
        entry.slot->updateRenderItem(m_frame.items[entry.itemIndex], identity, opacity);
    }

    QRectF bounds;
    bool first = true;

    for (const auto& item : m_frame.items) {
        if (item.opacity <= 0.0) {
            continue;
        }

        if (item.kind == RenderKind::Quad) {
            const QVector4D pts[4] = {
                QVector4D(item.targetRect.left(),  item.targetRect.top(),    0.0f, 1.0f),
                QVector4D(item.targetRect.right(), item.targetRect.top(),    0.0f, 1.0f),
                QVector4D(item.targetRect.left(),  item.targetRect.bottom(), 0.0f, 1.0f),
                QVector4D(item.targetRect.right(), item.targetRect.bottom(), 0.0f, 1.0f),
            };

            for (int pi = 0; pi < 4; ++pi) {
                const auto hp = item.transform.map(pts[pi]).toPointF();
                if (first) {
                    bounds = QRectF(hp, QSizeF(0, 0));
                    first = false;
                } else {
                    bounds = bounds.united(QRectF(hp, QSizeF(0, 0)));
                }
            }
        } else if (!item.vertices.isEmpty()) {
            for (const auto& p : item.vertices) {
                const auto hp = item.transform.map(QVector4D(p.x(), p.y(), 0.0f, 1.0f)).toPointF();
                if (first) {
                    bounds = QRectF(hp, QSizeF(0, 0));
                    first = false;
                } else {
                    bounds = bounds.united(QRectF(hp, QSizeF(0, 0)));
                }
            }
        }
    }

    m_frame.bounds = bounds;
}

bool DragonBonesNativePlayer::needsRebuildRenderCache() const
{
    if (!m_armature) {
        return false;
    }

    int renderableCount = 0;
    int changedCount = 0;

    for (auto* slotBase : m_armature->getSlots()) {
        auto* slot = dynamic_cast<QtSlot*>(slotBase);
        if (!slot) {
            continue;
        }

        const bool renderable = slot->hasRenderableDisplayForCache();
        const int displayIndex = slot->displayIndexForCache();

        if (renderable) {
            ++renderableCount;
        }

        bool found = false;
        for (const auto& entry : m_renderCache) {
            if (entry.slot == slot) {
                found = true;
                if (entry.wasRenderable != renderable || entry.lastDisplayIndex != displayIndex) {
                    ++changedCount;
                }
                break;
            }
        }

        if (!found && renderable) {
            ++changedCount;
        }
    }

    if (renderableCount != m_renderCache.size()) {
        return true;
    }

    return changedCount > 0;
}

void DragonBonesNativePlayer::advance(double dtSeconds)
{
    if (!(m_factory && m_factory->dragonBonesInstance())) {
        return;
    }

    m_factory->dragonBonesInstance()->advanceTime(static_cast<float>(dtSeconds));

    if (needsRebuildRenderCache()) {
        m_renderCacheDirty = true;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_renderCacheDirty && now - m_lastRenderCacheRebuildMs > 120) {
        buildRenderCache();
    }

    updateRenderCache();
}
