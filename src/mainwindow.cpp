#include "mainwindow.h"
#include "QAction"
#include "QByteArray"
#include "QDebug"
#include "QDesktopWidget"
#include "QDir"
#include "QDragEnterEvent"
#include "QDropEvent"
#include "QFileDialog"
#include "QGraphicsDropShadowEffect"
#include "QIcon"
#include "QInputDialog"
#include "QLayout"
#include "QLineEdit"
#include "QList"
#include "QMenu"
#include "QMessageBox"
#include "QMimeData"
#include "QMouseEvent"
#include "QObject"
#include "QPainter"
#include "QSizeGrip"
#include "QString"
#include "QStyle"
#include "QTextCodec"
#include "QTimer"
#include "chardet.h"
#include "cmath"
#include "configdialog.h"
#include "engine.h"
#include "helpdialog.h"
#include "nccdialog.h"
#include "parser.h"
#include "prefconstants.h"
#include "string"
#include "ui_mainwindow.h"
#include <QKeyEvent>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), timer(new QTimer(this)) {
    ui->setupUi(this);

    this->setWindowIcon(QIcon(":/icon.png"));

    Qt::WindowFlags flags = this->windowFlags();
    this->setWindowFlags(flags | Qt::X11BypassWindowManagerHint |
                         Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground, true);

    timer->setTimerType(Qt::PreciseTimer);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));

    connect(ui->backwardButton, SIGNAL(clicked()), this, SLOT(fastBackward()));
    connect(ui->forwardButton, SIGNAL(clicked()), this, SLOT(fastForward()));
    connect(ui->prevButton, SIGNAL(clicked()), this, SLOT(previous()));
    connect(ui->nextButton, SIGNAL(clicked()), this, SLOT(next()));
    connect(ui->toggleButton, SIGNAL(clicked()), this, SLOT(togglePlay()));
    ui->toggleButton->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->toggleButton,
            SIGNAL(customContextMenuRequested(const QPoint &)), this,
            SLOT(showToggleContextMenu(const QPoint &)));
    connect(ui->loadButton, SIGNAL(clicked()), this, SLOT(openFileDialog()));
    connect(ui->prefButton, SIGNAL(clicked()), this,
            SLOT(openSettingsWindow()));
    connect(ui->quitButton, SIGNAL(clicked()), qApp, SLOT(quit()));
    connect(ui->horizontalSlider, SIGNAL(sliderMoved(int)), this,
            SLOT(sliderMoved(int)));
    connect(ui->timeLabel, SIGNAL(clicked()), this,
            SLOT(openSkipToTimeDialog()));

    // Right-click on load button → load translation
    ui->loadButton->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->loadButton,
            &QPushButton::customContextMenuRequested,
            [this](const QPoint &pos) {
                QMenu m;
                m.addAction(tr("Load Primary Subtitle"), this,
                            SLOT(openFileDialog()));
                m.addAction(tr("Load Translation Subtitle"), this,
                            SLOT(loadTranslationFile()));
                m.exec(ui->loadButton->mapToGlobal(pos));
            });

    // Click on hint label = reveal
    ui->hintLabel->setCursor(Qt::PointingHandCursor);
    connect(ui->hintLabel, &QLabel::linkActivated, this,
            &MainWindow::revealTranslation);
    // Make hintLabel clickable
    ui->hintLabel->installEventFilter(this);

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        QSystemTrayIcon *trayIcon = new QSystemTrayIcon(this);
        trayIcon->setIcon(QIcon(":/icon.png"));
        connect(trayIcon,
                SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this,
                SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
        menu = new QMenu();
        QAction *settingsAct = new QAction("Preferences", menu);
        connect(settingsAct, SIGNAL(triggered()), this,
                SLOT(openSettingsWindow()));
        menu->addAction(settingsAct);
        QAction *quit = new QAction("Quit", menu);
        connect(quit, SIGNAL(triggered()), qApp, SLOT(quit()));
        menu->addAction(quit);
        trayIcon->setContextMenu(menu);
        trayIcon->show();
    }

    this->setAttribute(Qt::WA_Hover, true);
    ui->bottomWidgets->setAttribute(Qt::WA_NoMousePropagation);

    bool isRememberWindowPosAndSize =
        settings
            .value("appearance/rememberWindowPosAndSize",
                   QVariant::fromValue(
                       PrefConstants::REMEMBER_WINDOW_POS_AND_SIZE))
            .toBool();
    if (isRememberWindowPosAndSize) {
        this->loadPosAndSize();
    } else {
        this->setGeometry(QStyle::alignedRect(
            Qt::LeftToRight, Qt::AlignCenter, this->size(),
            qApp->desktop()->availableGeometry()));
    }

    bool resetSpeedFactorOnLaunch =
        settings
            .value("gen/resetSpeedFactorOnLaunch",
                   QVariant::fromValue(
                       PrefConstants::RESET_SPEED_FACTOR_ON_LAUNCH))
            .toBool();
    if (resetSpeedFactorOnLaunch)
        settings.setValue("gen/speedFactor", PrefConstants::SPEED_FACTOR);

    // --- ARD-parity setup ---

    // Vocab store (must be before learningMode connects)
    vocabStore = new VocabStore(this);

    // Learning mode
    learningMode = new LearningMode(this);
    connect(learningMode, &LearningMode::stateChanged, this,
            &MainWindow::onLearningModeStateChanged);
    connect(learningMode, &LearningMode::recallNeeded, vocabStore,
            [this](const QString &sentence) {
                if (vocabStore)
                    vocabStore->markRecallNeeded(sentence);
            });

    // Vocab panel (hidden initially)
    vocabPanel = new VocabPanel(vocabStore);
    vocabPanel->hide();

    // C-x prefix timer
    m_ctrlXTimer = new QTimer(this);
    m_ctrlXTimer->setSingleShot(true);
    m_ctrlXTimer->setInterval(2000);
    connect(m_ctrlXTimer, &QTimer::timeout, [this]() {
        m_ctrlXPending = false;
    });

    // Install app-wide event filter for keyboard shortcuts
    qApp->installEventFilter(this);

    this->loadPref();
    setAcceptDrops(true);

    loadVocabFileFromSettings();
}

