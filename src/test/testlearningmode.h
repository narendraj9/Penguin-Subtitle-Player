#ifndef TESTLEARNINGMODE_H
#define TESTLEARNINGMODE_H

#include <QtTest>

class TestLearningMode : public QObject {
    Q_OBJECT
private slots:
    void testInitialState();
    void testPauseStartsHidden();
    void testThinkTimerTriggersHint();
    void testRevealEmitsSignal();
    void testPlayResetsState();
    void testInactiveNoStateChange();
    void testSubtitleChangedWhilePaused();
    void testAutoHideDelay();
    void testZeroThinkTimeSkipsHidden();
};

#endif // TESTLEARNINGMODE_H
