# wordSnapV1 TDD 实施计划（TDD_PLAN）

更新时间：2026-03-27

> 本文件为动态执行计划：每次代码修改后都要同步更新状态。
> 核心原则参照 `docs/TDD_CORE.md`，本文件负责“做什么、做到哪一步、下一步做什么”。

## 1. 当前基线

- 已接入 CTest，现有测试：
  - `WordNormalizerTest`
  - `PhoneticExtractorTest`
  - `StarDictBackendTest`
  - `LookupCoordinatorTest`
  - `QueryHistoryServiceTest`
  - `AiAssistServiceTest`
  - `AiAssistPolicyTest`
  - `ResultCardWidgetTest`
- 现状问题：
  - 测试分层标签尚未覆盖 e2e。
  - 主链路编排仍有边界路径覆盖不足。
  - UI 自动化目前为最小回归集，复杂交互覆盖仍可扩展。

## 2. 分阶段计划（持续滚动）

### P0（已完成）TDD 规则落地

- [x] 新建 `docs/TDD_CORE.md`，固化项目 TDD 核心原则。
- [x] 新建 `docs/TDD_PLAN.md`，作为滚动执行面板。
- [x] 在 `AGENTS.md` 明确“后续修改需遵循 TDD_CORE”。

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

### P4（进行中）CI 分层门禁

- [x] PR 必跑：`unit + integration`。
- [ ] 夜间/发布前：`ui + e2e smoke`。
  - [x] 已新增 nightly/workflow_dispatch 工作流，执行 `ui` 标签测试并在存在 `e2e` 标签测试时自动执行。
- [x] 失败归因模板与回滚策略文档化。

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
- 2026-03-27：启动 P4：新增 GitHub Actions 工作流 `.github/workflows/ci-unit-integration.yml`，对 PR/main push 执行 `unit + integration` 分层门禁。
- 2026-03-27：P4 新增 `.github/workflows/nightly-ui-e2e-smoke.yml`，按夜间与手动触发执行 UI smoke，并在存在 e2e 测试时补跑 e2e smoke。
- 2026-03-27：P4 新增 `docs/CI_FAILURE_PLAYBOOK.md`，固化 CI 失败归因模板与回滚执行策略。

## 4. 本轮完成后更新规则

- 每次提交后，必须更新以下三项：
  1. 当前阶段状态（已完成/进行中/待开始）；
  2. 新增或调整的测试清单；
  3. 下一步可直接执行的最小动作。