MainWindow::~MainWindow() {
    settings.setValue("appearance/windowX", this->x());
    settings.setValue("appearance/windowY", this->y());
    settings.setValue("appearance/windowWidth", this->width());
    settings.setValue("appearance/windowHeight", this->height());
    if (vocabStore) {
        settings.setValue("learning/knownWords",
                          vocabStore->knownWordsList());
        settings.setValue("learning/recallSentences",
                          vocabStore->recallSentencesList());
    }
    delete menu;
    delete engine;
    delete translationEngine;
    delete ui;
}

// -------------------------------------------------------------------------
// Public slots
// -------------------------------------------------------------------------

void MainWindow::update() {
    if (!engine)
        return;

    if (currentTime >= engine->getFinishTime()) {
        setPlay(false);
        setup();
        return;
    }

    double add = speedFactor * INTERVAL;
    double intpart;
    double fracpart = modf(add, &intpart);
    intervalRemainder += fracpart;
    double remainderIntpart;
    intervalRemainder = modf(intervalRemainder, &remainderIntpart);
    currentTime += (int)intpart + (int)remainderIntpart;

    QString subtitleHtml = getSubtitle(skipped);
    skipped = false;

    // Update learning mode when subtitle changes
    if (m_learningModeEnabled && learningMode) {
        // Strip HTML for text comparison
        QString plain = subtitleHtml;
        plain.remove(QRegularExpression("<[^>]*>"));
        if (plain != m_lastSubtitleText) {
            learningMode->onSubtitleChanged(plain);
            m_lastSubtitleText = plain;
        }
    }

    // Apply highlights
    if (m_inlineHighlightsEnabled && !subtitleHtml.isEmpty())
        subtitleHtml = applyVocabHighlights(subtitleHtml);

    ui->subtitleLabel->setText(subtitleHtml);
    updateHighlights(subtitleHtml);
    updateLegend(subtitleHtml);
    updateTranslationDisplay();

    if (vocabPanel && vocabPanel->isVisible())
        vocabPanel->setCurrentSubtitle(m_lastSubtitleText);

    ui->timeLabel->setText(Engine::millisToTimeString(currentTime) + " / " +
                           Engine::millisToTimeString(engine->getFinishTime()));
    ui->horizontalSlider->setValue((int)(currentTime / SLIDER_RATIO));
}

