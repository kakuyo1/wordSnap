#pragma once

#include <QString>

class AiAssistPolicy final {
public:
    enum class Action {
        Skip,
        ShowError,
        Request
    };

    struct Context {
        bool lookupFound{false};
        bool aiAssistEnabled{false};
        bool aiServiceAvailable{false};
        QString cardTitle;
        QString queryWord;
    };

    struct Decision {
        Action action{Action::Skip};
        QString requestWord;
    };

    static Decision decide(const Context& context);
};
