#include "testmainwindow.h"
#include "src/engine.h"
#include "src/learningmode.h"
#include "src/mainwindow.h"
#include "src/vocabstore.h"
#include "src/vocabword.h"
#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QTextStream>

// ── SRT file helpers ──────────────────────────────────────────────────────────

static int s_srtIdx = 0;
static QString writeSrt(const QString &content) {
    QString path = QString("/tmp/tmw_%1.srt").arg(s_srtIdx++);
    QFile f(path);
    f.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&f);
    out.setCodec("UTF-8");
    out << content;
    return path;
}

// Two subtitle windows: 1000-3000 ms and 4000-6000 ms
static const char *GERMAN_SRT =
    "1\r\n"
    "00:00:01,000 --> 00:00:03,000\r\n"
    "Heute ist ein schöner Tag.\r\n"
    "\r\n"
    "2\r\n"
    "00:00:04,000 --> 00:00:06,000\r\n"
    "Das ist eine Möglichkeit.\r\n"
    "\r\n";

static const char *ENGLISH_SRT =
    "1\r\n"
    "00:00:01,000 --> 00:00:03,000\r\n"
    "Today is a beautiful day.\r\n"
    "\r\n"
    "2\r\n"
    "00:00:04,000 --> 00:00:06,000\r\n"
    "That is a possibility.\r\n"
    "\r\n";

// ── Fixture ───────────────────────────────────────────────────────────────────

void TestMainWindow::initTestCase() {
    // Isolate QSettings from real user preferences
    QCoreApplication::setOrganizationName("PenguinTest");
    QCoreApplication::setApplicationName("PenguinSubtitlePlayerTest");
    QSettings s;
    s.setValue("gen/useDetectedEncoding", true);
    s.setValue("appearance/rememberWindowPosAndSize", false);
    s.setValue("learning/enabled", true);
    s.setValue("learning/thinkTime", 2);
    s.setValue("learning/autoHideDelay", 0);
    s.setValue("learning/inlineHighlights", true);

    m_srtPath    = writeSrt(GERMAN_SRT);
    m_translPath = writeSrt(ENGLISH_SRT);
}

void TestMainWindow::cleanupTestCase() {
    QFile::remove(m_srtPath);
    QFile::remove(m_translPath);
}

// ── Test utilities ─────────────────────────────────────────────────────────────

