#include <QtTest/QtTest>

#include "app/AiAssistPolicy.h"

class AiAssistPolicyTest : public QObject {
    Q_OBJECT

private slots:
    void decideReturnsSkipWhenLookupNotFound();
    void decideReturnsSkipWhenLookupNotFoundEvenIfServiceUnavailable();
    void decideReturnsSkipWhenFeatureDisabled();
    void decideReturnsShowErrorWhenServiceUnavailable();
    void decideReturnsShowErrorWhenRequestWordEmpty();
    void decideUsesTrimmedCardTitleFirst();
    void decideFallsBackToTrimmedQueryWord();
};

void AiAssistPolicyTest::decideReturnsSkipWhenLookupNotFound() {
    const AiAssistPolicy::Decision decision = AiAssistPolicy::decide(AiAssistPolicy::Context{
        LookupCoordinator::Status::Unknown,
        true,
        true,
        QStringLiteral("run"),
        QStringLiteral("run"),
    });

    QCOMPARE(decision.action, AiAssistPolicy::Action::Skip);
    QVERIFY(decision.requestWord.isEmpty());
}

void AiAssistPolicyTest::decideReturnsSkipWhenLookupNotFoundEvenIfServiceUnavailable() {
    const AiAssistPolicy::Decision decision = AiAssistPolicy::decide(AiAssistPolicy::Context{
        LookupCoordinator::Status::DictUnavailable,
        true,
        false,
        QStringLiteral("run"),
        QStringLiteral("run"),
    });

    QCOMPARE(decision.action, AiAssistPolicy::Action::Skip);
    QVERIFY(decision.requestWord.isEmpty());
}

void AiAssistPolicyTest::decideReturnsSkipWhenFeatureDisabled() {
    const AiAssistPolicy::Decision decision = AiAssistPolicy::decide(AiAssistPolicy::Context{
        LookupCoordinator::Status::Found,
        false,
        true,
        QStringLiteral("run"),
        QStringLiteral("run"),
    });

    QCOMPARE(decision.action, AiAssistPolicy::Action::Skip);
    QVERIFY(decision.requestWord.isEmpty());
}

void AiAssistPolicyTest::decideReturnsShowErrorWhenServiceUnavailable() {
    const AiAssistPolicy::Decision decision = AiAssistPolicy::decide(AiAssistPolicy::Context{
        LookupCoordinator::Status::Found,
        true,
        false,
        QStringLiteral("run"),
        QStringLiteral("run"),
    });

    QCOMPARE(decision.action, AiAssistPolicy::Action::ShowError);
    QVERIFY(decision.requestWord.isEmpty());
}

void AiAssistPolicyTest::decideReturnsShowErrorWhenRequestWordEmpty() {
    const AiAssistPolicy::Decision decision = AiAssistPolicy::decide(AiAssistPolicy::Context{
        LookupCoordinator::Status::Found,
        true,
        true,
        QStringLiteral("   "),
        QStringLiteral("\n\t"),
    });

    QCOMPARE(decision.action, AiAssistPolicy::Action::ShowError);
    QVERIFY(decision.requestWord.isEmpty());
}

void AiAssistPolicyTest::decideUsesTrimmedCardTitleFirst() {
    const AiAssistPolicy::Decision decision = AiAssistPolicy::decide(AiAssistPolicy::Context{
        LookupCoordinator::Status::Found,
        true,
        true,
        QStringLiteral("  running  "),
        QStringLiteral("run"),
    });

    QCOMPARE(decision.action, AiAssistPolicy::Action::Request);
    QCOMPARE(decision.requestWord, QStringLiteral("running"));
}

void AiAssistPolicyTest::decideFallsBackToTrimmedQueryWord() {
    const AiAssistPolicy::Decision decision = AiAssistPolicy::decide(AiAssistPolicy::Context{
        LookupCoordinator::Status::Found,
        true,
        true,
        QString(),
        QStringLiteral("  run  "),
    });

    QCOMPARE(decision.action, AiAssistPolicy::Action::Request);
    QCOMPARE(decision.requestWord, QStringLiteral("run"));
}

QTEST_APPLESS_MAIN(AiAssistPolicyTest)

#include "AiAssistPolicyTest.moc"