void MainWindow::sliderMoved(int val) {
    if (!engine)
        return;
    currentTime = val * SLIDER_RATIO;
    QString subtitleHtml = getSubtitle(true);
    if (m_inlineHighlightsEnabled && !subtitleHtml.isEmpty())
        subtitleHtml = applyVocabHighlights(subtitleHtml);
    ui->subtitleLabel->setText(subtitleHtml);
    updateLegend(subtitleHtml);
    updateTranslationDisplay();
    ui->timeLabel->setText(Engine::millisToTimeString(currentTime) + " / " +
                           Engine::millisToTimeString(engine->getFinishTime()));
}

void MainWindow::togglePlay() {
    if (!engine)
        return;
    setPlay(!isPlaying);
}

void MainWindow::showToggleContextMenu(const QPoint &pos) {
    if (ui->toggleButton->isEnabled()) {
        QPoint globalPos = ui->toggleButton->mapToGlobal(pos);
        QMenu menu;
        menu.addAction(tr("Next Click Counts"));
        QAction *selectedItem = menu.exec(globalPos);
        if (selectedItem)
            activateNextClickCounts();
    }
}

void MainWindow::fastForward() {
    adjustTime(getAdjustInterval());
    update();
}

void MainWindow::fastBackward() {
    adjustTime(-getAdjustInterval());
    update();
}

void MainWindow::next() {
    if (!engine)
        return;
    currentTime = engine->getTimeWithSubtitleOffset(currentTime, 1);
    skipped = true;
    update();
}

void MainWindow::previous() {
    if (!engine)
        return;
    currentTime = engine->getTimeWithSubtitleOffset(currentTime, -1);
    skipped = true;
    update();
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason) {
    Q_UNUSED(reason);
}

void MainWindow::openSettingsWindow() {
    this->hide();
    ConfigDialog dialog;
    dialog.exec();
    this->show();
    this->loadPref();
    loadVocabFileFromSettings();
}

void MainWindow::openFileDialog() {
    this->hide();
    QString dir = settings.value("gen/dir").toString();
    if (!QDir(dir).exists())
        dir = "";
    QString path = QFileDialog::getOpenFileName(
        0, tr("Open Subtitle File"), dir,
        tr("Subtitle Files") + " (" + Parser().getFileDialogExt() + ")");
    if (!path.isNull())
        load(path);
    this->show();
}

void MainWindow::openSkipToTimeDialog() {
    if (!engine)
        return;
    this->hide();
    bool ok;
    QString timeStr = QInputDialog::getText(
        0, tr("Skip to Time"), tr("Skip to Time"), QLineEdit::Normal,
        Engine::millisToTimeString(currentTime), &ok);
    if (ok) {
        QRegularExpression timeRegex("^(\\d+):(\\d+):(\\d+)$");
        QRegularExpressionMatchIterator it = timeRegex.globalMatch(timeStr);
        if (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            long long time = Engine::calculateTime(match.captured(1),
                                                   match.captured(2),
                                                   match.captured(3), "0");
            currentTime = qMin(engine->getFinishTime(), qMax(0LL, time));
            update();
        }
    }
    this->show();
}

void MainWindow::activateNextClickCounts() {
    this->hide();
    NccDialog dialog(!isPlaying);
    dialog.exec();
    this->show();
    if (dialog.result() == QDialog::Accepted) {
        if (!engine)
            return;
        setPlay(!isPlaying);
    }
}

// -------------------------------------------------------------------------
// ARD-parity slots
// -------------------------------------------------------------------------

void MainWindow::revealTranslation() {
    if (learningMode)
        learningMode->onReveal();
}

void MainWindow::toggleVocabPanel() {
    if (!vocabPanel)
        return;
    if (vocabPanel->isVisible()) {
        vocabPanel->hide();
    } else {
        vocabPanel->refresh();
        vocabPanel->show();
        vocabPanel->raise();
    }
}

