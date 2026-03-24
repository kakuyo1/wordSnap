# wordSnap V1 原始需求对照

更新时间：2026-03-24

## 已完成

- 搭建 Qt6 Widgets + C++17 + CMake 工程，主程序可构建并运行。
- 实现单实例运行（`QLockFile`）。
- 实现系统托盘常驻与菜单（Capture / Settings 占位 / Exit）。
- 实现 Windows 全局热键（`RegisterHotKey`）并支持失败回退提示。
- 实现全屏框选截图（覆盖层 + 鼠标框选 + 取消）。
- 实现截图后 OCR 预处理（灰度、缩放、阈值）。
- 打通 OCR 流程（当前通过 Tesseract CLI 调用）。
- 实现 OCR 结果归一化（提取英文单词候选）。
- 实现 StarDict 最小可用查询（`.ifo + .idx + .dict`）。
- 实现结果展示闭环（鼠标附近卡片 + tooltip + tray 消息）。
- 实现启动配置读取（热键、显示模式、词典目录、tessdata 目录）。
- 增强 Tesseract 路径探测与错误提示（应用目录、环境变量、PATH、常见安装目录）。

## 未完成

- `SettingsDialog` 交互界面尚未实现（目前仅托盘占位）。
- 结果卡片信息尚未完善（音标 / 词性 / 词源折叠等）。
- Tesseract 尚未切换到 C++ API（当前为 CLI，属于实现差异）。
- StarDict 深度解析尚未完成（`sametypesequence` 精细字段处理）。
- 压缩词典格式尚未支持（`.dict.dz`、`.idx.gz`、`.syn`）。

## 备注

- 当前默认词典路径为 `applicationDirPath()/dict`，默认 tessdata 路径为 `applicationDirPath()/tessdata`。
- 若提示 `Dictionary unavailable`，请先确认 `dict` 目录存在且包含有效 StarDict 文件。
- 若提示 `Tesseract did not start`，请确认 PATH 中放的是 Tesseract 所在目录，或设置 `TESSERACT_EXE`/`TESSERACT_PATH`。
