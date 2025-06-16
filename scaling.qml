// scaling.qml
import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    // 动态绑定到父控件尺寸
    width: parent.width
    height: parent.height

    AnimatedImage {
        id: gifImage
        source: gifPath
        anchors.fill: parent
        playing: true
        smooth: true
        mipmap: true  // 启用Mipmap抗锯齿
        
        // 高质量缩放设置
        fillMode: Image.PreserveAspectFit
        asynchronous: true  // 异步加载
        
        // 透明通道处理
        opacity: parent.opacity
    }

    // 穿透鼠标事件到父窗口
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onPressed: mouse.accepted = false  // 事件穿透
    }
}