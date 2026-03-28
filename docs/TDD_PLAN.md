# wordSnapV1 TDD 实施计划（TDD_PLAN）

更新时间：2026-03-28

> 本文件为动态执行计划：每次代码修改后都要同步更新状态。
> 核心原则参照 `docs/TDD_CORE.md`，测试细则参照 `TESTING_GUIDELINES.md`，本文件负责“做什么、做到哪一步、下一步做什么”。

## 1. 当前基线

- 已接入 CTest，现有测试：
  - `WordNormalizerTest`
  - `PhoneticExtractorTest`
  - `StarDictBackendTest`
  - `LookupCoordinatorTest`
  - `QueryHistoryServiceTest`
  - `AiAssistServiceTest`
  - `AiAssistPolicyTest`
  - `ImagePreprocessorTest`
  - `OcrServiceTest`
  - `ResultCardWidgetTest`
  - `E2eSmokeTest`
- 现状问题：
  - 分层标签已覆盖 `unit/integration/ui/e2e`，后续按风险增量补测，避免低价值测试膨胀。
  - 新增测试需遵循“高价值 + 最小化”约束：单函数 3-5 case、优先参数化、避免 trivial 测试。
  - 主链路编排仍有边界路径覆盖不足。
  - UI 自动化目前为最小回归集，复杂交互覆盖仍可扩展。
  - 自 2026-03-27 起执行“先评估再 TDD”：UI/样式改动默认不强制自动化测试。

## 2. 分阶段计划（持续滚动）

### P0（已完成）TDD 规则落地

- [x] 新建 `docs/TDD_CORE.md`，固化项目 TDD 核心原则。
- [x] 新建 `docs/TDD_PLAN.md`，作为滚动执行面板。
- [x] 在 `AGENTS.md` 明确“后续修改需遵循 TDD_CORE”。
- [x] 将 `TESTING_GUIDELINES.md` 纳入测试强约束，并同步到测试相关文档。

验收标准：文档可追踪、规则有唯一来源、Agent 协作口径一致。

### P1（已完成）测试分层与关键分支补齐

- [x] 在 CTest 中为现有测试添加分层标签（unit/integration）。
- [x] 为 `LookupCoordinator` 补齐关键失败分支测试：
  - pipeline 未配置；
  - OCR 归一化后为空。
- [x] 固化按层运行命令并验证通过。

验收标准：

- `ctest --test-dir build -C Debug -L unit --output-on-failure` 可执行。
- `ctest --test-dir build -C Debug -L integration --output-on-failure` 可执行。
- 新增用例通过且覆盖目标分支。

### P2（进行中）主链路集成测试扩展

- [x] 补齐首批“截图->OCR->归一化->查词->结果构建”异常链路组合测试：
  - `normalizeCandidate` 返回前后空白时，查词入参与结果应使用去空白词。
  - 词典 `headword` 含前后空白时，展示文案应使用去空白词。
- [x] 新增“归一化结果含内部空白”失败链路用例：应直接判定为 `OCR_FAILED`，且不进入词典阶段。
- [ ] 继续补齐其余异常链路组合测试（跨模块失败路径）。
  - [x] 新增“预处理结果为空”失败链路用例：应返回 `OCR_FAILED`，且不进入 OCR 识别阶段。
  - [x] 新增“OCR 失败但未返回错误文案”失败链路用例：应回退到默认提示 `Ensure Tesseract is installed.`。
- [x] 补齐 AI 降级断言（service 层）：
  - 功能关闭 -> `Disabled`；
  - 配置无效 -> `InvalidConfiguration`；
  - 请求超时 -> `Timeout`，且服务可用性不被永久置为不可用。
- [x] 增加“词典不可用 + AI 降级”在编排层（AppController）的链路断言：
  - `AiAssistPolicy::Context` 由 `lookupStatus`（而非布尔值）驱动；
  - `DICT_UNAVAILABLE` + AI 服务不可用场景在策略测试中断言为静默跳过；
  - `AppController` 将 `LookupCoordinator::Result::status` 直接透传给策略。