void MainWindow::toggleLearningMode() {
    m_learningModeEnabled = !m_learningModeEnabled;
    settings.setValue("learning/enabled", m_learningModeEnabled);
    if (learningMode)
        learningMode->setActive(m_learningModeEnabled);
    updateTranslationDisplay();

    // Brief toast-like update of hint
    QString modeText = m_learningModeEnabled
        ? "<span style='color:#6BCB77;'>⟳ Learning Mode ON</span>"
          "<span style='color:rgba(160,160,160,160);'>  (C-x l to disable)</span>"
        : "<span style='color:#4D96FF;'>◎ Passive Mode ON</span>"
          "<span style='color:rgba(160,160,160,160);'>  (C-x l to enable learning)</span>";
    ui->hintLabel->setText(modeText);
    ui->hintLabel->setVisible(true);
    QTimer::singleShot(2500, [this]() {
        bool keepHint = learningMode &&
                        learningMode->state() == LearningMode::HintShowing;
        if (keepHint) {
            ui->hintLabel->setText(
                "<span style='color:rgba(180,180,180,180);'>▸ click here or press&nbsp;</span>"
                "<span style='color:#FFD93D;font-weight:bold;'>T</span>"
                "<span style='color:rgba(180,180,180,180);'>&nbsp;to reveal translation</span>");
        } else {
            ui->hintLabel->setVisible(false);
        }
    });
}

void MainWindow::toggleInlineHighlights() {
    m_inlineHighlightsEnabled = !m_inlineHighlightsEnabled;
    settings.setValue("learning/inlineHighlights", m_inlineHighlightsEnabled);
    ui->legendLabel->setVisible(m_inlineHighlightsEnabled &&
                                !ui->legendLabel->text().isEmpty());
}

void MainWindow::showHelp() {
    HelpDialog dlg(this);
    dlg.exec();
}

void MainWindow::loadTranslationFile() {
    this->hide();
    QString dir = settings.value("gen/dir").toString();
    if (!QDir(dir).exists())
        dir = "";
    QString path = QFileDialog::getOpenFileName(
        0, tr("Open Translation Subtitle File"), dir,
        tr("Subtitle Files") + " (" + Parser().getFileDialogExt() + ")");
    if (!path.isNull()) {
        try {
            QString chardet = charsetDetect(path);
            if (chardet == "ASCII")
                chardet = "UTF-8";
            QString encoding = getEncoding(chardet);
            delete translationEngine;
            translationEngine = new Engine(path, encoding);
            // Brief success toast
            this->show();
            ui->hintLabel->setText(
                "<span style='color:#6BCB77;'>✓ Translation subtitle loaded</span>");
            ui->hintLabel->setVisible(true);
            QTimer::singleShot(2000, [this]() {
                bool keepHint = learningMode &&
                                learningMode->state() == LearningMode::HintShowing;
                if (keepHint) {
                    ui->hintLabel->setText(
                        "<span style='color:rgba(180,180,180,180);'>▸ click here or press&nbsp;</span>"
                        "<span style='color:#FFD93D;font-weight:bold;'>T</span>"
                        "<span style='color:rgba(180,180,180,180);'>&nbsp;to reveal translation</span>");
                } else {
                    ui->hintLabel->setVisible(false);
                }
            });
            return;
        } catch (const std::exception &e) {
            QMessageBox::critical(nullptr, "Error loading translation",
                                  e.what(), QMessageBox::Ok);
        }
    }
    this->show();
}

// -------------------------------------------------------------------------
// Protected
// -------------------------------------------------------------------------

void MainWindow::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setCompositionMode(QPainter::CompositionMode_Clear);
    p.fillRect(this->rect(), Qt::transparent);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    // Make hintLabel clickable
    if (obj == ui->hintLabel && event->type() == QEvent::MouseButtonRelease) {
        revealTranslation();
        return true;
    }

    if (event->type() != QEvent::KeyPress)
        return false;

    QKeyEvent *ke = static_cast<QKeyEvent *>(event);

    // Handle C-x chord (second key of prefix sequence)
    if (m_ctrlXPending) {
        m_ctrlXPending = false;
        m_ctrlXTimer->stop();
        int key = ke->key();
        Qt::KeyboardModifiers mod = ke->modifiers();
        if (key == Qt::Key_X && mod == Qt::ControlModifier) {
            toggleVocabPanel();
            return true;
        }
        if (key == Qt::Key_L && (mod == Qt::NoModifier || mod == Qt::ShiftModifier)) {
            toggleLearningMode();
            return true;
        }
        if (key == Qt::Key_H && (mod == Qt::NoModifier || mod == Qt::ShiftModifier)) {
            toggleInlineHighlights();
            return true;
        }
        if (key == Qt::Key_Question ||
            (key == Qt::Key_Slash && mod == Qt::ShiftModifier)) {
            showHelp();
            return true;
        }
        return false; // unknown chord
    }

    // Ctrl+X starts the prefix (app-wide)
    if (ke->key() == Qt::Key_X && ke->modifiers() == Qt::ControlModifier) {
        m_ctrlXPending = true;
        m_ctrlXTimer->start();
        return true;
    }

    // The shortcuts below require Penguin to be the active window
    if (!this->isActiveWindow() && !vocabPanel->isActiveWindow())
        return false;

    // T = reveal translation
    if (ke->key() == Qt::Key_T && ke->modifiers() == Qt::NoModifier) {
        revealTranslation();
        return true;
    }

    // Ctrl++ / Ctrl+= = increase opacity
    if (ke->modifiers() == Qt::ControlModifier &&
        (ke->key() == Qt::Key_Plus || ke->key() == Qt::Key_Equal)) {
        adjustVocabOpacity(0.1);
        return true;
    }

    // Ctrl+- = decrease opacity
    if (ke->modifiers() == Qt::ControlModifier && ke->key() == Qt::Key_Minus) {
        adjustVocabOpacity(-0.1);
        return true;
    }

    // Esc = close help (if it's open) or ignore
    if (ke->key() == Qt::Key_Escape) {
        // handled by HelpDialog itself
        return false;
    }

    return false;
}

