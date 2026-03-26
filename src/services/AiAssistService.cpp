#include "services/AiAssistService.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QTimer>
#include <QUrl>

namespace {
constexpr int kMaxFieldWords = 30;

QString safeApiMessage(const QString& message) {
    const QString simplified = message.simplified();
    if (simplified.isEmpty()) {
        return QStringLiteral("AI request failed.");
    }
    return simplified;
}
} // namespace

AiAssistService::AiAssistService(QObject* parent)
    : QObject(parent),
      networkAccessManager_(new QNetworkAccessManager(this)) {
}

QString AiAssistService::applySettings(const AppSettings& settings) {
    aiEnabledInSettings_ = settings.aiAssistEnabled;
    timeoutMs_ = clampAiTimeoutMs(settings.aiTimeoutMs);
    apiKey_ = settings.aiApiKey.trimmed();
    endpointUrl_ = settings.aiBaseUrl.trimmed();
    model_ = settings.aiModel.trimmed();

    if (endpointUrl_.isEmpty()) {
        endpointUrl_ = AppSettings{}.aiBaseUrl;
    }
    if (model_.isEmpty()) {
        model_ = AppSettings{}.aiModel;
    }

    if (!aiEnabledInSettings_) {
        available_ = false;
        unavailableReason_ = QStringLiteral("AI assist is disabled in settings.");
        return {};
    }

    AppSettings effective = settings;
    effective.aiApiKey = apiKey_;
    effective.aiBaseUrl = endpointUrl_;
    effective.aiModel = model_;
    effective.aiTimeoutMs = timeoutMs_;

    QString validationError;
    if (!validateSettings(effective, &validationError)) {
        available_ = false;
        unavailableReason_ = validationError;
        return validationError;
    }

    available_ = true;
    unavailableReason_.clear();
    return {};
}

bool AiAssistService::isAvailable() const {
    return available_;
}

QString AiAssistService::unavailableReason() const {
    return unavailableReason_;
}

void AiAssistService::requestWordAssist(const QString& word, ResultCallback callback) {
    if (!callback) {
        return;
    }

    if (!available_) {
        callback(makeUnavailableResult());
        return;
    }

    const QString trimmedWord = word.trimmed();
    if (trimmedWord.isEmpty()) {
        Result result;
        result.status = Result::Status::ParseError;
        result.errorMessage = QStringLiteral("Word is empty.");
        callback(result);
        return;
    }

    QNetworkRequest request{QUrl(endpointUrl_)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(apiKey_).toUtf8());

    QJsonObject payload;
    payload.insert(QStringLiteral("model"), model_);
    payload.insert(QStringLiteral("temperature"), 0.2);
    payload.insert(QStringLiteral("max_tokens"), 260);
    payload.insert(
        QStringLiteral("response_format"),
        QJsonObject{{QStringLiteral("type"), QStringLiteral("json_object")}});

    const QString systemPrompt = QStringLiteral(
        "Return strict JSON only with keys definition_en, roots, etymology. "
        "Each field must be concise English with at most 30 words.");
    const QString userPrompt = QStringLiteral("word: %1").arg(trimmedWord);

    QJsonArray messages;
    messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("system")},
                                {QStringLiteral("content"), systemPrompt}});
    messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                {QStringLiteral("content"), userPrompt}});
    payload.insert(QStringLiteral("messages"), messages);

    QNetworkReply* reply = networkAccessManager_->post(
        request,
        QJsonDocument(payload).toJson(QJsonDocument::Compact));

    auto* timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, reply, [reply]() {
        reply->setProperty("ai_timeout", true);
        reply->abort();
    });
    timeoutTimer->start(timeoutMs_);

    connect(reply, &QNetworkReply::finished, this, [this, reply, timeoutTimer, callback = std::move(callback)]() {
        timeoutTimer->stop();

        Result result;
        result.httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray responseBody = reply->readAll();
        const bool timedOut = reply->property("ai_timeout").toBool();

        if (timedOut) {
            result.status = Result::Status::Timeout;
            result.errorMessage = QStringLiteral("AI request timed out.");
            reply->deleteLater();
            callback(result);
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            if (result.httpStatusCode >= 400) {
                result.status = Result::Status::HttpError;

                QJsonParseError parseError;
                const QJsonDocument bodyDocument = QJsonDocument::fromJson(responseBody, &parseError);
                if (parseError.error == QJsonParseError::NoError && bodyDocument.isObject()) {
                    const QString apiErrorMessage = readApiErrorMessage(bodyDocument.object());
                    result.errorMessage = safeApiMessage(apiErrorMessage);
                }

                if (result.errorMessage.isEmpty()) {
                    result.errorMessage = safeApiMessage(reply->errorString());
                }

                if (result.httpStatusCode == 401 || result.httpStatusCode == 403) {
                    available_ = false;
                    unavailableReason_ = QStringLiteral("AI API authentication failed.");
                }
            } else {
                result.status = Result::Status::NetworkError;
                result.errorMessage = safeApiMessage(reply->errorString());
            }

            reply->deleteLater();
            callback(result);
            return;
        }

        QString parseError;
        if (!parseStructuredContent(responseBody, &result.content, &parseError)) {
            result.status = Result::Status::ParseError;
            result.errorMessage = safeApiMessage(parseError);
            reply->deleteLater();
            callback(result);
            return;
        }

        result.status = Result::Status::Success;
        reply->deleteLater();
        callback(result);
    });
}

