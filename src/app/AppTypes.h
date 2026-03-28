#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>

// Result card visual style.
enum class ResultCardStyle {
    KraftPaper,
    WhitePaper,
    Glassmorphism,
    Terminal
};

constexpr int kMinResultCardOpacityPercent = 35;
constexpr int kMaxResultCardOpacityPercent = 100;
constexpr int kDefaultResultCardDurationMs = 6500;
constexpr int kMinResultCardDurationMs = 1000;
constexpr int kMaxResultCardDurationMs = 30000;
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

inline int clampResultCardDurationMs(const int value) {
    if (value < kMinResultCardDurationMs) {
        return kMinResultCardDurationMs;
    }
    if (value > kMaxResultCardDurationMs) {
        return kMaxResultCardDurationMs;
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
    bool launchOnStartup{false};
    QString starDictDir;
    QString tessdataDir;
    int resultCardOpacityPercent{92};
    int resultCardDurationMs{kDefaultResultCardDurationMs};
    ResultCardStyle resultCardStyle{ResultCardStyle::KraftPaper};
    int queryHistoryLimit{kDefaultQueryHistoryLimit};
    bool aiAssistEnabled{false};
    QString aiApiKey;
    QString aiBaseUrl{QStringLiteral("https://api.deepseek.com/v1/chat/completions")};
    QString aiModel{QStringLiteral("deepseek-chat")};
    int aiTimeoutMs{kDefaultAiTimeoutMs};
};

// Converts result card style enum to stable settings string.
inline QString resultCardStyleToString(const ResultCardStyle style) {
    switch (style) {
    case ResultCardStyle::KraftPaper:
        return QStringLiteral("kraft_paper");
    case ResultCardStyle::WhitePaper:
        return QStringLiteral("white_paper");
    case ResultCardStyle::Glassmorphism:
        return QStringLiteral("glassmorphism");
    case ResultCardStyle::Terminal:
        return QStringLiteral("terminal");
    }

    return QStringLiteral("kraft_paper");
}

// Parses result card style from settings string.
inline ResultCardStyle resultCardStyleFromString(QString value) {
    value = value.trimmed().toLower();
    if (value == QStringLiteral("white_paper")) {
        return ResultCardStyle::WhitePaper;
    }
    if (value == QStringLiteral("glassmorphism")) {
        return ResultCardStyle::Glassmorphism;
    }
    if (value == QStringLiteral("terminal")) {
        return ResultCardStyle::Terminal;
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