// -------------------------------------------------------------------------
// Private
// -------------------------------------------------------------------------

void MainWindow::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasUrls() && e->mimeData()->urls().size() == 1)
        e->acceptProposedAction();
}

void MainWindow::dragMoveEvent() {}

void MainWindow::dropEvent(QDropEvent *e) {
    this->hide();
    QString path = e->mimeData()->urls()[0].toLocalFile();
    int index = path.lastIndexOf(".");
    QString ext = index == -1 ? "" : path.mid(index);
    if (!path.isNull() && index != -1 && Parser().hasParser(ext)) {
        // If engine already loaded, treat second drop as translation
        if (engine) {
            try {
                QString chardet = charsetDetect(path);
                if (chardet == "ASCII") chardet = "UTF-8";
                QString encoding = getEncoding(chardet);
                delete translationEngine;
                translationEngine = new Engine(path, encoding);
            } catch (...) {
                load(path);
            }
        } else {
            load(path);
        }
    }
    this->show();
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    if (ui->topSpacer->underMouse()) {
        ui->topSpacer->setCursor(Qt::ClosedHandCursor);
        m_nMouseClick_X_Coordinate = event->x();
        m_nMouseClick_Y_Coordinate = event->y();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    ui->topSpacer->setCursor(Qt::OpenHandCursor);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (ui->topSpacer->underMouse()) {
        move(event->globalX() - m_nMouseClick_X_Coordinate,
             event->globalY() - m_nMouseClick_Y_Coordinate);
    }
}

void MainWindow::enterEvent(QEvent *event) {
    Q_UNUSED(event);
    ui->topWidgets->show();
    ui->bottomWidgets->show();
}

void MainWindow::leaveEvent(QEvent *event) {
    Q_UNUSED(event);
    ui->topWidgets->hide();
    ui->bottomWidgets->hide();
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event);
    if (engine)
        ui->subtitleLabel->setText(getSubtitle(false));
}

void MainWindow::loadPosAndSize() {
    int x = settings
                .value("appearance/windowX",
                       QVariant::fromValue(PrefConstants::WINDOW_X))
                .toInt();
    int y = settings
                .value("appearance/windowY",
                       QVariant::fromValue(PrefConstants::WINDOW_Y))
                .toInt();
    int w = settings
                .value("appearance/windowWidth",
                       QVariant::fromValue(PrefConstants::WINDOW_WIDTH))
                .toInt();
    int h = settings
                .value("appearance/windowHeight",
                       QVariant::fromValue(PrefConstants::WINDOW_HEIGHT))
                .toInt();
    this->setGeometry(x, y, w, h);
}

