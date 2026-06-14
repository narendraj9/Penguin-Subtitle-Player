#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "engine.h"
#include "learningmode.h"
#include "vocabpanel.h"
#include "vocabstore.h"
#include <QHash>
#include <QKeyEvent>
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSet>
#include <QSettings>
#include <QString>
#include <QSystemTrayIcon>
#include <QTimer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void load(QString path);
    void setPlay(bool play);

public slots:
    void update();
    void sliderMoved(int val);
    void togglePlay();
    void showToggleContextMenu(const QPoint &pos);
    void fastForward();
    void fastBackward();
    void next();
    void previous();
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void openSettingsWindow();
    void openFileDialog();
    void openSkipToTimeDialog();
    void activateNextClickCounts();

    // ARD-parity learning slots
    void revealTranslation();
    void toggleVocabPanel();
    void toggleLearningMode();
    void toggleInlineHighlights();
    void showHelp();
    void loadTranslationFile();

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragMoveEvent();
    void dropEvent(QDropEvent *e) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void loadPosAndSize();
    void loadPref();
    void setup();
    void enableControls();
    QString getSubtitle(bool sliderMoved);
    QString getEncoding(QString preset);
    QString promptForEncoding(QStringList codecNames, int recommendIndex);
    void adjustTime(long long interval);
    long long getAdjustInterval();

    // ARD-parity helpers
    void onLearningModeStateChanged(LearningMode::State state);
    void updateTranslationDisplay();
    void updateHighlights(const QString &subtitleHtml);
    void updateLegend(const QString &subtitleHtml);
    QString applyVocabHighlights(const QString &html);
    void adjustVocabOpacity(double delta);
    void loadVocabFileFromSettings();
    void updateLearningButtons();
    void requestLlmVocabulary(const QString &subtitleText);
    void onLlmVocabularyReply(QNetworkReply *reply);
    void refreshDisplayedSubtitle();

    Ui::MainWindow *ui;

    int m_nMouseClick_X_Coordinate = 0;
    int m_nMouseClick_Y_Coordinate = 0;
    long long int currentTime = 0LL;
    double speedFactor = 1.0;
    double intervalRemainder = 0;
    const long long int INTERVAL = 200LL;
    const long long int SLIDER_RATIO = 1000LL;
    Engine *engine = nullptr;
    Engine *translationEngine = nullptr;
    QTimer *timer = nullptr;
    bool isPlaying = false;
    QMenu *menu = nullptr;
    QSettings settings;
    bool skipped = false;

    // ARD-parity members
    LearningMode *learningMode = nullptr;
    VocabStore *vocabStore = nullptr;
    VocabPanel *vocabPanel = nullptr;

    bool m_learningModeEnabled = true;
    bool m_inlineHighlightsEnabled = true;

    bool m_ctrlXPending = false;
    QTimer *m_ctrlXTimer = nullptr;

    QString m_lastSubtitleText;

    QNetworkAccessManager *m_vocabNetwork = nullptr;
    QHash<QString, QVector<VocabWord>> m_llmVocabCache;
    QSet<QString> m_pendingVocabRequests;
    QVector<VocabWord> m_currentLlmWords;

    friend class TestMainWindow;
};

#endif // MAINWINDOW_H
