# wordSnapV1

Windows-only 离线托盘单词识别查词工具（V1）。

核心流程：全局热键触发截图框选 -> OCR -> 单词归一化 -> StarDict 查询 -> 鼠标附近结果卡片显示。

## 技术栈

- C++17
- Qt 6 Widgets
- CMake
- Tesseract OCR（当前通过 CLI 调用）
- StarDict（当前支持 `.ifo + .idx + .dict`）

## 当前状态

- 已打通最小可运行闭环。
- 可通过托盘菜单或全局热键触发截图识别。
- 设置页尚未实现（托盘项为占位）。

详见：`docs/requirements-status.md`。

## 目录结构

```text
wordSnapV1/
  CMakeLists.txt
  src/
    app/
    platform/win/
    services/
    ui/
  docs/
  scripts/
```

## 构建

要求：

- Windows
- CMake >= 3.21
- Qt 6.5+（Core/Gui/Widgets）
- 可用的 C++17 编译器（MSVC 或 MinGW）

示例（PowerShell）：

```powershell
cmake -S . -B build -G "Ninja"
cmake --build build
```

## 运行前准备

### 1) Tesseract

- 安装 Tesseract，并确保 `tesseract.exe` 可被程序发现。
- 推荐方式：将 Tesseract 安装目录加入 PATH（不是 `tesseract.exe` 文件本身）。
- 可选环境变量：
  - `TESSERACT_EXE`：直接指向 `tesseract.exe`
  - `TESSERACT_PATH`：可指向目录或 `tesseract.exe`
- 若使用自定义 tessdata，可将 `eng.traineddata` 放在应用目录下 `tessdata/`，或在设置中配置路径。

### 2) StarDict 词典

- 默认读取应用目录下 `dict/`。
- 当前仅支持未压缩文件组合：`xxx.ifo` + `xxx.idx` + `xxx.dict`。
- 若只有 `.dict.dz`，请先解压为 `.dict`。

## 使用说明

- 启动程序后常驻系统托盘。
- 默认热键：`Ctrl+Alt+S`。
- 按热键后框选目标区域，程序会自动 OCR + 查词。
- 查询结果以 tooltip、卡片和托盘消息展示。

## 常见问题

- `Dictionary unavailable`：检查 `dict/` 路径及词典文件完整性。
- `Tesseract did not start`：检查 PATH、`TESSERACT_EXE`/`TESSERACT_PATH` 配置是否正确。
- 热键无响应：确认未被其他程序占用；程序会尝试回退到 `Ctrl+Alt+S` 并提示。

## 后续计划

- 增加 `SettingsDialog`。
- 完善结果卡片字段（音标/词性等）。
- 评估切换到 Tesseract C++ API。
- 增强 StarDict 解析与压缩格式支持。