void MainWindow::loadPref() {
    speedFactor =
        settings
            .value("gen/speedFactor",
                   QVariant::fromValue(PrefConstants::SPEED_FACTOR))
            .toDouble();

    QColor bgColor = QColor::fromRgb(
        settings
            .value("appearance/bgColor",
                   QVariant::fromValue(PrefConstants::BG_COLOR))
            .toUInt());
    int bgAlpha =
        settings
            .value("appearance/bgAlpha",
                   QVariant::fromValue(PrefConstants::BG_ALPHA))
            .toInt();
    this->setStyleSheet(
        QString("background-color:rgba(%1,%2,%3,%4)")
            .arg(bgColor.red())
            .arg(bgColor.green())
            .arg(bgColor.blue())
            .arg(bgAlpha));

    QColor fontColor = QColor::fromRgb(
        settings
            .value("appearance/fontColor",
                   QVariant::fromValue(PrefConstants::FONT_COLOR))
            .toUInt());
    ui->subtitleLabel->setStyleSheet(
        QString("background-color:transparent; color:rgba(%1,%2,%3)")
            .arg(fontColor.red())
            .arg(fontColor.green())
            .arg(fontColor.blue()));

    QFont f;
    f.fromString(
        settings.value("appearance/font", PrefConstants::FONT).toString());
    ui->subtitleLabel->setFont(f);

    // Translation label at ~65% of subtitle font size (minimum 11pt)
    QFont tf = f;
    tf.setPointSize(qMax(11, (int)(f.pointSize() * 0.65)));
    ui->translationLabel->setFont(tf);

    bool fontShadowEnable =
        settings
            .value("appearance/fontShadowEnable",
                   QVariant::fromValue(PrefConstants::FONT_SHADOW_ENABLE))
            .toBool();
    if (fontShadowEnable) {
        QGraphicsDropShadowEffect *dse = new QGraphicsDropShadowEffect();
        dse->setBlurRadius(
            settings
                .value("appearance/fontShadowBlurRadius",
                       QVariant::fromValue(
                           PrefConstants::FONT_SHADOW_BLUR_RADIUS))
                .toInt());
        dse->setOffset(
            settings
                .value("appearance/fontShadowOffsetX",
                       QVariant::fromValue(PrefConstants::FONT_SHADOW_OFFSET_X))
                .toReal(),
            settings
                .value("appearance/fontShadowOffsetY",
                       QVariant::fromValue(PrefConstants::FONT_SHADOW_OFFSET_Y))
                .toReal());
        QColor fontShadowColor = QColor::fromRgb(
            settings
                .value("appearance/fontShadowColor",
                       QVariant::fromValue(PrefConstants::FONT_SHADOW_COLOR))
                .toUInt());
        dse->setColor(fontShadowColor);
        ui->subtitleLabel->setGraphicsEffect(dse);
    } else {
        ui->subtitleLabel->setGraphicsEffect(nullptr);
    }

    // Learning mode prefs
    m_learningModeEnabled =
        settings
            .value("learning/enabled",
                   QVariant::fromValue(PrefConstants::LEARNING_MODE_ENABLED))
            .toBool();
    m_inlineHighlightsEnabled =
        settings
            .value("learning/inlineHighlights",
                   QVariant::fromValue(PrefConstants::INLINE_HIGHLIGHTS_ENABLED))
            .toBool();

    if (learningMode) {
        learningMode->setActive(m_learningModeEnabled);
        learningMode->setThinkTime(
            settings
                .value("learning/thinkTime",
                       QVariant::fromValue(PrefConstants::LEARNING_THINK_TIME))
                .toInt());
        learningMode->setAutoHideDelay(
            settings
                .value("learning/autoHideDelay",
                       QVariant::fromValue(
                           PrefConstants::LEARNING_AUTO_HIDE_DELAY))
                .toInt());
    }

    if (vocabStore) {
        QStringList known =
            settings.value("learning/knownWords").toStringList();
        QStringList recall =
            settings.value("learning/recallSentences").toStringList();
        vocabStore->setKnownWords(known);
        vocabStore->setRecallSentences(recall);
    }

    if (vocabPanel) {
        double opacity = settings
                             .value("learning/vocabPanelOpacity",
                                    PrefConstants::VOCAB_PANEL_OPACITY)
                             .toDouble();
        vocabPanel->setWindowOpacity(opacity);
    }
}

void MainWindow::load(QString path) {
    try {
        QString chardet = charsetDetect(path);
        qDebug() << "Detected Charset:" << chardet;
        if (chardet == "ASCII")
            chardet = "UTF-8";
        QString encoding = getEncoding(chardet);
        delete engine;
        engine = new Engine(path, encoding);
        // Clear translation engine when loading new primary
        delete translationEngine;
        translationEngine = nullptr;
        setup();
        setPlay(true);
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, "Error loading subtitle", e.what(),
                              QMessageBox::Ok, QMessageBox::Ok);
    }
}

