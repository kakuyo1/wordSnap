#include <QtTest/QtTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QStringList>
#include <QTimer>

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
    void requestWordAssistReturnsInvalidConfigurationWhenSettingsInvalid();
    void requestWordAssistReturnsDisabledWhenFeatureOff();
    void requestWordAssistReturnsTimeoutAndKeepsServiceAvailable();
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

void AiAssistServiceTest::requestWordAssistReturnsInvalidConfigurationWhenSettingsInvalid() {
    AiAssistService service;

    AppSettings settings;
    settings.aiAssistEnabled = true;
    settings.aiApiKey.clear();
    settings.aiBaseUrl = QStringLiteral("https://api.deepseek.com/v1/chat/completions");
    settings.aiModel = QStringLiteral("deepseek-chat");
    settings.aiTimeoutMs = kDefaultAiTimeoutMs;

    const QString warning = service.applySettings(settings);
    QCOMPARE(warning, QStringLiteral("AI API key is empty."));

    bool callbackCalled = false;
    AiAssistService::Result result;
    service.requestWordAssist(QStringLiteral("run"), [&](const AiAssistService::Result& callbackResult) {
        callbackCalled = true;
        result = callbackResult;
    });

    QVERIFY(callbackCalled);
    QCOMPARE(result.status, AiAssistService::Result::Status::InvalidConfiguration);
    QCOMPARE(result.errorMessage, QStringLiteral("AI API key is empty."));
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

void AiAssistServiceTest::requestWordAssistReturnsTimeoutAndKeepsServiceAvailable() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    QTcpSocket* hangingSocket = nullptr;
    connect(&server, &QTcpServer::newConnection, this, [&]() {
        hangingSocket = server.nextPendingConnection();
        QVERIFY(hangingSocket != nullptr);
        connect(hangingSocket, &QTcpSocket::readyRead, hangingSocket, [hangingSocket]() {
            hangingSocket->readAll();
        });
    });

    AiAssistService service;
    AppSettings settings;
    settings.aiAssistEnabled = true;
    settings.aiApiKey = QStringLiteral("test-key");
    settings.aiBaseUrl = QStringLiteral("http://127.0.0.1:%1/v1/chat/completions").arg(server.serverPort());
    settings.aiModel = QStringLiteral("deepseek-chat");
    settings.aiTimeoutMs = kMinAiTimeoutMs;

    const QString warning = service.applySettings(settings);
    QCOMPARE(warning, QString());
    QVERIFY(service.isAvailable());

    AiAssistService::Result result;
    bool callbackCalled = false;
    QEventLoop loop;
    QTimer guard;
    guard.setSingleShot(true);
    guard.start(5000);
    connect(&guard, &QTimer::timeout, &loop, &QEventLoop::quit);

    service.requestWordAssist(QStringLiteral("run"), [&](const AiAssistService::Result& callbackResult) {
        callbackCalled = true;
        result = callbackResult;
        loop.quit();
    });

    loop.exec();

    QVERIFY(callbackCalled);
    QCOMPARE(result.status, AiAssistService::Result::Status::Timeout);
    QCOMPARE(result.errorMessage, QStringLiteral("AI request timed out."));
    QVERIFY(service.isAvailable());

    if (hangingSocket != nullptr) {
        hangingSocket->close();
        hangingSocket->deleteLater();
    }
}

QTEST_MAIN(AiAssistServiceTest)

#include "AiAssistServiceTest.moc"
