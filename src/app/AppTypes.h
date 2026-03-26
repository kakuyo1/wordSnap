#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>

// Definition display mode for dictionary entries.
enum class DisplayMode {
    Zh,
    En,
    Bilingual
};

// Result card visual style.
enum class ResultCardStyle {
    KraftPaper,
    Glassmorphism,
    Terminal,
    Clay
};

constexpr int kMinResultCardOpacityPercent = 35;
constexpr int kMaxResultCardOpacityPercent = 100;
constexpr int kDefaultQueryHistoryLimit = 300;
constexpr int kMinQueryHistoryLimit = 50;
constexpr int kMaxQueryHistoryLimit = 2000;
constexpr int kDefaultAiTimeoutMs = 8000;
constexpr int kMinAiTimeoutMs = 1000;
constexpr int kMaxAiTimeoutMs = 30000;

inline int clampResultCardOpacityPercent(const int value) {
    if (value < kMinResultCardOpacityPercent) {
        return kMinResultCardOpacityPercent;
    }
    if (value > kMaxResultCardOpacityPercent) {
        return kMaxResultCardOpacityPercent;
    }
    return value;
}

inline int clampQueryHistoryLimit(const int value) {
    if (value < kMinQueryHistoryLimit) {
        return kMinQueryHistoryLimit;
    }
    if (value > kMaxQueryHistoryLimit) {
        return kMaxQueryHistoryLimit;
    }
    return value;
}

inline int clampAiTimeoutMs(const int value) {
    if (value < kMinAiTimeoutMs) {
        return kMinAiTimeoutMs;
    }
    if (value > kMaxAiTimeoutMs) {
        return kMaxAiTimeoutMs;
    }
    return value;
}

// OCR output for a single-word recognition pipeline.
struct OcrWordResult {
    QString rawText;
    QString normalizedText;
    float confidence{0.0F};
    bool success{false};
};

// Normalized dictionary entry model used by UI.
struct DictionaryEntry {
    QString headword;
    QString phoneticUk;
    QString phoneticUs;
    QString phonetic;
    QString partOfSpeech;
    QStringList definitionsZh;
    QStringList definitionsEn;
    QString etymology;
    QString rawHtml;
    bool found{false};
};

struct AiAssistContent {
    QString definitionEn;
    QString roots;
    QString etymology;
};

// Application-level persistent settings.
struct AppSettings {
    QString hotkey{QStringLiteral("Shift+Alt+S")};
    DisplayMode displayMode{DisplayMode::Bilingual};
    QString starDictDir;
    QString tessdataDir;
    int resultCardOpacityPercent{92};
    ResultCardStyle resultCardStyle{ResultCardStyle::KraftPaper};
    int queryHistoryLimit{kDefaultQueryHistoryLimit};
    bool aiAssistEnabled{false};
    QString aiApiKey;
    QString aiBaseUrl{QStringLiteral("https://api.deepseek.com/v1/chat/completions")};
    QString aiModel{QStringLiteral("deepseek-chat")};
    int aiTimeoutMs{kDefaultAiTimeoutMs};
};

// Converts display mode enum to stable settings string.
inline QString displayModeToString(const DisplayMode mode) {
    switch (mode) {
    case DisplayMode::Zh:
        return QStringLiteral("zh");
    case DisplayMode::En:
        return QStringLiteral("en");
    case DisplayMode::Bilingual:
        return QStringLiteral("bilingual");
    }

    return QStringLiteral("bilingual");
}

// Parses display mode from settings string.
inline DisplayMode displayModeFromString(QString value) {
    value = value.trimmed().toLower();
    if (value == QStringLiteral("zh")) {
        return DisplayMode::Zh;
    }
    if (value == QStringLiteral("en")) {
        return DisplayMode::En;
    }
    return DisplayMode::Bilingual;
}

// Converts result card style enum to stable settings string.
inline QString resultCardStyleToString(const ResultCardStyle style) {
    switch (style) {
    case ResultCardStyle::KraftPaper:
        return QStringLiteral("kraft_paper");
    case ResultCardStyle::Glassmorphism:
        return QStringLiteral("glassmorphism");
    case ResultCardStyle::Terminal:
        return QStringLiteral("terminal");
    case ResultCardStyle::Clay:
        return QStringLiteral("clay");
    }

    return QStringLiteral("kraft_paper");
}

// Parses result card style from settings string.
inline ResultCardStyle resultCardStyleFromString(QString value) {
    value = value.trimmed().toLower();
    if (value == QStringLiteral("glassmorphism")) {
        return ResultCardStyle::Glassmorphism;
    }
    if (value == QStringLiteral("terminal")) {
        return ResultCardStyle::Terminal;
    }
    if (value == QStringLiteral("clay")) {
        return ResultCardStyle::Clay;
    }
    return ResultCardStyle::KraftPaper;
}

// Persistent single query snapshot used by history and replay.
struct QueryHistoryRecord {
    QDateTime timestampUtc;
    QString statusCode;
    QString queryWord;
    QString headword;
    QString preview;
    QString phonetic;
};
