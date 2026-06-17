#ifndef TESTMAINWINDOW_H
#define TESTMAINWINDOW_H

#include <QObject>
#include <QtTest>

class MainWindow;

class TestMainWindow : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    // Initial state
    void testHintLabelHiddenInitially();
    void testTranslationLabelHiddenInitially();

    // Learning-mode pause/reveal flow
    void testPauseWithZeroThinkTimeShowsHint();
    void testRevealHidesHintAndShowsTranslation();
    void testPlayAfterRevealResetsToIdle();
    void testLearningModeHidesTranslationUntilReveal();

    // Passive mode
    void testPassiveModeTranslationVisibleAtSubtitleTime();
    void testPassiveModeTranslationHiddenBetweenSubtitles();

    // Vocab panel
    void testToggleVocabPanelShowsAndHides();

    // Mode toggle
    void testToggleLearningModeFlipsFlag();

    // Inline highlights
    void testInlineHighlightsEnabledByDefault();
    void testInlineHighlightAddsSpanForKnownWord();
    void testToggleInlineHighlightsRefreshesCurrentSubtitle();
    void testCtrlXCtrlHTogglesInlineHighlights();
    void testHighlightDoesNotCorruptHtmlTagAttributes();

private:
    QString m_srtPath;
    QString m_translPath;

    // Set currentTime + m_lastSubtitleText directly (friend access) so that
    // setPlay(false) → onPause(subtitleText) receives a non-empty string.
    static void positionAt(MainWindow &w, long long timeMs,
                           const QString &subtitleText);
};

#endif // TESTMAINWINDOW_H
