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
- 建立 CTest 测试基线并纳入 `WordNormalizerTest` / `PhoneticExtractorTest` / `StarDictBackendTest` / `LookupCoordinatorTest` / `QueryHistoryServiceTest` / `AiAssistServiceTest` / `AiAssistPolicyTest` / `ResultCardWidgetTest` / `E2eSmokeTest`。
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
- 提取 AppController 的 AI 触发决策为 `AiAssistPolicy`，并新增 `AiAssistPolicyTest` 覆盖“触发/跳过/错误态/词源回退”分支；其中明确约束 `DICT_UNAVAILABLE` 等非 `FOUND` 路径即使 AI 不可用也应静默跳过。
- `AiAssistPolicy` 上下文由布尔 `lookupFound` 升级为 `lookupStatus`，避免编排映射歧义；`AppController` 直接透传查词状态供策略决策。
- 新增可复用测试夹具 `LookupCoordinatorFixture`（`tests/support/LookupCoordinatorFixture.h`），并在 `LookupCoordinatorTest` 复用默认样本与依赖桩，降低主链路异常用例维护成本。
- `LookupCoordinator` 新增预处理空图防御：当预处理结果为空时直接返回 `OCR_FAILED`，并阻断后续 OCR 调用；对应回归测试已补齐。
- 新增 `ResultCardWidgetTest`（`ui` 标签）最小 smoke 回归：覆盖结果卡片基础字段渲染与自动隐藏行为，确保关键窗口创建/显示/销毁主流程可回归验证。
- `ResultCardWidgetTest` 追加关键行为回归：覆盖贴边回正与 AI 内容扩展后的回正断言，防止浮层因尺寸变化越界。
- 新增 GitHub Actions 门禁工作流 `ci-unit-integration.yml`：在 PR 与 `main` push 自动执行构建以及 `unit + integration` 分层测试。
- 新增夜间 smoke 工作流 `nightly-ui-e2e-smoke.yml`：按计划任务/手动触发执行 `ui` 标签测试，并在存在 `e2e` 标签测试时自动补跑 `e2e`。
- 新增 `docs/CI_FAILURE_PLAYBOOK.md`：统一 CI 失败归因记录字段、排查命令与回滚策略，形成标准化处置闭环。
- 新增 `E2eSmokeTest`（`e2e` 标签）并打通夜间 smoke 触发链路，当前分层标签已覆盖 `unit/integration/ui/e2e`。
- 新增 `ImagePreprocessorTest`（`unit`）并将 OCR 预处理二值化升级为“固定阈值 + 亮度范围自适应阈值”，提升低对比度文本保真度。
- 新增 `OcrServiceTest`（`unit`），并为 `OcrService` 引入可注入 `ProcessRunner`，可稳定回归 OCR 启动失败/超时/非零退出默认错误文案/空输出，以及 `tessdataDir` 参数组装分支。
- 将 Tesseract 路径探测逻辑抽离为 `TesseractExecutableResolver`，并在 `OcrServiceTest` 增加应用目录优先、环境变量/PATH 回退与命令兜底分支回归。
- `LookupCoordinatorTest` 在“OCR 失败且错误为空”场景补充链路完整性断言：明确经过预处理与识别阶段后再回退默认提示文案。
- 修复结果卡片颤动问题：回正动画在小位移场景直接落位，AI loading 点动画改为等宽等字号透明度切换，避免非边界场景的持续抖动。
- `ResultCardWidgetTest` 新增 UI 回归：断言 AI loading 期间卡片几何保持稳定（远离屏幕边缘场景）。

## 未完成

- Tesseract 尚未切换到 C++ API（当前为 CLI，属于实现差异）。
- StarDict 深度解析尚未完成（`sametypesequence` 精细字段处理）。
- 压缩词典格式尚未支持（`.dict.dz`、`.idx.gz`、`.syn`）。

## 备注

- 当前默认词典路径为 `applicationDirPath()/dict`，默认 tessdata 路径为 `applicationDirPath()/tessdata`。
- 若提示 `Dictionary unavailable`，请先确认 `dict` 目录存在且包含有效 StarDict 文件。
- 若提示 `Tesseract did not start`，请确认 PATH 中放的是 Tesseract 所在目录，或设置 `TESSERACT_EXE`/`TESSERACT_PATH`。