- [x] 提取并覆盖 AI 请求决策策略（`AiAssistPolicy`）：
  - 非 `FOUND` 结果不触发 AI；
  - 即使 AI 服务不可用，非 `FOUND`（如 `DICT_UNAVAILABLE`）仍应静默跳过；
  - 功能关闭时静默跳过；
  - 服务不可用或请求词为空时返回错误态；
  - `cardTitle` 优先、`queryWord` 回退并统一去空白。
- [x] 建立最小测试数据夹具（可复用样本）：
  - 新增 `tests/support/LookupCoordinatorFixture.h` 统一提供默认 OCR/归一化/词典样本与依赖桩；
  - `LookupCoordinatorTest` 关键分支改为复用该夹具，减少重复搭桩噪音。

### P3（已完成）UI/行为自动化最小集

- [x] 为 `ResultCardWidget` 增加关键行为测试（贴边回正、超时隐藏、内容扩展后回正）。
- [x] 建立最小 UI smoke 测试，确保关键窗口可创建与销毁。

### P4（已取消）CI 分层门禁

- [x] 已按当前项目策略移除 GitHub Actions 工作流（`ci-unit-integration` / `nightly-ui-e2e-smoke`）。
- [x] 同步清理 CI 工作流相关文档描述，避免后续执行预期偏差。

### P5（已收口）OCR 鲁棒性最小闭环

- [x] 新增 `ImagePreprocessorTest`（`unit`）覆盖预处理基础契约：
  - 空输入返回空图；
  - 低对比度文本经预处理后仍保留前景/背景可分离性。
- [x] 调整 `ImagePreprocessor` 二值化策略：
  - 在固定阈值基础上，按图像亮度范围自适应计算阈值（`(min + max) / 2`）；
  - 低对比度文本场景优先保证文本不被整体抹白。
- [x] 补齐 `OcrService` 可测边界（`unit`）：
  - 进程启动失败；
  - 进程超时；
  - 进程非零退出且无标准错误时回退默认错误文案；
  - 正常退出但输出为空；
  - 正常返回文本并去空白。
- [x] 为 `OcrService` 引入可注入进程执行器（`ProcessRunner`），将外部进程调用与结果判定解耦，便于 TDD 下稳定模拟失败路径。
- [x] 补齐 `OcrService` 参数组装回归（`unit`）：
  - `tessdataDir` 存在 `eng.traineddata` 时应附加 `--tessdata-dir` 参数；
  - `eng.traineddata` 缺失时不应附加该参数。
- [x] 在编排层补齐 OCR 失败默认降级文案组合链路断言：
  - 识别失败且错误为空时，除默认文案外，明确断言链路已执行到 `preprocess` 与 `recognize`（非前置阶段短路）。
- [x] 完成 `resolveTesseractExecutable` 可测抽离：
  - 新增 `TesseractExecutableResolver`，将运行时快照采集与路径决策逻辑解耦；
  - 补齐环境变量/PATH 分支回归（应用目录优先、PATH 回退、最终命令兜底）。
- [x] 补齐 `QStandardPaths::findExecutable` / 常见安装目录命中分支回归：
  - `discoveredTesseract` 命中时优先于常见安装目录；
  - 未命中发现路径时可回退到 `Program Files` 常见安装目录。
- [x] 补齐 `TESSERACT_PATH` 指向“可执行文件路径（非目录）”场景回归，锁定历史兼容行为。
- [x] 增强 `ImagePreprocessor` 预处理策略（`unit`）：
  - 参数化策略维度（阈值模式/缩放倍率/反色输入）；
  - 增加最多 4 档轻量策略重试与最佳候选回退；
  - 在 Debug 构建输出策略 ID、阈值、重试次数等调试信息。
- [x] 扩展 `ImagePreprocessorTest`（`unit`）高价值回归：
  - 深底浅字场景下输出极性归一化（优先白底黑字）；
  - 复杂灰度输入输出保持二值域（仅 `0/255`）。
