#include "testlearningmode.h"
#include "src/learningmode.h"
#include <QCoreApplication>
#include <QSignalSpy>

void TestLearningMode::testInitialState() {
    LearningMode lm;
    QCOMPARE(lm.state(), LearningMode::Idle);
    QVERIFY(lm.isActive());
}

void TestLearningMode::testPauseStartsHidden() {
    LearningMode lm;
    lm.setThinkTime(10); // long enough not to fire during test
    lm.onPause("Heute ist ein schöner Tag.");
    QCOMPARE(lm.state(), LearningMode::Hidden);
}

void TestLearningMode::testThinkTimerTriggersHint() {
    LearningMode lm;
    lm.setThinkTime(0); // fires immediately (0 = skip to HintShowing)
    // With thinkTime=0 onPause goes straight to HintShowing
    lm.onPause("Heute ist ein schöner Tag.");
    QCOMPARE(lm.state(), LearningMode::HintShowing);
}

void TestLearningMode::testRevealEmitsSignal() {
    LearningMode lm;
    lm.setThinkTime(0);
    lm.onPause("Das ist eine Möglichkeit.");

    QSignalSpy spy(&lm, &LearningMode::recallNeeded);
    lm.onReveal();
    QCOMPARE(lm.state(), LearningMode::Revealed);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toString(), "Das ist eine Möglichkeit.");
}

void TestLearningMode::testPlayResetsState() {
    LearningMode lm;
    lm.setThinkTime(0);
    lm.onPause("Hallo.");
    QCOMPARE(lm.state(), LearningMode::HintShowing);
    lm.onPlay();
    QCOMPARE(lm.state(), LearningMode::Idle);
}

void TestLearningMode::testInactiveNoStateChange() {
    LearningMode lm;
    lm.setActive(false);
    lm.onPause("Hallo.");
    QCOMPARE(lm.state(), LearningMode::Idle);
    lm.onReveal();
    QCOMPARE(lm.state(), LearningMode::Idle);
}

void TestLearningMode::testSubtitleChangedWhilePaused() {
    LearningMode lm;
    lm.setThinkTime(10);
    lm.onPause("Erster Satz.");
    QCOMPARE(lm.state(), LearningMode::Hidden);

    // Subtitle changes (e.g. user pressed next while paused in ARD)
    lm.onSubtitleChanged("Zweiter Satz.");
    // Should restart the think timer (still Hidden, new subtitle)
    QCOMPARE(lm.state(), LearningMode::Hidden);
}

void TestLearningMode::testAutoHideDelay() {
    LearningMode lm;
    lm.setThinkTime(0);
    lm.setAutoHideDelay(0); // disabled
    lm.onPause("Test.");
    lm.onReveal();
    QCOMPARE(lm.state(), LearningMode::Revealed);
    // Auto-hide is disabled, so state stays Revealed
    QTest::qWait(100);
    QCOMPARE(lm.state(), LearningMode::Revealed);
}

void TestLearningMode::testZeroThinkTimeSkipsHidden() {
    LearningMode lm;
    lm.setThinkTime(0);
    lm.onPause("Test.");
    // With thinkTime==0, we should go directly to HintShowing
    QCOMPARE(lm.state(), LearningMode::HintShowing);
}
