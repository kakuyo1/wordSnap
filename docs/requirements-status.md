# wordSnap V1 原始需求对照

更新时间：2026-03-27

## 已完成

- 搭建 Qt6 Widgets + C++17 + CMake 工程，主程序可构建并运行。
- 实现单实例运行（`QLockFile`）。
- 实现系统托盘常驻与菜单（Capture / Settings / Exit）。
- 实现 Windows 全局热键（`RegisterHotKey`）并支持失败回退提示。
- 实现全屏框选截图（覆盖层 + 鼠标框选 + 取消）。
- 实现截图后 OCR 预处理（灰度、缩放、阈值）。
- 打通 OCR 流程（当前通过 Tesseract CLI 调用）。
- 实现 OCR 结果归一化（提取英文单词候选）。
- 实现 StarDict 最小可用查询（`.ifo + .idx + .dict`）。
- 实现结果展示闭环（鼠标附近卡片 + tooltip + tray 消息）。
- 实现启动配置读取（热键、词典目录、tessdata 目录）。
- 实现 `SettingsDialog`（热键、词典目录、tessdata 目录可编辑并落盘）。
- 增强 Tesseract 路径探测与错误提示（应用目录、环境变量、PATH、常见安装目录）。
- 结果状态统一为 `FOUND` / `OCR_FAILED` / `UNKNOWN` / `DICT_UNAVAILABLE`，卡片与托盘文案一致。
- 结果卡片支持风格切换（Kraft paper / Glassmorphism / Terminal），并支持运行时应用。
- 设置项新增 `ResultCardStyle` 与 `QueryHistoryLimit` 并持久化。
- 新增查询历史 V1（JSONL 持久化、按词/时间过滤、快速复查、清空历史、最近 N 条裁剪）。
- 建立 CTest 测试基线并纳入 `WordNormalizerTest` / `PhoneticExtractorTest` / `StarDictBackendTest` / `LookupCoordinatorTest` / `QueryHistoryServiceTest`。
- 接入 AI Assist（DeepSeek OpenAI-Compatible）：设置页新增 AI 配置，启动校验无效配置并自动降级。
- 查词流程改为“两阶段渲染”：先展示基础词典结果，再异步请求 AI 并在卡片底部回填。
- 新增 `AiAssistServiceTest`，覆盖配置校验、结构化解析、字段长度约束与关闭态降级。
- 设置页新增结果卡片持续时间（ms）配置，并持久化到本地设置。
- 结果卡片自动隐藏逻辑支持悬停保持：超时时若鼠标在卡片内则保持显示，移出后自动关闭。
- 移除 display mode 后，设置持久化层会在保存时自动清理历史遗留键 `display_mode`。

## 未完成

- Tesseract 尚未切换到 C++ API（当前为 CLI，属于实现差异）。
- StarDict 深度解析尚未完成（`sametypesequence` 精细字段处理）。
- 压缩词典格式尚未支持（`.dict.dz`、`.idx.gz`、`.syn`）。

## 备注

- 当前默认词典路径为 `applicationDirPath()/dict`，默认 tessdata 路径为 `applicationDirPath()/tessdata`。
- 若提示 `Dictionary unavailable`，请先确认 `dict` 目录存在且包含有效 StarDict 文件。
- 若提示 `Tesseract did not start`，请确认 PATH 中放的是 Tesseract 所在目录，或设置 `TESSERACT_EXE`/`TESSERACT_PATH`。
