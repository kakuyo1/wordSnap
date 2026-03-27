# wordSnapV1 架构说明

更新时间：2026-03-27

## 1. 架构目标

- 保持主链路清晰：截图 -> OCR -> 归一化 -> 查词 -> 展示。
- 让平台相关实现（Win32）与业务逻辑解耦，便于后续跨平台。
- 为 AI、历史、性能治理等新增能力预留可扩展边界。

## 2. 当前整体架构（运行视角）

```mermaid
flowchart LR
    U[用户] --> HK[全局热键 / 托盘菜单]
    HK --> AC[AppController]

    AC --> CO[CaptureOverlay]
    CO --> SC[ScreenCaptureService]
    SC --> IP[ImagePreprocessor]
    IP --> OCR[OcrService]
    OCR --> WN[WordNormalizer]

    WN --> DS[DictionaryService]
    DS --> SB[StarDictBackend]
    SB --> DICT[(.ifo / .idx / .dict)]

    OCR --> TESS[(tesseract.exe)]

    AC --> RC[ResultCardWidget]
    AC --> TT[QToolTip]
    AC --> TC[TrayController]
    AC --> AI[AiAssistService]
    AI --> NET[QNetworkAccessManager]
    NET --> DSAPI[(DeepSeek API)]

    AC --> SS[SettingsService]
    SS --> QS[(QSettings)]

    AC --> GHM[GlobalHotkeyManager]
    GHM --> WINAPI[(RegisterHotKey / WM_HOTKEY)]
```

## 3. 当前模块架构（代码组织视角）

```mermaid
flowchart TB
    subgraph UI[UI 层 src/ui]
        U1[TrayController]
        U2[CaptureOverlay]
        U3[ResultCardWidget]
        U4[SettingsDialog]
        U5[theme/ResultCardTheme]
    end

    subgraph APP[应用编排层 src/app]
        A1[AppController]
        A2[AppTypes]
    end

    subgraph SRV[服务层 src/services]
        S1[ScreenCaptureService]
        S2[ImagePreprocessor]
        S3[OcrService]
        S4[WordNormalizer]
        S5[DictionaryService]
        S6[StarDictBackend]
        S7[SettingsService]
        S8[AiAssistService]
    end

    subgraph PLAT[平台层 src/platform/win]
        P1[GlobalHotkeyManager]
    end

    subgraph EXT[外部依赖]
        E1[Qt6 Widgets]
        E2[Tesseract CLI]
        E3[StarDict 文件]
        E4[Win32 API]
    end

    U1 --> A1
    U2 --> A1
    U3 --> A1
    U4 --> A1
    U5 --> U3

    A1 --> S1
    A1 --> S2
    A1 --> S3
    A1 --> S4
    A1 --> S5
    A1 --> S7
    A1 --> S8
    A1 --> P1

    S5 --> S6

    S3 --> E2
    S6 --> E3
    S8 --> E5[HTTP API]
    P1 --> E4

    UI --> E1
    APP --> E1
    SRV --> E1
```

## 4. 查词主流程时序图

```mermaid
sequenceDiagram
    participant User as 用户
    participant GHM as GlobalHotkeyManager
    participant AC as AppController
    participant CO as CaptureOverlay
    participant SC as ScreenCaptureService
    participant IP as ImagePreprocessor
    participant OCR as OcrService
    participant WN as WordNormalizer
    participant DS as DictionaryService
    participant SB as StarDictBackend
    participant RC as ResultCardWidget
    participant TC as TrayController
    participant AI as AiAssistService

    User->>GHM: 按下全局热键
    GHM->>AC: hotkeyPressed()
    AC->>CO: beginCapture()
    User->>CO: 框选区域
    CO->>AC: regionSelected(rect)

    AC->>SC: capture(rect)
    SC-->>AC: capturedImage
    AC->>IP: preprocessForWordRecognition(image)
    IP-->>AC: preprocessedImage

    AC->>OCR: recognizeSingleWord(image, tessdata)
    OCR-->>AC: OcrWordResult
    AC->>WN: normalizeCandidate(rawText)
    WN-->>AC: normalizedWord

    AC->>DS: lookup(word)
    DS->>SB: lookup(word)
    SB-->>DS: DictionaryEntry
    DS-->>AC: DictionaryEntry

    AC->>RC: showMessage(headword, body, phonetic, ...)
    AC->>TC: showInfo(...)
    AC->>RC: showAiLoading()
    AC->>AI: requestWordAssist(word)
    AI-->>AC: structured result / error
    AC->>RC: showAiContent(...) or showAiError(...)
```

## 5. 目标扩展架构（规划态）

```mermaid
flowchart LR
    subgraph Core[核心链路]
        AC2[AppController]
        OCR2[OCR Pipeline]
        DICT2[Dictionary Pipeline]
        UI2[ResultCard / Tray]
    end

    subgraph Ext[扩展能力]
        AI[AiAssistService]
        HIS[HistoryService]
        DIAG[DiagnosticsService]
        PKG[Packaging Scripts]
    end

    subgraph Infra[基础设施]
        CFG[SettingsService]
        NET[QNetworkAccessManager]
        DB[(SQLite/JSONL)]
        LOG[(Structured Logs)]
    end

    AC2 --> OCR2
    AC2 --> DICT2
    AC2 --> UI2

    AC2 --> AI
    AC2 --> HIS
    AC2 --> DIAG

    AI --> NET
    HIS --> DB
    DIAG --> LOG

    PKG --> UI2
    PKG --> CFG
```

## 6. 模块职责与边界

| 模块 | 职责 | 允许依赖 | 不应承担 |
|---|---|---|---|
| `AppController` | 业务流程编排、状态分派 | `ui/`, `services/`, `platform/win` | 复杂解析细节、平台 API 细节 |
| `ui/*` | 展示与用户交互 | `AppTypes`、Qt UI | 业务流程编排、数据持久化 |
| `services/OcrService` | OCR 调用与错误处理 | Qt Core、Tesseract CLI | 词典解析、UI 文案拼装 |
| `services/StarDictBackend` | 词典加载与查询 | Qt Core、文件系统 | UI 逻辑、热键逻辑 |
| `services/SettingsService` | 配置读写 | QSettings | 运行时业务决策 |
| `platform/win/*` | Win32 适配 | Win32 API | 业务展示与查词逻辑 |

## 7. 架构约束（当前）

- 维持 Windows-first：Win32 仅留在 `src/platform/win`。
- 不在核心链路引入异常机制，沿用返回值 + 错误消息模式。
- 新增能力（AI/历史/诊断）优先以 Service 方式接入，避免把复杂逻辑堆入 `AppController`。
- 任何跨层调用要先判断是否破坏边界；破坏边界的改动必须先做接口抽象。
