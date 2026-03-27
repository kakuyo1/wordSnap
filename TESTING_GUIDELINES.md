# 项目测试指南（TESTING_GUIDELINES.md）

## 1. 测试哲学（核心约束）
- **只写高价值测试**：优先测试**业务逻辑、公共 API、关键路径、边缘 case**。**禁止** 测试 trivial 函数（getter/setter、简单赋值、UI 布局 glue code）。
- **测试金字塔**：80% 单元测试（QTest/Google Test），15% 集成测试，5% 回归测试。只测“会坏”的地方。
- **回归测试**：只为 bug fix / 新功能 / 变更点写。已通过的旧行为不需要额外回归测试。
- **最小化测试量**：一个函数最多 3-5 个 test case（正常+2个边界+1个异常）。用 parameterized test / data-driven test 减少重复。
- **不测实现细节**：只测**公开接口行为**，不测 private 方法、不 mock 内部状态（除非必要）。

## 2. Qt/C++ 特定规则
- 使用 **QTest** 框架（优先）或 Google Test。
- 测试命名：`test_函数名_场景`（如 `test_calculateDiscount_normalCase`）。
- 必须包含：`QCOMPARE` / `QVERIFY` / `QSignalSpy`（信号必须测）。
- 边界 case：空 QString、负数、最大值、Qt::Invalid 等。
- 禁止：测试 QWidget 样式、简单信号槽连接（除非核心逻辑）。
- Mock：只用 Google Mock 或 QTest 的 stub，**避免过度 mock**。

## 3. 输出格式要求
- 只输出测试代码 + 必要注释。
- 每个测试开头写一行中文/英文注释说明“为什么测这个”。
- 测试文件放 `tests/` 目录，命名 `tst_xxx.cpp`。
- **禁止** 生成多余的 fixture/setup（除非真的需要共享）。

## 4. 覆盖率目标
- 核心模块目标 80%+，非核心 60% 即可。
- 用 coverage 工具验证后才接受测试。

每次生成测试前，必须严格遵守本文件。