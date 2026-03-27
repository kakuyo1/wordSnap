# AGENTS.md

面向在 `wordSnapV1` 仓库中工作的编码 Agent 协作说明（中文）。

## 0.用户偏好

- 所有回答使用中文
- 每一轮回答后若涉及到有关文档，则需要对该文档进行更新
- 每次单独修改/增加模块产生的规划md文件默认写到docs/Temp下，这些文件不允许上传到远端仓库
- 每完成一轮代码修改后，按当前变更整理并提交一条本地 commit（默认不 push）
- 后续功能/修复默认遵循 TDD（以 `docs/TDD_CORE.md` 为准），并实时更新 `docs/TDD_PLAN.md`

## 1. 规则来源与优先级

- 首要规则来源：本文件。
- 已检查以下附加规则文件（当前均不存在）：
  - `.cursor/rules/`
  - `.cursorrules`
  - `.github/copilot-instructions.md`
- 若后续出现上述文件，需将其视为强约束并合并执行。

## 2. 项目定位

- 技术栈：C++17 + Qt6 Widgets + CMake。
- 产品定位：Windows-first 托盘常驻取词工具。
- 主链路：截图 -> 预处理 -> OCR -> 归一化 -> StarDict 查词 -> 卡片/托盘展示。
- 入口：`src/main.cpp`。
- 编排中枢：`src/app/AppController.cpp`。

## 3. 仓库结构速览

- `src/app/`：应用级编排与共享类型。
- `src/ui/`：托盘、截图覆盖层、设置对话框、结果卡片。
- `src/services/`：截图/OCR/归一化/词典/设置等服务。
- `src/platform/win/`：Win32 热键适配层。
- `docs/`：需求状态、规划、架构文档。
- `CMakeLists.txt`：可执行目标 `wordSnapV1`。

## 4. Agent 标准工作流（建议）

1. 先读需求、`docs/TDD_CORE.md`、`docs/TDD_PLAN.md` 与相关代码，再做最小改动方案。
2. 优先保持现有行为兼容，除非需求明确要求改行为。
3. 按 `Red -> Green -> Refactor` 执行：先写失败测试，再实现，再重构。
4. 改代码后先构建，再运行相关自动化测试（至少覆盖本次改动影响面）。
5. 同步更新文档（`README.md` / `AGENTS.md` / `docs/*`，且 `TDD_PLAN` 必须更新进度）。
6. 输出时写清：改了什么、为什么改、如何验证。

## 5. 构建与运行命令

默认在仓库根目录执行。

### 5.1 配置

```powershell
cmake -S . -B build
```

或（Ninja）：

```powershell
cmake -S . -B build -G "Ninja"
```

### 5.2 构建

多配置生成器（如 VS）：

```powershell
cmake --build build --config Debug
```

单配置生成器（如 Ninja）：

```powershell
cmake --build build
```

### 5.3 运行

```powershell
.\build\Debug\wordSnapV1.exe
```

说明：某些环境下构建末尾可能出现 `pwsh.exe` 提示；若可执行文件已产出，可先视为非阻塞告警。

## 6. 测试策略与命令（重点）

当前现状：

- 已接入 CTest，当前已有 6 个测试 target（详见 `tests/`）。
- TDD 核心原则见 `docs/TDD_CORE.md`，滚动实施计划见 `docs/TDD_PLAN.md`。

统一命令：

```powershell
ctest --test-dir build --output-on-failure
```

按层运行（推荐）：

```powershell
ctest --test-dir build -C Debug -L unit --output-on-failure
ctest --test-dir build -C Debug -L integration --output-on-failure
```

单测（必须支持，示例）：

```powershell
ctest --test-dir build -R "^WordNormalizerTest$" --output-on-failure
```

多配置下单测：

```powershell
ctest --test-dir build -C Debug -R "^WordNormalizerTest$" --output-on-failure
```

查看已发现测试：

```powershell
ctest --test-dir build -N
```

## 7. 代码风格与实现约束

### 7.1 通用编码风格

- `.cpp` 中先包含对应头文件，再按项目/Qt/STL 分组 include。
- 缩进 4 空格，不使用 Tab。
- 控制流显式花括号，优先 early return。
- 仅在必要处添加注释，避免“解释代码表面含义”的冗余注释。

### 7.2 类型与错误处理

- Qt 边界优先 Qt 类型（`QString`、`QByteArray`、`QRect` 等）。
- 非 trivial 参数优先 `const T&`。
- 继续使用返回值 + `QString* errorMessage` 模式，不引入异常。
- 保持降级能力（例如词典不可用时仍可 OCR 提示）。

### 7.3 命名规范

- 类型：`PascalCase`。
- 函数：`lowerCamelCase`。
- 成员变量：尾随下划线（`foo_`）。
- 常量：`k` 前缀（`kKeyHotkey`）。

### 7.4 Qt / UI 约束

- 尊重 QObject 父子关系，避免悬空对象。
- 信号槽连接尽量集中在构造或初始化阶段。
- 结果卡片为浮层小窗，避免引入抢焦点行为。
- 扩展设置项时，保持历史 key 的兼容性。

### 7.5 平台边界

- Win32 相关逻辑仅放在 `src/platform/win/`。
- 非 Windows 路径显式 `Q_UNUSED(...)` 并返回合理 fallback。
- 未经明确需求，不扩张“已支持平台”的对外承诺。

## 8. 文档规范（本仓库统一中文）

- 新增/更新文档默认使用中文。
- `docs/ARCHITECTURE.md` 使用 Mermaid 维护整体与模块架构图。
- `docs/PLAN.md` 维护里程碑、验收标准、回退方案。
- `README.md` 保持“快速上手 + 当前能力 + 文档导航”结构。
- 若行为变化影响用户体验，必须同步更新 README 对应章节。

## 9. 路线图对齐（开发优先级）

以 `docs/PLAN.md` 为准，建议按以下顺序推进：

1. M0：先补测试骨架与核心单测。
2. M1：结果卡片风格、状态规范、历史查询。
3. M2：DeepSeek 接入（异步 + 降级 + 输出约束）。
4. M3：OCR 鲁棒性与全屏/热键冲突治理。
5. M4/M5：打包发布、性能与稳定性收口。
6. M6：跨平台可行性预研。

## 10. 变更纪律

- 优先小步提交，避免一次性大改。
- 未明确要求时，不新增第三方依赖或重型工具链。
- 不随意做仓库级格式化，减少无关 diff。
- 不得无授权修改安全/网络/计费相关行为。
- 不得回退或覆盖用户已有改动（除非明确要求）。

## 11. 交付前自检清单

- 代码是否可构建。
- 新逻辑是否有对应验证（测试或明确手工步骤）。
- 错误路径是否有可读提示并可降级。
- 文档是否同步（README/PLAN/ARCHITECTURE/AGENTS）。
- 变更是否保持平台边界和模块边界。
