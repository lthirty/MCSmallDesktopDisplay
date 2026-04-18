# MCSmallDesktopDisplay Firmware

本目录是当前可持续开发与烧写的主工程（PlatformIO）。

## 快速开始

1. 安装 VS Code + PlatformIO 插件（或命令行 PlatformIO）。
2. 打开本目录 `2.Firmware/MCSmallDesktopDisplay`。
3. 如需修改串口，编辑 `platformio.ini` 中的 `upload_port`（默认 `COM3`）。
4. 编译：

```powershell
platformio run
```

5. 烧写：

```powershell
platformio run -t upload --upload-port COM3
```

6. 串口监视：

```powershell
platformio device monitor -b 115200
```

## 说明

- 编译成功后，`tools/export_bin.py` 会将 bin 导出到 `2.Firmware/02.bin文件-封装好的代码/`。
- 已移除 `.pio/.history/.vscode` 等过程性文件，仓库保持精简，可直接继续开发调试。
