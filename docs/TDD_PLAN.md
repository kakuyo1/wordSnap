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
- 现状问题：
  - 测试分层标签不完整（unit/integration/ui/e2e 未明确）。
  - 主链路编排仍有边界路径覆盖不足。
  - UI 边界行为尚缺自动化回归。

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
- [x] 补齐 AI 降级断言（service 层）：
  - 功能关闭 -> `Disabled`；
  - 配置无效 -> `InvalidConfiguration`；
  - 请求超时 -> `Timeout`，且服务可用性不被永久置为不可用。
- [ ] 增加“词典不可用 + AI 降级”在编排层（AppController）的链路断言。
- [ ] 建立最小测试数据夹具（可复用样本）。

### P3（待开始）UI/行为自动化最小集

- [ ] 为 `ResultCardWidget` 增加关键行为测试（贴边回正、超时隐藏、内容扩展后回正）。
- [ ] 建立最小 UI smoke 测试，确保关键窗口可创建与销毁。

### P4（待开始）CI 分层门禁

- [ ] PR 必跑：`unit + integration`。
- [ ] 夜间/发布前：`ui + e2e smoke`。
- [ ] 失败归因模板与回滚策略文档化。

## 3. 当前迭代执行记录

- 2026-03-27：初始化 TDD 文档体系（P0 完成）。
- 2026-03-27：完成 P1：新增 `unit/integration` 标签并补齐 `LookupCoordinator` 关键失败分支测试。
- 2026-03-27：进入 P2，下一步扩展主链路异常组合集成测试。
- 2026-03-27：P2 首批完成：新增 `LookupCoordinator` 空白输入链路回归测试并修复对应实现。
- 2026-03-27：P2 继续推进：新增“归一化结果含内部空白”回归测试，修复后确保不会误入词典查询。
- 2026-03-27：P2 新增 `AiAssistServiceTest` 降级场景覆盖（Disabled/InvalidConfiguration/Timeout）。

## 4. 本轮完成后更新规则

- 每次提交后，必须更新以下三项：
  1. 当前阶段状态（已完成/进行中/待开始）；
  2. 新增或调整的测试清单；
  3. 下一步可直接执行的最小动作。
