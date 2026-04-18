# MCSmallDesktopDisplay 项目交接文档

## 一、项目概述

基于 ESP8266 (ESP-12E) 的小型桌面气象时钟显示器，驱动 240×240 ST7789 TFT 屏幕。

**核心功能：**
- 实时天气显示（温度、湿度、AQI、气象预警，数据来源 weather.com.cn）
- NTP 时钟（阿里云 NTP 服务器，东八区）
- WiFiManager AP 模式配网（AP 名称 AutoConnectAP，配网地址 192.168.4.1）
- 常驻 Web 管理页面（亮度/城市/屏幕方向/天气刷新间隔/DHT 开关/图片上传）
- 可选 DHT11 温湿度传感器（GPIO 12）
- 自定义开机 Logo 和运行页右下角图片（通过 Web 上传 JPEG）
- 串口调试命令（115200 波特率）

**当前版本：V1.5.3（2026.04.18）— 基线版本**

---

## 二、硬件配置

| 参数 | 值 |
|------|-----|
| MCU | ESP8266 ESP-12E |
| 屏幕 | 1.3寸 240×240 ST7789 TFT |
| Flash | 4MB |
| RAM | 80KB（可用约 50KB） |
| SPI 频率 | 27MHz |

**引脚分配：**

| 功能 | GPIO |
|------|------|
| SCK | GPIO14 |
| MOSI | GPIO13 |
| TFT_CS | GPIO15 |
| TFT_DC | GPIO0 |
| TFT_RST | GPIO2 |
| LCD 背光 | GPIO5（PWM 控制） |
| DHT11 | GPIO12（可选） |

---

## 三、开发环境配置

### 3.1 必需工具

1. **PlatformIO** — 通过 VS Code 插件安装，或独立安装 PlatformIO Core CLI
   - 安装后 CLI 位于：`%USERPROFILE%\.platformio\penv\Scripts\pio.exe`
   - 如果 `pio` 不在 PATH 中，使用完整路径调用
2. **Python 3.x** — 用于构建脚本和图片转换工具
   - 需要 `Pillow` 库：`pip install Pillow`
   - 需要 `pyserial` 库（串口调试用）：`pip install pyserial`
3. **Git** — 版本管理

### 3.2 串口配置

- **烧写端口**：COM3（在 `platformio.ini` 中配置，按实际修改）
- **烧写波特率**：921600
- **串口监视波特率**：115200

### 3.3 首次克隆后

```bash
git clone https://github.com/lthirty/MCSmallDesktopDisplay.git
# PlatformIO 会自动下载依赖库，无需手动安装
```

---

## 四、编译与烧写

### 4.1 编译

```bash
# 如果 pio 在 PATH 中
pio run

# 如果不在 PATH 中（Windows）
"%USERPROFILE%\.platformio\penv\Scripts\pio.exe" run
```

### 4.2 编译并烧写

```bash
pio run --target upload
# 或
"%USERPROFILE%\.platformio\penv\Scripts\pio.exe" run --target upload
```

### 4.3 自动备份机制

每次编译成功后，`tools/export_bin.py` 会自动执行：
1. 复制 bin 到 `02.bin文件-封装好的代码/` 目录（以版本号命名）
2. 复制 bin 到 `01.Backup/`（命名格式：`MCSmallDesktopDisplay-版本-时间.bin`）
3. 压缩项目到 `01.Backup/`（命名格式：`MCSmallDesktopDisplay-版本-时间.zip`）

压缩时自动排除 `.pio`、`.vscode`、`01.Backup`、`02.bin文件` 等目录。

### 4.4 串口重置 WiFi

设备运行中通过串口发送 `0x05` 可重置 WiFi 并重启：

```python
python -c "import serial,time; s=serial.Serial('COM3',115200,timeout=2); time.sleep(0.5); s.write(b'0x05'); time.sleep(5); print(s.read(2000).decode('utf-8','ignore')); s.close()"
```

---

## 五、项目结构

```
MCSmallDesktopDisplay/
├── MCSmallDesktopDisplay.ino   # 主程序（所有业务逻辑）
├── platformio.ini              # PlatformIO 构建配置
├── number.cpp / number.h       # 数字时钟字体渲染
├── weathernum.cpp / weathernum.h # 天气图标渲染
├── qr.h                        # 二维码图片数据（SmartConfig 用，当前未启用）
├── font/                        # 时钟数字字体 + 中文字库 ZdyLwFont_20
├── img/                         # 内嵌 JPEG 图片资源（C 头文件格式）
│   ├── boot_logo_lianli.h      # 默认开机 Logo（横标白.png 转换）
│   ├── indoor_lianli.h         # 默认右下角图标（竖标白-纯图片.png 转换）
│   ├── boot_logo_bubu.h        # 旧版开机 Logo（备用）
│   ├── indoor_bubu.h           # 旧版右下角图标（备用）
│   ├── temperature.h / humidity.h # 温湿度图标
│   ├── config_tip_cn.h         # 中文配置提示图片（已弃用，改为英文）
│   └── pangzi/                 # 太空人动画帧（10帧）
├── 03.图片/                     # 原始 PNG 图片素材
├── tools/
│   ├── export_bin.py           # 编译后自动备份脚本
│   └── png2header.py           # PNG 转 JPEG C 头文件工具
├── 01.Backup/                   # 历史版本备份（bin + zip）
├── data/                        # LittleFS 数据文件
└── .gitignore
```

