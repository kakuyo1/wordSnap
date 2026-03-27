#include "app/AiAssistPolicy.h"

AiAssistPolicy::Decision AiAssistPolicy::decide(const Context& context) {
    if (!context.lookupFound || !context.aiAssistEnabled) {
        return Decision{};
    }

    if (!context.aiServiceAvailable) {
        Decision decision;
        decision.action = Action::ShowError;
        return decision;
    }

    QString requestWord = context.cardTitle.trimmed();
    if (requestWord.isEmpty()) {
        requestWord = context.queryWord.trimmed();
    }

    if (requestWord.isEmpty()) {
        Decision decision;
        decision.action = Action::ShowError;
        return decision;
    }

    Decision decision;
    decision.action = Action::Request;
    decision.requestWord = requestWord;
    return decision;
}