bool AiAssistService::validateSettings(const AppSettings& settings, QString* errorMessage) {
    if (!settings.aiAssistEnabled) {
        if (errorMessage != nullptr) {
            errorMessage->clear();
        }
        return true;
    }

    if (settings.aiApiKey.trimmed().isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("AI API key is empty.");
        }
        return false;
    }

    const QString endpoint = settings.aiBaseUrl.trimmed();
    const QUrl endpointUrl(endpoint);
    const QString scheme = endpointUrl.scheme().toLower();
    if (endpoint.isEmpty() || !endpointUrl.isValid()
        || (scheme != QStringLiteral("https") && scheme != QStringLiteral("http"))) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("AI endpoint URL is invalid.");
        }
        return false;
    }

    if (settings.aiModel.trimmed().isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("AI model is empty.");
        }
        return false;
    }

    if (settings.aiTimeoutMs < kMinAiTimeoutMs || settings.aiTimeoutMs > kMaxAiTimeoutMs) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("AI timeout is out of range.");
        }
        return false;
    }

    if (errorMessage != nullptr) {
        errorMessage->clear();
    }
    return true;
}

bool AiAssistService::parseStructuredContent(const QByteArray& responseBody,
                                             AiAssistContent* content,
                                             QString* errorMessage) {
    if (content == nullptr) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Output content buffer is null.");
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(responseBody, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("AI response is not valid JSON.");
        }
        return false;
    }

    const QJsonObject root = document.object();
    const QString apiError = readApiErrorMessage(root);
    if (!apiError.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = apiError;
        }
        return false;
    }

    const QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("AI response missing choices.");
        }
        return false;
    }

    const QString rawContent = choices.first().toObject()
                                   .value(QStringLiteral("message"))
                                   .toObject()
                                   .value(QStringLiteral("content"))
                                   .toString()
                                   .trimmed();
    if (rawContent.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("AI response content is empty.");
        }
        return false;
    }

    QJsonObject contentObject;
    if (!parseContentObject(rawContent, &contentObject, errorMessage)) {
        return false;
    }

    AiAssistContent parsed;
    parsed.definitionEn = clampWords(contentObject.value(QStringLiteral("definition_en")).toString(), kMaxFieldWords);
    parsed.roots = clampWords(contentObject.value(QStringLiteral("roots")).toString(), kMaxFieldWords);
    parsed.etymology = clampWords(contentObject.value(QStringLiteral("etymology")).toString(), kMaxFieldWords);

    if (parsed.definitionEn.isEmpty() && parsed.roots.isEmpty() && parsed.etymology.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("AI response contains no usable fields.");
        }
        return false;
    }

    *content = parsed;
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }
    return true;
}

AiAssistService::Result AiAssistService::makeUnavailableResult() const {
    Result result;
    result.status = aiEnabledInSettings_ ? Result::Status::InvalidConfiguration : Result::Status::Disabled;
    result.errorMessage = unavailableReason_;
    return result;
}

QString AiAssistService::clampWords(const QString& text, const int maxWords) {
    const QString simplified = text.simplified();
    if (simplified.isEmpty()) {
        return {};
    }

    const QStringList words = simplified.split(' ', Qt::SkipEmptyParts);
    if (words.size() <= maxWords) {
        return simplified;
    }

    return words.mid(0, maxWords).join(' ');
}

QString AiAssistService::stripJsonFences(QString text) {
    text = text.trimmed();
    if (!text.startsWith(QStringLiteral("```")) || !text.endsWith(QStringLiteral("```"))) {
        return text;
    }

    const int firstLineBreak = text.indexOf('\n');
    const int lastFence = text.lastIndexOf(QStringLiteral("```"));
    if (firstLineBreak < 0 || lastFence <= firstLineBreak) {
        return text;
    }

    return text.mid(firstLineBreak + 1, lastFence - firstLineBreak - 1).trimmed();
}

bool AiAssistService::parseContentObject(const QString& rawContent,
                                         QJsonObject* contentObject,
                                         QString* errorMessage) {
    if (contentObject == nullptr) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Content object buffer is null.");
        }
        return false;
    }

    QString cleanedContent = stripJsonFences(rawContent);
    QJsonParseError parseError;
    QJsonDocument contentDocument = QJsonDocument::fromJson(cleanedContent.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !contentDocument.isObject()) {
        const int braceStart = cleanedContent.indexOf('{');
        const int braceEnd = cleanedContent.lastIndexOf('}');
        if (braceStart >= 0 && braceEnd > braceStart) {
            const QString candidate = cleanedContent.mid(braceStart, braceEnd - braceStart + 1);
            contentDocument = QJsonDocument::fromJson(candidate.toUtf8(), &parseError);
        }
    }

    if (parseError.error != QJsonParseError::NoError || !contentDocument.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("AI response content is not valid JSON object.");
        }
        return false;
    }

    *contentObject = contentDocument.object();
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }
    return true;
}

QString AiAssistService::readApiErrorMessage(const QJsonObject& root) {
    const QJsonValue errorValue = root.value(QStringLiteral("error"));
    if (!errorValue.isObject()) {
        return {};
    }

    const QString message = errorValue.toObject().value(QStringLiteral("message")).toString().trimmed();
    return safeApiMessage(message);
}