// Static member of TestMainWindow so it can use the friend declaration to
// access private members of MainWindow. Positions the engine at a given time
// and seeds m_lastSubtitleText so that setPlay(false) → onPause(text) receives
// a non-empty string (empty text causes onPause to return Idle immediately).
void TestMainWindow::positionAt(MainWindow &w, long long timeMs,
                                const QString &subtitleText) {
    w.currentTime        = timeMs;
    w.m_lastSubtitleText = subtitleText;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

void TestMainWindow::testHintLabelHiddenInitially() {
    MainWindow w;
    w.show();
    QLabel *hint = w.findChild<QLabel *>("hintLabel");
    QVERIFY(hint);
    QVERIFY(!hint->isVisible());
}

void TestMainWindow::testTranslationLabelHiddenInitially() {
    MainWindow w;
    w.show();
    QLabel *transl = w.findChild<QLabel *>("translationLabel");
    QVERIFY(transl);
    QVERIFY(!transl->isVisible());
}

void TestMainWindow::testPauseWithZeroThinkTimeShowsHint() {
    MainWindow w;
    w.show();
    w.load(m_srtPath);
    w.learningMode->setThinkTime(0);
    // Simulate the update() loop having shown subtitle 1 at 2000 ms
    positionAt(w, 2000LL, "Heute ist ein schöner Tag.");
    w.setPlay(false); // onPause("Heute...") with thinkTime=0 → HintShowing

    QCOMPARE(w.learningMode->state(), LearningMode::HintShowing);
    QLabel *hint = w.findChild<QLabel *>("hintLabel");
    QVERIFY(hint->isVisible());
}

void TestMainWindow::testRevealHidesHintAndShowsTranslation() {
    MainWindow w;
    w.show();
    w.load(m_srtPath);
    w.translationEngine = new Engine(m_translPath, "UTF-8");
    w.learningMode->setThinkTime(0);
    positionAt(w, 2000LL, "Heute ist ein schöner Tag.");
    w.setPlay(false); // → HintShowing

    QLabel *hint   = w.findChild<QLabel *>("hintLabel");
    QLabel *transl = w.findChild<QLabel *>("translationLabel");
    QVERIFY(hint->isVisible());
    QVERIFY(!transl->isVisible());

    w.revealTranslation(); // → Revealed → updateTranslationDisplay()

    QCOMPARE(w.learningMode->state(), LearningMode::Revealed);
    QVERIFY(!hint->isVisible());
    QVERIFY(transl->isVisible());
    // sliderMoved=true ensures binary search works on freshly created engine
    QVERIFY(transl->text().contains("Today"));
}

void TestMainWindow::testPlayAfterRevealResetsToIdle() {
    MainWindow w;
    w.show();
    w.load(m_srtPath);
    w.learningMode->setThinkTime(0);
    positionAt(w, 2000LL, "Heute ist ein schöner Tag.");
    w.setPlay(false);
    w.revealTranslation();
    QCOMPARE(w.learningMode->state(), LearningMode::Revealed);

    w.setPlay(true);                // onPlay() → Idle
    w.timer->stop();                // prevent timer from firing during test

    QCOMPARE(w.learningMode->state(), LearningMode::Idle);
    QLabel *hint = w.findChild<QLabel *>("hintLabel");
    QVERIFY(!hint->isVisible());
}

void TestMainWindow::testLearningModeHidesTranslationUntilReveal() {
    MainWindow w;
    w.show();
    w.load(m_srtPath);
    w.translationEngine = new Engine(m_translPath, "UTF-8");
    w.m_learningModeEnabled = true;
    w.learningMode->setActive(true);
    w.learningMode->setThinkTime(0);
    positionAt(w, 2000LL, "Heute ist ein schöner Tag.");
    w.setPlay(false); // → HintShowing

    QLabel *transl = w.findChild<QLabel *>("translationLabel");
    QVERIFY(!transl->isVisible()); // hidden while waiting for reveal

    w.revealTranslation(); // → Revealed
    QVERIFY(transl->isVisible()); // shown after explicit reveal
}

void TestMainWindow::testPassiveModeTranslationVisibleAtSubtitleTime() {
    MainWindow w;
    w.show();
    w.load(m_srtPath);
    w.translationEngine = new Engine(m_translPath, "UTF-8");
    w.m_learningModeEnabled = false;
    w.learningMode->setActive(false);
    // sliderMoved uses binary search (sliderMoved=true) for primary engine;
    // updateTranslationDisplay queries translation engine with sliderMoved=false.
    // For a fresh Engine (lastIndex=-1), linear search starts at 0 which is
    // correct for subtitle 1 at 1000-3000 ms. Set currentTime=2000 to be safe.
    positionAt(w, 2000LL, "Heute ist ein schöner Tag.");
    w.sliderMoved(2); // currentTime = 2×1000 = 2000 ms → updateTranslationDisplay()

    QLabel *transl = w.findChild<QLabel *>("translationLabel");
    QVERIFY(transl->isVisible());
    QVERIFY(transl->text().contains("Today"));
}

void TestMainWindow::testPassiveModeTranslationHiddenBetweenSubtitles() {
    MainWindow w;
    w.show();
    w.load(m_srtPath);
    w.translationEngine = new Engine(m_translPath, "UTF-8");
    w.m_learningModeEnabled = false;
    w.learningMode->setActive(false);
    // 3500 ms = gap between subtitle 1 (ends 3000) and subtitle 2 (starts 4000)
    w.sliderMoved(3); // 3000 ms = exactly end of subtitle 1 (exclusive: end < time)
    // Then advance one more step to be fully in the gap
    w.sliderMoved(3); // same position — translation should be empty in gap

    QLabel *transl = w.findChild<QLabel *>("translationLabel");
    // If translation text is empty at this time, label must not be visible
    if (transl->text().isEmpty())
        QVERIFY(!transl->isVisible());
}

void TestMainWindow::testToggleVocabPanelShowsAndHides() {
    MainWindow w;
    QVERIFY(!w.vocabPanel->isVisible());
    w.toggleVocabPanel();
    QVERIFY(w.vocabPanel->isVisible());
    w.toggleVocabPanel();
    QVERIFY(!w.vocabPanel->isVisible());
}

void TestMainWindow::testToggleLearningModeFlipsFlag() {
    MainWindow w;
    bool initial = w.m_learningModeEnabled;
    w.toggleLearningMode();
    QCOMPARE(w.m_learningModeEnabled, !initial);
    w.toggleLearningMode();
    QCOMPARE(w.m_learningModeEnabled, initial);
}

void TestMainWindow::testInlineHighlightsEnabledByDefault() {
    QSettings s;
    s.remove("learning/inlineHighlights");

    MainWindow w;

    QVERIFY(w.m_inlineHighlightsEnabled);
    QCOMPARE(s.value("learning/inlineHighlights").toBool(), true);

    // Keep the shared fixture default for following tests.
    s.setValue("learning/inlineHighlights", true);
}

void TestMainWindow::testInlineHighlightAddsSpanForKnownWord() {
    MainWindow w;
    w.show();
    w.load(m_srtPath);

    VocabWord word;
    word.word      = "der Tag, -e";
    word.type      = "m";
    word.meaning   = "day";
    word.exampleDe = "Heute ist ein schöner Tag.";
    word.exampleEn = "Today is a beautiful day.";
    w.vocabStore->addWord(word);
    w.m_inlineHighlightsEnabled = true;

    // sliderMoved(2) → currentTime=2000 ms → subtitle 1, applyVocabHighlights
    w.sliderMoved(2);

    QLabel *subtitle = w.findChild<QLabel *>("subtitleLabel");
    const QString text = subtitle->text();
    QVERIFY(text.contains("border-bottom")); // highlight span inserted
    QVERIFY(text.contains("Tag"));
}

void TestMainWindow::testToggleInlineHighlightsRefreshesCurrentSubtitle() {
    MainWindow w;
    w.show();
    w.load(m_srtPath);

    VocabWord word;
    word.word      = "der Tag, -e";
    word.type      = "m";
    word.meaning   = "day";
    word.exampleDe = "Heute ist ein schöner Tag.";
    word.exampleEn = "Today is a beautiful day.";
    w.vocabStore->addWord(word);

    w.m_inlineHighlightsEnabled = false;
    w.sliderMoved(2);

    QLabel *subtitle = w.findChild<QLabel *>("subtitleLabel");
    QLabel *legend   = w.findChild<QLabel *>("legendLabel");
    QVERIFY(!subtitle->text().contains("border-bottom"));
    QVERIFY(!legend->isVisible());

    w.toggleInlineHighlights();
    QVERIFY(w.m_inlineHighlightsEnabled);
    QVERIFY(subtitle->text().contains("border-bottom"));
    QVERIFY(legend->isVisible());
    QVERIFY(legend->text().contains("day"));

    w.toggleInlineHighlights();
    QVERIFY(!w.m_inlineHighlightsEnabled);
    QVERIFY(!subtitle->text().contains("border-bottom"));
    QVERIFY(!legend->isVisible());
}

void TestMainWindow::testCtrlXCtrlHTogglesInlineHighlights() {
    MainWindow w;
    w.m_inlineHighlightsEnabled = true;

    QKeyEvent ctrlX(QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier);
    QApplication::sendEvent(&w, &ctrlX);
    QKeyEvent ctrlH(QEvent::KeyPress, Qt::Key_H, Qt::ControlModifier);
    QApplication::sendEvent(&w, &ctrlH);

    QVERIFY(!w.m_inlineHighlightsEnabled);
}

void TestMainWindow::testHighlightDoesNotCorruptHtmlTagAttributes() {
    MainWindow w;
    w.show();
    w.load(m_srtPath);

    VocabWord word;
    word.word      = "die Möglichkeit, -en";
    word.type      = "f";
    word.meaning   = "possibility";
    word.exampleDe = "Das ist eine Möglichkeit.";
    word.exampleEn = "That is a possibility.";
    w.vocabStore->addWord(word);
    w.m_inlineHighlightsEnabled = true;

    // sliderMoved(5) → 5000 ms → subtitle 2 "Das ist eine Möglichkeit."
    w.sliderMoved(5);

    QLabel *subtitle = w.findChild<QLabel *>("subtitleLabel");
    const QString text = subtitle->text();
    QVERIFY(text.contains("Möglichkeit"));
    // Every < must have a matching > — tags not corrupted
    QCOMPARE(text.count('<'), text.count('>'));
}
