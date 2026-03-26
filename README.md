# wordSnapV1

`wordSnapV1` 是一个 Windows 优先的离线托盘取词工具（V1）。

核心链路：全局热键/托盘触发截图 -> OCR 识别 -> 单词归一化 -> StarDict 查词 -> 鼠标附近结果卡片展示。

## 1. 当前能力

- 已具备可运行闭环：托盘常驻、全局热键、框选截图、OCR、词典查询、结果展示。
- 结果卡片支持四种风格（Kraft paper / Glassmorphism / Terminal / Clay）与透明度设置。
- 结果状态统一为 `FOUND` / `OCR_FAILED` / `UNKNOWN` / `DICT_UNAVAILABLE`，卡片与托盘文案保持一致。
- 已支持查询历史 V1：本地 JSONL 持久化、按词/时间过滤、快速复查、清空历史、最近 N 条限制（默认 300，可配置）。
- 设置页可配置热键、显示模式、词典目录、tessdata 目录、卡片风格、历史条数上限。
- 启动时可自动探测 Tesseract 路径并给出可读错误提示。

当前项目状态详见：`docs/requirements-status.md`。

## 2. 技术栈

- C++17
- Qt 6 Widgets
- CMake
- Tesseract OCR（当前为 CLI 调用）
- StarDict（当前支持 `.ifo + .idx + .dict`）

## 3. 环境要求

- Windows 10/11
- CMake >= 3.21
- Qt 6.5+（Core / Gui / Widgets）
- C++17 编译器（MSVC 或 MinGW）

## 4. 构建与运行

### 4.1 使用多配置生成器（Visual Studio）

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

运行：

```powershell
.\build\Debug\wordSnapV1.exe
```

### 4.2 使用单配置生成器（Ninja）

```powershell
cmake -S . -B build -G "Ninja"
cmake --build build
```

运行产物通常位于 `build/` 目录。

## 5. 运行前准备

### 5.1 安装与配置 Tesseract

- 安装 Tesseract，并确保 `tesseract.exe` 可被程序发现。
- 推荐将“安装目录”加入 PATH（不要只放 `tesseract.exe` 文件路径）。
- 可选环境变量：
  - `TESSERACT_EXE`：直接指向 `tesseract.exe`
  - `TESSERACT_PATH`：可指向目录或 `tesseract.exe`
- 若使用自定义模型，请保证 `eng.traineddata` 可在配置的 `tessdata` 目录中找到。

### 5.2 准备 StarDict 词典

- 默认读取应用目录下 `dict/`。
- 当前支持：`xxx.ifo + xxx.idx + xxx.dict`。
- 若只有 `.dict.dz`，需先解压为 `.dict`。

## 6. 使用说明

1. 启动程序后常驻系统托盘。
2. 默认热键为 `Shift+Alt+S`（失败时会尝试回退为 `Ctrl+Alt+S`）。
3. 按热键后框选目标区域，系统自动执行 OCR + 查词。
4. 查询结果会通过结果卡片、Tooltip、托盘消息呈现。
5. 可在托盘 `Settings` 中调整主要参数。

## 7. 文档导航

- `docs/TODOLIST.md`：原始待办清单。
- `docs/PLAN.md`：可执行里程碑规划（已细化到验收标准）。
- `docs/ARCHITECTURE.md`：项目整体架构与模块架构（Mermaid 图）。
- `docs/requirements-status.md`：需求完成度对照。
- `AGENTS.md`：面向编码 Agent 的协作规范。

## 8. 目录结构

```text
wordSnapV1/
  CMakeLists.txt
  AGENTS.md
  README.md
  src/
    app/
    platform/win/
    services/
    ui/
  docs/
    TODOLIST.md
    PLAN.md
    ARCHITECTURE.md
    requirements-status.md
```

## 9. 常见问题

- `Dictionary unavailable`
  - 检查词典目录是否存在并包含完整的 `.ifo/.idx/.dict` 组合。
- `Tesseract did not start`
  - 检查 PATH、`TESSERACT_EXE`、`TESSERACT_PATH` 配置是否有效。
- 热键无响应
  - 可能与其他程序冲突；请在设置中改为其他组合键。

## 10. 路线图概览

- 建立自动化测试与 TDD 基线（优先级最高）。
- 完善 ResultCard 风格和状态规范，增加查询历史功能。
- 接入 DeepSeek（异步加载、失败降级、结构化输出）。
- 提升 OCR 在游戏/网页场景下的鲁棒性。
- 打通 `windeployqt + Inno Setup` 发布流程。

详细计划请查看：`docs/PLAN.md`。
