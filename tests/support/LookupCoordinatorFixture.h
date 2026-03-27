#pragma once

#include <QImage>
#include <QString>

#include "app/LookupCoordinator.h"

struct LookupCoordinatorFixture {
    QImage capturedImage{4, 4, QImage::Format_ARGB32};
    QImage preprocessedImage{4, 4, QImage::Format_ARGB32};
    OcrWordResult ocrResult{QStringLiteral("run"), QString(), 0.0F, true};
    QString normalizedCandidate{QStringLiteral("run")};
    QString ocrError;
    bool dictionaryReady{true};
    DictionaryEntry dictionaryEntry{};

    bool preprocessCalled{false};
    bool recognizeCalled{false};
    bool normalizeCalled{false};
    bool dictionaryReadyCalled{false};
    bool lookupCalled{false};
    QString lookupWord;

    LookupCoordinatorFixture() {
        dictionaryEntry = makeFoundEntry(QStringLiteral("run"), QStringLiteral("to move fast"));
    }

    static DictionaryEntry makeFoundEntry(const QString& headword, const QString& definition) {
        DictionaryEntry entry;
        entry.found = true;
        entry.headword = headword;
        if (!definition.isEmpty()) {
            entry.definitionsEn.push_back(definition);
        }
        return entry;
    }

    LookupCoordinator createCoordinator() {
        return LookupCoordinator(LookupCoordinator::Dependencies{
            [this](const QRect&) {
                return capturedImage;
            },
            [this](const QImage&) {
                preprocessCalled = true;
                return preprocessedImage;
            },
            [this](const QImage&, const QString&, QString* errorMessage) {
                recognizeCalled = true;
                if (errorMessage != nullptr) {
                    *errorMessage = ocrError;
                }
                return ocrResult;
            },
            [this](const QString&) {
                normalizeCalled = true;
                return normalizedCandidate;
            },
            [this]() {
                dictionaryReadyCalled = true;
                return dictionaryReady;
            },
            [this](const QString& word) {
                lookupCalled = true;
                lookupWord = word;
                return dictionaryEntry;
            },
        });
    }
};
