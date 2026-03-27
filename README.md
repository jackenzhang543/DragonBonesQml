# DragonBones Qml Plugin

A Qml plugin to add DragonBones for Qml.

Qt version: 6.10

Example:

```qml
import DragonBones

Window {
    width: 960
    height: 720
    visible: true
    color: "#202020"
    title: "DragonBones QML Plugin Demo"

    DragonBonesItem {
        anchors.centerIn: parent
        skeletonSource: "qrc:/legend/dragonbones/samples/XingXiang_ske.json"
        atlasSource: "qrc:/legend/dragonbones/samples/XingXiang_tex.json"
        textureSource: "qrc:/legend/dragonbones/samples/XingXiang_tex.png"
        dragonBonesName: "demoDB"
        armatureName: "XingXiang"
        animationName: "DaiJi"
        playing: true
        scaleFactor: 0.5
    }
}
```