- [x] 将 PATH 分隔符（`;`）与兼容防御测试候选迁移到 `docs/FUTURE.md`，按反馈触发再执行。

### P6（已收口，按当前需求）M3-A 样本集与评估基线

- [x] 新增 `docs/Temp/M3-样本清单模板.md`：
  - 固化样本字段字典（场景/视觉/质量/GT/区域说明）；
  - 提供首轮 30 条样本池建议清单，覆盖网页/文档/游戏/终端。
- [x] 新增 `docs/Temp/M3-基线评估记录-20260327.md`：
  - 固化 `success/near_hit/fail` 判定口径；
  - 固化总体成功率、困难子集成功率、P95 时延统计口径；
  - 建立 30 条样本逐项记录表。
- [x] 已完成手工实测子集：`M3A-WEB-01~08` 均快速识别成功（用户验收通过）。
- [x] 将 M3 后续候选项迁移至 `docs/FUTURE.md`，当前迭代先收口。

TDD 评估：本阶段主要为样本与评估流程文档建设，不属于核心代码逻辑，采用手工流程验证。

## 3. 当前迭代执行记录

- 2026-03-27：初始化 TDD 文档体系（P0 完成）。
- 2026-03-27：完成 P1：新增 `unit/integration` 标签并补齐 `LookupCoordinator` 关键失败分支测试。
- 2026-03-27：进入 P2，下一步扩展主链路异常组合集成测试。
- 2026-03-27：P2 首批完成：新增 `LookupCoordinator` 空白输入链路回归测试并修复对应实现。
- 2026-03-27：P2 继续推进：新增“归一化结果含内部空白”回归测试，修复后确保不会误入词典查询。
- 2026-03-27：P2 新增 `AiAssistServiceTest` 降级场景覆盖（Disabled/InvalidConfiguration/Timeout）。
- 2026-03-27：P2 提取 `AiAssistPolicy` 并新增 `AiAssistPolicyTest`，将 AppController 的 AI 触发分支收敛为可单测策略。
- 2026-03-27：P2 新增 `LookupCoordinatorFixture` 测试夹具并在 `LookupCoordinatorTest` 复用，降低后续异常链路用例编写成本。
- 2026-03-27：P2 新增“预处理结果为空”回归测试并修复 `LookupCoordinator`：预处理失败时直接 `OCR_FAILED` 且阻断 OCR 调用。
- 2026-03-27：P2 将 AI 策略上下文从 `lookupFound` 升级为 `lookupStatus`，补齐 `DICT_UNAVAILABLE + AI 不可用` 的编排层断言。
- 2026-03-27：P2 新增“识别失败且错误文案为空”回归测试，锁定默认降级提示文案行为。
- 2026-03-27：启动 P3：新增 `ResultCardWidgetTest`（`ui` 标签）最小 smoke，用例覆盖窗口显示字段与自动隐藏基础行为。
- 2026-03-27：P3 补齐 `ResultCardWidget` 关键行为断言：贴边回正与 AI 内容扩展后回正，完成 UI 最小自动化集。
- 2026-03-27：P4 新增 `E2eSmokeTest` 并接入 `e2e` 标签，形成 `unit/integration/ui/e2e` 分层测试闭环。
- 2026-03-27：启动 P5：新增 `ImagePreprocessorTest` 并引入自适应二值化阈值，提升低对比度 OCR 预处理鲁棒性。
- 2026-03-27：P5 继续推进：新增 `OcrServiceTest`，并将 `OcrService` 进程调用抽象为可注入 `ProcessRunner`，完成 OCR 启动失败/超时/空输出/成功输出边界回归。
- 2026-03-27：P5 扩展 `OcrServiceTest`：补齐 `tessdataDir` 参数组装断言，覆盖 `eng.traineddata` 存在/缺失两条分支。
- 2026-03-27：P5 扩展 `OcrServiceTest`：新增“非零退出 + 空 stderr”回归，锁定默认错误文案 `Tesseract process failed.`。
- 2026-03-27：P5 在 `LookupCoordinatorTest` 增加链路完整性断言，确保“OCR 失败默认提示”场景经过预处理与识别阶段后再降级返回。
- 2026-03-27：P3 回归修复：优化 `ResultCardWidget` 回正动画与 AI loading 动效，消除非边界场景下卡片颤动；新增 UI 用例断言 loading 期间几何稳定。
- 2026-03-27：P5 将 Tesseract 可执行探测抽离为 `TesseractExecutableResolver`，在 `OcrServiceTest` 新增环境变量/PATH/兜底命令回归，降低路径探测改动回归风险。
- 2026-03-27：P5 继续扩展 `OcrServiceTest`：补齐 discovered 可执行路径优先级与常见安装目录命中分支，完善路径探测决策回归矩阵。
- 2026-03-27：P5 补齐 `TESSERACT_PATH` 兼容回归：环境变量直接给出 `tesseract.exe` 文件路径时可被正确识别。
- 2026-03-27：P5 扩展 `ImagePreprocessor` 为多策略重试流程（阈值/缩放/反色可组合），并在 `ImagePreprocessorTest` 新增深底浅字极性归一化与二值域回归用例。
- 2026-03-27：启动 P6（M3-A）：补齐样本清单模板与首轮基线评估记录模板，冻结 30 条样本池并统一评分口径。
- 2026-03-27：P6 手工验证进展：`M3A-WEB-01~08` 快速识别成功，按当前需求先收口 M3 迭代。
- 2026-03-27：将 M3 后续候选（含 PATH 分隔符防御测试）统一迁移至 `docs/FUTURE.md`，待反馈触发。
- 2026-03-27：调整 TDD 执行口径：改为“先评估再 TDD”，核心逻辑优先 TDD，UI/样式类改动可采用手工验证并在计划中留痕。
- 2026-03-27：P3 UI 缺陷修复（非 TDD）：修复 AI loading 动效不可见问题（由透明度动画在 QLabel 富文本中表现不稳定导致），改为“颜色 + 字符位移帧”方案；验证方式为本地构建通过与手工 UI 观察。
- 2026-03-27：P3 UI 调整（非 TDD）：按产品反馈撤回“颜色 + 字符位移帧”方案，恢复原始三点透明度 loading 动画；验证方式为本地构建通过与手工 UI 观察。
- 2026-03-27：P3 UI 调整（非 TDD）：增强三点 loading 的跳跃感，活动点恢复“上浮+放大”效果，同时保持卡片几何稳定策略；验证方式为本地构建通过与手工 UI 观察。
- 2026-03-27：按当前项目策略移除 GitHub Actions 工作流 `ci-unit-integration` 与 `nightly-ui-e2e-smoke`，并清理 `README` / `requirements-status` / `TDD_PLAN` 中相关说明。
- 2026-03-27：新增 `TESTING_GUIDELINES.md` 并将其纳入测试执行强约束，同步更新 `AGENTS.md` / `TDD_CORE.md` / `README.md` / `PLAN.md` / `requirements-status.md`。
- 2026-03-28：M4 文档一致性评估（非 TDD）：按最新决策将“发布清单文件”口径收敛为“写入 GitHub Release notes”，并同步 `docs/PLAN.md`、`docs/FUTURE.md` 与 `docs/Temp/2026-03-28-M4-windows-packaging-release-plan.md`。
- 2026-03-28：M4 脚本落地（非 TDD）：新增 `scripts/release/windows/package-win.ps1`、`scripts/release/windows/publish-release.ps1` 与 `installer/windows/wordSnapV1.iss` 初版。评估结论：本轮为发布工程脚本/安装器编排，不属于核心业务逻辑，采用脚本语法校验 + 手工发布演练策略（已完成 PowerShell 语法解析通过）。

## 4. 本轮完成后更新规则

- 每次提交后，必须更新以下三项：
  1. 当前阶段状态（已完成/进行中/待开始）；
  2. 新增或调整的测试清单；
  3. 下一步可直接执行的最小动作。