void MainWindow::setup() {
    skipped = true;
    currentTime = 0;
    intervalRemainder = 0;
    m_lastSubtitleText.clear();
    ui->subtitleLabel->setText("");
    ui->hintLabel->setVisible(false);
    ui->translationLabel->setVisible(false);
    ui->legendLabel->setVisible(false);
    ui->timeLabel->setText(Engine::millisToTimeString(0) + " / " +
                           Engine::millisToTimeString(
                               engine->getFinishTime()));
    ui->horizontalSlider->setRange(
        0, (int)(engine->getFinishTime() / SLIDER_RATIO));
    ui->horizontalSlider->setValue(0);
    if (learningMode)
        learningMode->reset();
    enableControls();
}

void MainWindow::enableControls() {
    ui->backwardButton->setEnabled(true);
    ui->forwardButton->setEnabled(true);
    ui->prevButton->setEnabled(true);
    ui->nextButton->setEnabled(true);
    ui->toggleButton->setEnabled(true);
    ui->horizontalSlider->setEnabled(true);
}

void MainWindow::setPlay(bool play) {
    isPlaying = play;
    if (isPlaying) {
        timer->start(INTERVAL);
        if (learningMode)
            learningMode->onPlay();
    } else {
        timer->stop();
        if (learningMode) {
            QString plain = m_lastSubtitleText;
            learningMode->onPause(plain);
        }
    }
    ui->toggleButton->setIcon(
        QIcon(isPlaying ? ":/icons/ic_pause_opt_48px.png"
                        : ":/icons/ic_play_opt_48px.png"));
}

QString MainWindow::getSubtitle(bool sliderMoved) {
    return engine->currentSubtitle(currentTime, sliderMoved);
}

QString MainWindow::getEncoding(QString preset) {
    QStringList codecNames;
    QList<QByteArray> codecs = QTextCodec::availableCodecs();
    for (QList<QByteArray>::const_iterator it = codecs.constBegin();
         it != codecs.constEnd(); it++)
        codecNames.push_back(it->constData());
    codecNames.sort();
    codecNames.removeDuplicates();

    int recommendIndex = -1;
    QStringList defaultEncodingList =
        codecNames.filter(PrefConstants::ENCODING, Qt::CaseInsensitive);
    if (defaultEncodingList.size() > 0)
        recommendIndex = codecNames.indexOf(defaultEncodingList[0]);

    if (!preset.isEmpty()) {
        QStringList list = codecNames.filter(preset, Qt::CaseInsensitive);
        if (list.size() > 0)
            recommendIndex = codecNames.indexOf(list[0]);
    }

    if (!settings
             .value("gen/useDetectedEncoding",
                    QVariant::fromValue(PrefConstants::USE_DETECTED_ENCODING))
             .toBool()) {
        QString chosenEncoding = promptForEncoding(codecNames, recommendIndex);
        if (!chosenEncoding.isEmpty())
            return chosenEncoding;
    }

    if (codecNames.size() == 0)
        return "";
    if (recommendIndex == -1)
        recommendIndex = 0;
    return codecNames[recommendIndex];
}

QString MainWindow::promptForEncoding(QStringList codecNames,
                                      int recommendIndex) {
    const QString RECOMMEND = " (Recommended)";
    if (recommendIndex != -1)
        codecNames[recommendIndex] += RECOMMEND;
    bool ok;
    QString encoding =
        QInputDialog::getItem(0, tr("Select Encoding"), tr("Select Encoding"),
                              codecNames, recommendIndex, false, &ok);
    if (ok)
        return encoding.replace(RECOMMEND, "");
    return "";
}

void MainWindow::adjustTime(long long interval) {
    if (!engine)
        return;
    currentTime =
        qMin(engine->getFinishTime(), qMax(0LL, currentTime + interval));
}

long long MainWindow::getAdjustInterval() {
    return settings
        .value("gen/adjust",
               QVariant::fromValue(PrefConstants::ADJUST_INTERVAL))
        .toInt();
}

// -------------------------------------------------------------------------
// ARD-parity helpers
// -------------------------------------------------------------------------

void MainWindow::onLearningModeStateChanged(LearningMode::State state) {
    updateTranslationDisplay();
    switch (state) {
    case LearningMode::Idle:
        ui->hintLabel->setVisible(false);
        break;
    case LearningMode::Hidden:
        ui->hintLabel->setVisible(false);
        break;
    case LearningMode::HintShowing:
        ui->hintLabel->setText(
            "<span style='color:rgba(180,180,180,180);'>▸ click here or press&nbsp;</span>"
            "<span style='color:#FFD93D;font-weight:bold;'>T</span>"
            "<span style='color:rgba(180,180,180,180);'>&nbsp;to reveal translation</span>");
        ui->hintLabel->setVisible(true);
        break;
    case LearningMode::Revealed:
        ui->hintLabel->setVisible(false);
        break;
    }
}

