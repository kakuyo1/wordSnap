#pragma once

#include <QString>

#include "app/LookupCoordinator.h"

class AiAssistPolicy final {
public:
    enum class Action {
        Skip,
        ShowError,
        Request
    };

    struct Context {
        LookupCoordinator::Status lookupStatus{LookupCoordinator::Status::OcrFailed};
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
