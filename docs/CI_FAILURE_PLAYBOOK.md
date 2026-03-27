# CI 失败归因与回滚模板

更新时间：2026-03-27

## 1. 使用范围

- 适用于 GitHub Actions 的分层测试工作流：
  - `ci-unit-integration.yml`
  - `nightly-ui-e2e-smoke.yml`
- 目标：在 CI 失败时快速定位、标准化记录、可控回滚。

## 2. 快速分诊（10 分钟内）

1. 确认失败层级：`unit` / `integration` / `ui` / `e2e`。
2. 判断是否可复现：本地执行同命令重跑。
3. 判断失败类型：
   - 代码逻辑回归；
   - 测试波动（flaky）；
   - 环境依赖问题（Qt/网络/Runner 资源）。
4. 记录最小证据：失败测试名、首个断言、对应提交哈希。

## 3. 标准排查命令

```powershell
ctest --test-dir build -C Debug -L unit --output-on-failure
ctest --test-dir build -C Debug -L integration --output-on-failure
ctest --test-dir build -C Debug -L ui --output-on-failure
ctest --test-dir build -C Debug -L e2e --output-on-failure
```

针对疑似波动用例（示例）：

```powershell
ctest --test-dir build -C Debug -R "^ResultCardWidgetTest$" --output-on-failure --repeat until-fail:5
```

## 4. 失败归因模板

```markdown
### CI 失败归因记录

- Workflow: <workflow-name>
- Commit: <commit-hash>
- Layer: <unit|integration|ui|e2e>
- Failed Test(s): <test-name-list>
- First Error: <first-assert-or-stack>
- Reproducible Locally: <yes/no>
- Root Cause: <logic|flaky|environment>
- Scope: <affected-modules>
- Fix Plan: <one-sentence>
- ETA: <time>
```

## 5. 回滚策略

优先级从高到低：

1. **小步修复优先**：直接提交修复 commit，保持门禁开启。
2. **单测隔离**：若确认 flaky，先修复测试稳定性；不得无理由删除断言。
3. **功能降级开关**：仅对可降级路径生效（例如 AI 辅助），不得影响主链路可用性。
4. **紧急回滚提交**：当主分支持续红灯且短时无法修复时，回滚最近引入回归的 commit。

## 6. 禁止项

- 禁止通过关闭 CI 或移除测试绕过问题。
- 禁止未定位根因就永久放宽断言。
- 禁止在无记录的情况下执行临时修补。

## 7. 结案标准

- 相关层级测试恢复全绿。
- 已补充必要回归测试（避免同类问题复发）。
- `docs/TDD_PLAN.md` 同步更新本次事件与处置结果。
