#pragma once

#include <QString>
#include <QStringList>

// Definition display mode for dictionary entries.
enum class DisplayMode {
    Zh,
    En,
    Bilingual
};

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

// Application-level persistent settings.
struct AppSettings {
    QString hotkey{QStringLiteral("Shift+Alt+S")};
    DisplayMode displayMode{DisplayMode::Bilingual};
    QString starDictDir;
    QString tessdataDir;
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
