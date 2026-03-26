#include <QtTest/QtTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#include "services/AiAssistService.h"

namespace {
int wordCount(const QString& text) {
    return text.simplified().split(' ', Qt::SkipEmptyParts).size();
}
} // namespace

class AiAssistServiceTest : public QObject {
    Q_OBJECT

private slots:
    void validateSettingsRejectsMissingApiKey();
    void parseStructuredContentExtractsJsonPayload();
    void parseStructuredContentTrimsLongFields();
    void requestWordAssistReturnsDisabledWhenFeatureOff();
};

void AiAssistServiceTest::validateSettingsRejectsMissingApiKey() {
    AppSettings settings;
    settings.aiAssistEnabled = true;
    settings.aiApiKey.clear();
    settings.aiBaseUrl = QStringLiteral("https://api.deepseek.com/v1/chat/completions");
    settings.aiModel = QStringLiteral("deepseek-chat");
    settings.aiTimeoutMs = kDefaultAiTimeoutMs;

    QString errorMessage;
    QVERIFY(!AiAssistService::validateSettings(settings, &errorMessage));
    QCOMPARE(errorMessage, QStringLiteral("AI API key is empty."));
}

void AiAssistServiceTest::parseStructuredContentExtractsJsonPayload() {
    const QJsonObject innerObject{{QStringLiteral("definition_en"), QStringLiteral("to run fast")},
                                  {QStringLiteral("roots"), QStringLiteral("Old English and Proto-Germanic")},
                                  {QStringLiteral("etymology"), QStringLiteral("from an early motion verb")}};
    const QString wrapped = QStringLiteral("```json\n%1\n```")
                                .arg(QString::fromUtf8(QJsonDocument(innerObject).toJson(QJsonDocument::Compact)));

    QJsonObject messageObject;
    messageObject.insert(QStringLiteral("content"), wrapped);

    QJsonObject choiceObject;
    choiceObject.insert(QStringLiteral("message"), messageObject);

    QJsonArray choices;
    choices.append(choiceObject);

    QJsonObject outerObject;
    outerObject.insert(QStringLiteral("choices"), choices);

    AiAssistContent content;
    QString errorMessage;
    QVERIFY(AiAssistService::parseStructuredContent(
        QJsonDocument(outerObject).toJson(QJsonDocument::Compact),
        &content,
        &errorMessage));
    QCOMPARE(errorMessage, QString());
    QCOMPARE(content.definitionEn, QStringLiteral("to run fast"));
    QCOMPARE(content.roots, QStringLiteral("Old English and Proto-Germanic"));
    QCOMPARE(content.etymology, QStringLiteral("from an early motion verb"));
}

void AiAssistServiceTest::parseStructuredContentTrimsLongFields() {
    QStringList words;
    for (int i = 0; i < 35; ++i) {
        words.push_back(QStringLiteral("word%1").arg(i + 1));
    }

    const QString longText = words.join(' ');
    const QJsonObject innerObject{{QStringLiteral("definition_en"), longText},
                                  {QStringLiteral("roots"), longText},
                                  {QStringLiteral("etymology"), longText}};

    QJsonObject messageObject;
    messageObject.insert(
        QStringLiteral("content"),
        QString::fromUtf8(QJsonDocument(innerObject).toJson(QJsonDocument::Compact)));

    QJsonObject choiceObject;
    choiceObject.insert(QStringLiteral("message"), messageObject);

    QJsonArray choices;
    choices.append(choiceObject);

    QJsonObject outerObject;
    outerObject.insert(QStringLiteral("choices"), choices);

    AiAssistContent content;
    QString errorMessage;
    QVERIFY(AiAssistService::parseStructuredContent(
        QJsonDocument(outerObject).toJson(QJsonDocument::Compact),
        &content,
        &errorMessage));

    QCOMPARE(wordCount(content.definitionEn), 30);
    QCOMPARE(wordCount(content.roots), 30);
    QCOMPARE(wordCount(content.etymology), 30);
}

void AiAssistServiceTest::requestWordAssistReturnsDisabledWhenFeatureOff() {
    AiAssistService service;

    AppSettings settings;
    settings.aiAssistEnabled = false;
    QVERIFY(service.applySettings(settings).isEmpty());

    bool callbackCalled = false;
    AiAssistService::Result result;
    service.requestWordAssist(QStringLiteral("run"), [&](const AiAssistService::Result& callbackResult) {
        callbackCalled = true;
        result = callbackResult;
    });

    QVERIFY(callbackCalled);
    QCOMPARE(result.status, AiAssistService::Result::Status::Disabled);
}

QTEST_MAIN(AiAssistServiceTest)

#include "AiAssistServiceTest.moc"