void MainWindow::updateTranslationDisplay() {
    if (!translationEngine) {
        ui->translationLabel->setVisible(false);
        return;
    }

    QString translation = translationEngine->currentSubtitle(currentTime, false);

    if (!m_learningModeEnabled) {
        // Passive mode: always show
        ui->translationLabel->setText(translation);
        ui->translationLabel->setVisible(!translation.isEmpty());
        return;
    }

    if (!learningMode) {
        ui->translationLabel->setVisible(false);
        return;
    }

    LearningMode::State state = learningMode->state();
    if (state == LearningMode::Revealed) {
        ui->translationLabel->setText(translation);
        ui->translationLabel->setVisible(!translation.isEmpty());
    } else {
        ui->translationLabel->setVisible(false);
    }
}

void MainWindow::updateHighlights(const QString &subtitleHtml) {
    Q_UNUSED(subtitleHtml);
    // Highlight processing happens inside applyVocabHighlights called before setText
}

void MainWindow::updateLegend(const QString &subtitleHtml) {
    if (!m_inlineHighlightsEnabled || !vocabStore) {
        ui->legendLabel->setVisible(false);
        return;
    }
    QVector<VocabWord> active = vocabStore->wordsForSubtitle(subtitleHtml);
    if (active.isEmpty()) {
        ui->legendLabel->setVisible(false);
        return;
    }

    QString html;
    for (const VocabWord &w : active) {
        QString color = VOCAB_COLORS[w.colorIndex % VOCAB_COLOR_COUNT];
        html += QString(
                    "<span style='color:%1;font-weight:bold;'>%2</span>"
                    "<span style='color:#cccccc;'> = %3</span>  ")
                    .arg(color, w.baseForm().toHtmlEscaped(),
                         w.meaning.toHtmlEscaped());
    }
    ui->legendLabel->setText(html.trimmed());
    ui->legendLabel->setVisible(true);
}

QString MainWindow::applyVocabHighlights(const QString &html) {
    if (!vocabStore || vocabStore->words().isEmpty())
        return html;

    // Tokenize into alternating text nodes and HTML tags
    QRegularExpression tagRe("<[^>]+>");
    QStringList parts;
    QVector<bool> isTag;
    int pos = 0;
    auto it = tagRe.globalMatch(html);
    while (it.hasNext()) {
        auto m = it.next();
        if (m.capturedStart() > pos) {
            parts << html.mid(pos, m.capturedStart() - pos);
            isTag << false;
        }
        parts << m.captured();
        isTag << true;
        pos = m.capturedEnd();
    }
    if (pos < html.length()) {
        parts << html.mid(pos);
        isTag << false;
    }

    // Replace word matches only in text nodes, leaving tags untouched
    for (int i = 0; i < parts.size(); ++i) {
        if (isTag[i])
            continue;
        QString text = parts[i];
        for (const VocabWord &w : vocabStore->words()) {
            QString base = w.baseForm();
            if (base.isEmpty())
                continue;
            QString color = VOCAB_COLORS[w.colorIndex % VOCAB_COLOR_COUNT];
            QRegularExpression wordRe(
                "\\b(" + QRegularExpression::escape(base) + ")\\b",
                QRegularExpression::CaseInsensitiveOption);
            text.replace(wordRe,
                QString("<span style='border-bottom:2px solid %1;"
                        "color:inherit;'>\\1</span>").arg(color));
        }
        parts[i] = text;
    }
    return parts.join("");
}

void MainWindow::adjustVocabOpacity(double delta) {
    if (!vocabPanel)
        return;
    vocabPanel->adjustOpacity(delta);
    double op = vocabPanel->windowOpacity();
    settings.setValue("learning/vocabPanelOpacity", op);
}

void MainWindow::loadVocabFileFromSettings() {
    if (!vocabStore)
        return;
    QString path = settings.value("learning/vocabFile").toString();
    if (!path.isEmpty() && QFile::exists(path))
        vocabStore->loadFromFile(path);
}
