# SmallDesktopDisplay
# 沙雕系列-SD -太空人天气时钟显示

**视频介绍：** https://www.bilibili.com/video/BV1V64y1e72M/

## 1. 硬件打样说明

**PCB打样的话暂时没发现有啥需要特别注意的，注意附带的文件下单提示。** PCB文件可以直接拿去工厂打样，两层板很便宜，器件BOM的话也都是比较常用的。

`Hardware`文件内目前包含两个版本的PCB电路：

* **SD2-推荐桌面摆件** ：基于下面的版本轻微修改，将按键删去，无需按键，一键直达编译下载;新增了触摸扩展板，<code>IO口标识需要根据实际连接的改</code>
* **SD3-电脑主机显示** ：即视频中出现的版本，修改了PCB形状以适配新的外壳


**外壳加工** 根据自己喜欢的版本选择，`3D Model`文件夹目前包含2个版本的外壳文件：一 一 对 应文件即可


## 2. 固件编译说明

固件框架主要基于Arduino开发完成，玩过Arduino的基本没有上手难度了，把Firmware/Libraries里面的库安装到Arduino库目录（如果你用的是Arduino IDE的话）即可。

> 由于后续发现，使用的是VS code上面的PlatfromIO插件进行Arduino开发，因为VS code+PlatfromIO编译速度起飞，强烈推荐使用VS code+PlatfromIO来编译，大家可以选择自己喜欢的IDE就好了。
> 


**然后这里需要官方库文件才能正常使用：**

肯定得安装ESP83266的Arduino<code>**支持包2.6.3**</code>（百度也有海量教程，本文件已经附带安装包，直接安装即可）


**代码优化版本：VS code+PlatfromIO** https://github.com/SmallDesktopDisplay-team/SmallDesktopDisplay

## 3. v1.4加气象预警 自定义改动记录（2026-04-09）

本次在 `2.Firmware/源代码/v1.4加气象预警/v1.4加气象预警/SmallDesktopDisplay` 中做了以下定制：

1. 开机等待WiFi连接画面 Logo 替换
- 原“adrobot”图标已替换为 `5.Docs/图片素材/开机/布布头像.jpg`
- 代码位置：`loading()`，资源头文件为 `img/boot_logo_bubu.h`

2. 正常运行画面右下角区域替换
- 原内温/内湿数字区域（屏幕右下角）替换为同一张布布头像
- 资源头文件为 `img/indoor_bubu.h`

3. 去闪屏优化
- 由于 `IndoorTem()` 会在主循环中被周期调用，若每次都重绘 JPG 会出现约 1-2 秒闪动一次
- 当前已改为“仅在整屏刷新时重绘头像”，日常刷新不重复解码绘制，避免闪屏

## 4. QQ交流群

请注明来意！

官方群1：<code>811058758</code> （已满）

官方群2：<code>887171863</code>（已满）

官方群3：<code>477140920</code>（未满）

## 其他的后续再补充，有用的话记得点星星~

