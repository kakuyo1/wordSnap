#pragma once

#include <QObject>
#include <QString>

#include <functional>

#include "app/AppTypes.h"

class QNetworkAccessManager;
class QJsonObject;

class AiAssistService final : public QObject {
    Q_OBJECT

public:
    struct Result {
        enum class Status {
            Success,
            Disabled,
            InvalidConfiguration,
            Timeout,
            NetworkError,
            HttpError,
            ParseError
        };

        Status status{Status::Disabled};
        AiAssistContent content;
        QString errorMessage;
        int httpStatusCode{0};
    };

    using ResultCallback = std::function<void(const Result&)>;

    explicit AiAssistService(QObject* parent = nullptr);

    QString applySettings(const AppSettings& settings);
    bool isAvailable() const;
    QString unavailableReason() const;

    void requestWordAssist(const QString& word, ResultCallback callback);

    static bool validateSettings(const AppSettings& settings, QString* errorMessage);
    static bool parseStructuredContent(const QByteArray& responseBody,
                                       AiAssistContent* content,
                                       QString* errorMessage);

private:
    Result makeUnavailableResult() const;
    static QString clampWords(const QString& text, int maxWords);
    static QString stripJsonFences(QString text);
    static bool parseContentObject(const QString& rawContent,
                                   QJsonObject* contentObject,
                                   QString* errorMessage);
    static QString readApiErrorMessage(const QJsonObject& root);

    QNetworkAccessManager* networkAccessManager_{nullptr};
    bool aiEnabledInSettings_{false};
    bool available_{false};
    QString unavailableReason_;
    QString apiKey_;
    QString endpointUrl_;
    QString model_;
    int timeoutMs_{kDefaultAiTimeoutMs};
};
