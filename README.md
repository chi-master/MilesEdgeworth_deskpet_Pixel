使用Qt 6.5.1 + Visual Studio 2022.

图片素材和音频素材均来自游戏《逆转裁判》和《逆转检事》.

b站有演示：https://www.bilibili.com/video/BV1KfEDzDEmL/?share_source=copy_web&vd_source=4e1f5a80e26a44a2d0bb8612a4952199

这是我第一次从零开始一个项目一直到发布，还有很多不完善的地方，请大家多多包涵。在这里记录一下实现的内容和一点心得:

0. 窗口（桌宠本体）无边框透明置顶，不在任务栏显示（Qt::Tool），在状态栏显示(QSystemTrayIcon)，拖拽移动窗口.
    注意窗口类型设置为Qt::Tool之后关闭窗口不会退出程序，我的解决方法是重写`closeEvent()`关闭时发送信号给QApplication，让它执行`quit()`
   
1. 按照一定规律随机播放gif
   
2. 单击不同区域触发不同动画
   
3. 双击触发动画并播放语音（此处涉及到单击、双击和拖拽的区分，想了挺久的）
   
   3.1 “看招”时飞出检察官徽章涉及到多窗口问题，窗口的自动移动使用了QPropertyAnimation
   
   3.2 播放语音使用QSoundEffect，需要Qt的multimedia模块
   
4. 右键菜单的若干功能
   
5. 图片置顶查看器：以QGraphicsView为基础。支持多开，上限10个。此处涉及多窗口问题
   
   5.1 采用了双向链表结构来管理打开的若干图片查看器窗口，打开时new，关闭时delete。退出桌宠时全部delete
   
   5.2 在调试过程中发现QPixmap随着图片切换会占越来越多内存，后来查资料发现是因为加载图片时图片数据加入到QPixmapCache缓冲区上，所以通过及时调用`QPixmapCache::clear()`，就能解决内存占用过大的问题

6. 支持双屏模式下的拖拽和跨屏走动. 右键设置双屏选项, 选择"单屏"则限制走动范围在当前所在窗口, 选择"主屏幕在左侧"则在跑步时可跨越主屏幕右边界穿越到第二屏幕的左边界, 选择"主屏幕在右侧"则在跑步时可跨越主屏幕左边界穿越到第二屏幕的右边界. 选项不影响拖拽, 拖拽始终跟随鼠标.

7. 快速晃动触发害怕地震的小动画。主要也是靠的重写press,move和release的鼠标事件来实现，还搭配了一个计时器。
