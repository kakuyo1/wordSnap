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
- 结果卡片支持风格切换（Kraft paper / White paper / Glassmorphism / Terminal），并支持运行时应用。
- 设置项新增 `ResultCardStyle` 与 `QueryHistoryLimit` 并持久化。
- 新增查询历史 V1（JSONL 持久化、按词/时间过滤、快速复查、清空历史、最近 N 条裁剪）。
- 建立 CTest 测试基线并纳入 `WordNormalizerTest` / `PhoneticExtractorTest` / `StarDictBackendTest` / `LookupCoordinatorTest` / `QueryHistoryServiceTest` / `AiAssistServiceTest`。
- 接入 AI Assist（DeepSeek OpenAI-Compatible）：设置页新增 AI 配置，启动校验无效配置并自动降级。
- 查词流程改为“两阶段渲染”：先展示基础词典结果，再异步请求 AI 并在卡片底部回填。
- 设置页新增结果卡片持续时间（ms）配置，并持久化到本地设置。
- 结果卡片自动隐藏逻辑支持悬停保持：超时时若鼠标在卡片内则保持显示，移出后自动关闭。
- 结果卡片会在单屏可用区域内自动回正：智能选择锚点象限并在越界时平滑回正，避免窗口被屏幕边缘裁切。
- 移除 display mode 后，设置持久化层会在保存时自动清理历史遗留键 `display_mode`。
- 结果卡片主题实现已从 `ResultCardWidget` 抽离到 `src/ui/theme/` 模块，便于后续扩展与维护。
- 新增 White paper 结果卡片风格：极简白纸基底、作业本横线、纸面纹理与柔和阴影高光（无卷角）。
- 新增 TDD 文档与协作约束：`docs/TDD_CORE.md`（核心原则）与 `docs/TDD_PLAN.md`（滚动计划），并在 `AGENTS.md` 固化执行口径。
- CTest 测试支持按标签分层运行（`unit` / `integration`），并补充 `LookupCoordinatorTest` 关键失败分支覆盖。
- `LookupCoordinator` 在归一化与结果展示链路增加空白防御：查词入参会使用去空白词，词典 `headword` 展示前会去空白，避免误判与文案异常。
- 新增 `LookupCoordinatorTest` 异常组合回归用例，覆盖“归一化结果含空白”“词典 headword 含空白”两条链路。
- `LookupCoordinator` 对“归一化结果含内部空白”的候选词直接判定为无效（`OCR_FAILED`），并阻断后续词典查询链路。
- `AiAssistServiceTest` 新增 AI 降级场景回归：配置无效返回 `InvalidConfiguration`、请求超时返回 `Timeout`，并验证超时后服务仍保持可用。

## 未完成

- Tesseract 尚未切换到 C++ API（当前为 CLI，属于实现差异）。
- StarDict 深度解析尚未完成（`sametypesequence` 精细字段处理）。
- 压缩词典格式尚未支持（`.dict.dz`、`.idx.gz`、`.syn`）。

## 备注

- 当前默认词典路径为 `applicationDirPath()/dict`，默认 tessdata 路径为 `applicationDirPath()/tessdata`。
- 若提示 `Dictionary unavailable`，请先确认 `dict` 目录存在且包含有效 StarDict 文件。
- 若提示 `Tesseract did not start`，请确认 PATH 中放的是 Tesseract 所在目录，或设置 `TESSERACT_EXE`/`TESSERACT_PATH`。