---

## 六、关键设计决策与约束

### 6.1 内存限制（最重要）

ESP8266 只有约 50KB 可用堆内存，Flash 已用 86%。以下操作容易导致 OOM 崩溃或重启：

- **不要在 Web 请求处理函数（handleconfig）中发起 HTTP 请求**（如 getCityWeater）。应设置标志位，让 loop() 异步执行。
- **不要在 TFT_eSprite 上下文中调用 loadFont() 加载中文字库**，会导致内存不足崩溃。
- **Web 管理页的 HTML/CSS/JS 全部拼接在 C++ String 中**，每次请求都会产生大量临时 String 对象。这是已知的内存碎片风险，但目前 Flash 空间不足以将静态资源存入 LittleFS。

### 6.2 TFT 字体

TFT_eSPI 内置字体只有 font 1/2/4/6/7/8，没有 font 3。当前启用的是 font 2（16px）和 font 4（26px）。中文使用自定义字库 `ZdyLwFont_20`（20px），但只能在 `tft` 直接绘制时使用，不能在 Sprite 中 loadFont（内存不够）。

### 6.3 图片格式

ESP8266 上的 TJpgDec 库只支持 JPEG 解码。PNG 图片需要先用 `tools/png2header.py` 转换为 JPEG C 头文件嵌入固件。用户通过 Web 上传的图片由前端 JS 预处理为 JPEG 后再上传。

### 6.4 EEPROM 地址布局

| 地址 | 内容 |
|------|------|
| 1 | LCD 亮度（0-100） |
| 2 | 屏幕旋转方向（0-3） |
| 3 | DHT 使能标志 |
| 4 | 天气更新间隔（分钟） |
| 10-14 | 城市代码（5 字节） |
| 30+ | WiFi SSID + 密码（96 字节） |

EEPROM 总分配 1024 字节。地址手动分配，无边界检查。

### 6.5 天气 API

- 城市代码查询：`http://wgeo.weather.com.cn/ip/`
- 天气数据：`http://d1.weather.com.cn/weather_index/{cityCode}.html`
- 需要设置 User-Agent 和 Referer 头，否则会被拒绝

---

## 七、已知问题与经验教训

### 7.1 Web 端多次修改城市会导致重启

**原因**：`handleconfig()` 中直接调用 `getCityWeater()` 发起 HTTP 请求，在 Web 请求上下文中嵌套 HTTP 请求会耗尽内存。

**解决方案（V1.5.1）**：Web 保存城市后只设置 `weaterTime=0` 和 `UpdateWeater_en=1` 标志，由 `loop()` 异步获取天气。

### 7.2 中文字库在 Sprite 中使用会导致 OOM 崩溃

**原因**：`loadFont(ZdyLwFont_20)` 在 Sprite 上下文中调用时，字库数据 + Sprite 缓冲区同时占用内存，超出可用堆。

**解决方案（V1.5.1）**：Service IP 页面改用英文 font 4 显示，不加载中文字库。

### 7.3 WiFiManager Captive Portal 内嵌浏览器按钮无响应

**现象**：手机连接 AutoConnectAP 后弹出的内嵌浏览器中，"Configure WiFi" 按钮点击无效。

**原因**：内嵌浏览器缓存/残留问题，手机重启后恢复正常。

**当前处理（V1.5.3）**：保持 Captive Portal 开启，启用 `setAPClientCheck(true)`，设置 5 分钟配网超时。屏幕提示用户可通过 192.168.4.1 手动访问。

### 7.4 01.Backup 中的 zip 文件不能解压到项目目录内

**原因**：`platformio.ini` 中 `src_dir = .`，PlatformIO 会扫描当前目录下所有子目录的源文件。如果 zip 被解压到 `01.Backup/` 下产生了 `.ino` 和 `.cpp` 文件，会导致 "multiple definition" 链接错误。

**解决方案**：`export_bin.py` 的 `_EXCLUDE_DIRS` 已排除 `01.Backup`，但如果手动解压 zip 到项目目录内，必须删除解压出的文件夹后再编译。

### 7.5 城市名称映射有限

`getCityCodeByName()` 只硬编码了 12 个城市。用户输入其他城市名会静默失败（返回 0）。Web 前端 JS 中也有相同的映射表需要同步维护。

---

## 八、每次修改必须执行的流程

1. **升级版本号**：修改 `#define Version "Vx.x.x"` 和文件头部的更改说明
2. **编译烧写**：`pio run --target upload`（自动生成备份到 01.Backup）
3. **提交推送**：`git add -A . && git commit -m "Vx.x.x: 描述" && git push origin main`

---

## 九、串口调试命令

| 命令 | 功能 |
|------|------|
| 0x01 → 输入数值 | 设置亮度（0-100） |
| 0x02 → 输入代码 | 设置城市代码（9位，输入0自动获取） |
| 0x03 → 输入数值 | 设置屏幕方向（0-3） |
| 0x04 → 输入数值 | 设置天气更新间隔（1-60分钟） |
| 0x05 | 重置 WiFi 并重启 |

---

## 十、GitHub 仓库

- **地址**：https://github.com/lthirty/MCSmallDesktopDisplay
- **分支**：main
- **远程**：origin（主仓库），lthirty（fork 源）
