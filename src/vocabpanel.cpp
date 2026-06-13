#include "vocabpanel.h"
#include "vocabword.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QPainter>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSizeGrip>
#include <QStyleOption>
#include <QTextStream>
#include <QVector>

VocabPanel::VocabPanel(VocabStore *store, QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint |
                          Qt::WindowStaysOnTopHint),
      m_store(store) {
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(280, 200);
    resize(380, 500);

    setWindowOpacity(m_opacity);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setSpacing(0);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    // Header
    m_header = new QWidget(this);
    m_header->setObjectName("vpHeader");
    m_header->setFixedHeight(36);
    m_header->setStyleSheet(
        "#vpHeader { background: rgba(30,30,30,230); border-radius: 0px; }");
    m_header->setCursor(Qt::OpenHandCursor);

    auto *headerLayout = new QHBoxLayout(m_header);
    headerLayout->setContentsMargins(10, 0, 6, 0);
    m_titleLabel = new QLabel("Vocabulary Panel", m_header);
    m_titleLabel->setStyleSheet("color: #dddddd; font-size: 13px; font-weight: bold;");
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch(1);

    auto *closeBtn = new QPushButton("✕", m_header);
    closeBtn->setFixedSize(24, 24);
    closeBtn->setStyleSheet(
        "QPushButton { background: transparent; color: #aaaaaa; border: none; "
        "font-size: 14px; } QPushButton:hover { color: white; }");
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::hide);
    headerLayout->addWidget(closeBtn);

    outerLayout->addWidget(m_header);

    // Content background
    auto *body = new QWidget(this);
    body->setStyleSheet("background: rgba(20,20,20,210);");
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 4, 0, 0);
    bodyLayout->setSpacing(0);

    // Toolbar
    auto *toolbar = new QWidget(body);
    auto *tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(8, 0, 8, 4);
    tbLayout->setSpacing(6);

    auto *loadBtn = new QPushButton("Load vocab.json", toolbar);
    loadBtn->setStyleSheet(
        "QPushButton { background: #3a5a3a; color: #ddffdd; border: none; "
        "padding: 4px 8px; font-size: 11px; border-radius: 3px; } "
        "QPushButton:hover { background: #4a7a4a; }");
    connect(loadBtn, &QPushButton::clicked, this, &VocabPanel::loadVocabFile);
    tbLayout->addWidget(loadBtn);

    auto *exportBtn = new QPushButton("Export Anki TSV", toolbar);
    exportBtn->setStyleSheet(
        "QPushButton { background: #3a3a6a; color: #ddddff; border: none; "
        "padding: 4px 8px; font-size: 11px; border-radius: 3px; } "
        "QPushButton:hover { background: #4a4a8a; }");
    connect(exportBtn, &QPushButton::clicked, this, &VocabPanel::exportToAnki);
    tbLayout->addWidget(exportBtn);
    tbLayout->addStretch(1);

    bodyLayout->addWidget(toolbar);

    // Scroll area
    m_scroll = new QScrollArea(body);
    m_scroll->setWidgetResizable(true);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setStyleSheet("QScrollArea { border: none; background: transparent; }");

    m_content = new QWidget();
    m_content->setStyleSheet("background: transparent;");
    m_contentLayout = new QVBoxLayout(m_content);
    m_contentLayout->setSpacing(4);
    m_contentLayout->setContentsMargins(6, 4, 6, 4);
    m_contentLayout->addStretch(1);

    m_scroll->setWidget(m_content);
    bodyLayout->addWidget(m_scroll, 1);

    // Resize grip
    auto *grip = new QSizeGrip(body);
    grip->setStyleSheet("QSizeGrip { background: transparent; }");
    auto *gripLayout = new QHBoxLayout();
    gripLayout->setContentsMargins(0, 0, 2, 2);
    gripLayout->addStretch(1);
    gripLayout->addWidget(grip);
    bodyLayout->addLayout(gripLayout);

    outerLayout->addWidget(body, 1);

    connect(m_store, &VocabStore::wordsChanged, this, &VocabPanel::refresh);

    // Position top-right
    QRect avail = QApplication::desktop()->availableGeometry();
    move(avail.right() - width() - 20, avail.top() + 60);
}

void VocabPanel::setCurrentSubtitle(const QString &subtitle) {
    if (subtitle == m_currentSubtitle)
        return;
    if (!m_currentSubtitle.isEmpty()) {
        m_recentSubtitles.prepend(m_currentSubtitle);
        if (m_recentSubtitles.size() > 5)
            m_recentSubtitles.removeLast();
    }
    m_currentSubtitle = subtitle;
    if (isVisible())
        refresh();
}

void VocabPanel::adjustOpacity(double delta) {
    m_opacity = qBound(0.2, m_opacity + delta, 1.0);
    setWindowOpacity(m_opacity);
    m_titleLabel->setText(
        QString("Vocabulary Panel  [opacity %1%]").arg((int)(m_opacity * 100)));
}

void VocabPanel::refresh() {
    clearGroups();
    buildGroups();
}

void VocabPanel::clearGroups() {
    QLayoutItem *item;
    while ((item = m_contentLayout->takeAt(0)) != nullptr) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }
    m_contentLayout->addStretch(1);
}

void VocabPanel::buildGroups() {
    const QVector<VocabWord> &words = m_store->words();
    if (words.isEmpty()) {
        auto *empty = new QLabel("No vocabulary loaded.\nClick 'Load vocab.json' to add words.");
        empty->setStyleSheet("color: #888888; font-size: 12px; padding: 12px;");
        empty->setAlignment(Qt::AlignCenter);
        empty->setWordWrap(true);
        m_contentLayout->insertWidget(0, empty);
        return;
    }

    // Group words by exampleDe sentence
    QMap<QString, QVector<VocabWord>> groups;
    QStringList groupOrder;
    for (const VocabWord &w : words) {
        QString key = w.exampleDe.isEmpty() ? "(no example)" : w.exampleDe;
        if (!groups.contains(key))
            groupOrder.append(key);
        groups[key].append(w);
    }

    // Determine active/recent groups
    auto wordsInSubtitle = [&](const QString &sub) -> QStringList {
        QStringList sentences;
        for (auto it = groups.begin(); it != groups.end(); ++it) {
            for (const VocabWord &w : it.value()) {
                QString plain = sub;
                plain.remove(QRegularExpression("<[^>]*>"));
                QString base = w.baseForm();
                if (!base.isEmpty()) {
                    QRegularExpression re(
                        "\\b" + QRegularExpression::escape(base) + "\\b",
                        QRegularExpression::CaseInsensitiveOption);
                    if (re.match(plain).hasMatch()) {
                        sentences.append(it.key());
                        break;
                    }
                }
            }
        }
        return sentences;
    };

    QStringList activeSentences = wordsInSubtitle(m_currentSubtitle);
    QStringList recentSentences;
    for (const QString &r : m_recentSubtitles)
        for (const QString &s : wordsInSubtitle(r))
            if (!activeSentences.contains(s) && !recentSentences.contains(s))
                recentSentences.append(s);

    // Sort: active first, recent second, rest in order
    QStringList ordered;
    for (const QString &s : activeSentences)
        if (groupOrder.contains(s))
            ordered.append(s);
    for (const QString &s : recentSentences)
        if (groupOrder.contains(s))
            ordered.append(s);
    for (const QString &s : groupOrder)
        if (!ordered.contains(s))
            ordered.append(s);

    int insertPos = 0;
    for (const QString &sentence : ordered) {
        if (!groups.contains(sentence))
            continue;
        bool active = activeSentences.contains(sentence);
        bool recent = recentSentences.contains(sentence);
        QWidget *grp =
            buildGroupWidget(sentence, groups[sentence], active, recent);
        m_contentLayout->insertWidget(insertPos++, grp);
    }
}

QWidget *VocabPanel::buildGroupWidget(const QString &sentence,
                                      const QVector<VocabWord> &words,
                                      bool active, bool recent) {
    auto *frame = new QFrame();
    frame->setFrameShape(QFrame::StyledPanel);
    QString bgColor = active ? "rgba(0,120,120,180)"
                             : (recent ? "rgba(120,110,20,120)"
                                       : "rgba(40,40,40,160)");
    frame->setStyleSheet(
        QString("QFrame { background: %1; border-radius: 5px; }").arg(bgColor));

    auto *layout = new QVBoxLayout(frame);
    layout->setSpacing(3);
    layout->setContentsMargins(8, 6, 8, 6);

    // Sentence header
    if (!sentence.isEmpty() && sentence != "(no example)") {
        bool anyRecall =
            std::any_of(words.begin(), words.end(),
                        [](const VocabWord &w) { return w.recallNeeded; });
        auto *sentLabel = new QLabel(
            (anyRecall ? "● " : "○ ") + sentence);
        sentLabel->setStyleSheet(
            QString("color: %1; font-size: 12px; font-style: italic;")
                .arg(anyRecall ? "#ff8888" : "#aaaaaa"));
        sentLabel->setWordWrap(true);
        layout->addWidget(sentLabel);
    }

    // Word rows
    for (const VocabWord &w : words) {
        auto *row = new QWidget(frame);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 1, 0, 1);
        rowLayout->setSpacing(6);

        // Recall dot
        auto *dot = new QLabel(w.recallNeeded ? "●" : "·");
        dot->setStyleSheet(
            QString("color: %1; font-size: 14px;")
                .arg(w.recallNeeded ? "#ff6666" : "#666666"));
        dot->setFixedWidth(14);
        rowLayout->addWidget(dot);

        // Color swatch
        auto *swatch = new QLabel("▐");
        swatch->setStyleSheet(
            QString("color: %1; font-size: 16px;")
                .arg(VOCAB_COLORS[w.colorIndex % VOCAB_COLOR_COUNT]));
        swatch->setFixedWidth(16);
        rowLayout->addWidget(swatch);

        // Word + type + meaning
        auto *wordLabel = new QLabel(
            QString("<b>%1</b> <span style='color:#aaaaaa'>(%2)</span> = %3")
                .arg(w.word.toHtmlEscaped(), w.type.toHtmlEscaped(),
                     w.meaning.toHtmlEscaped()));
        wordLabel->setStyleSheet("color: #eeeeee; font-size: 12px;");
        wordLabel->setWordWrap(true);
        wordLabel->setTextFormat(Qt::RichText);
        rowLayout->addWidget(wordLabel, 1);

        // Known button
        auto *knownBtn = new QPushButton("known ✓");
        knownBtn->setFixedHeight(22);
        knownBtn->setStyleSheet(
            "QPushButton { background: #2a4a2a; color: #88cc88; border: 1px "
            "solid #3a6a3a; padding: 0 6px; font-size: 10px; border-radius: "
            "3px; } QPushButton:hover { background: #3a6a3a; }");
        QString wordCopy = w.word;
        VocabStore *store = m_store;
        connect(knownBtn, &QPushButton::clicked, [store, wordCopy]() {
            store->markKnown(wordCopy);
        });
        rowLayout->addWidget(knownBtn);

        layout->addWidget(row);
    }

    return frame;
}

void VocabPanel::exportToAnki() {
    QString path = QFileDialog::getSaveFileName(
        this, "Export Anki TSV", "vocab_anki.tsv", "TSV files (*.tsv)");
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    for (const VocabWord &w : m_store->words()) {
        QString front =
            QString("%1 (%2) = %3").arg(w.word, w.type, w.meaning);
        QString back =
            QString("%1<br>%2").arg(w.exampleDe.toHtmlEscaped(),
                                    w.exampleEn.toHtmlEscaped());
        QStringList tags;
        tags << "penguin-subtitle-player";
        if (w.recallNeeded)
            tags << "recall-needed";
        out << front << "\t" << back << "\t" << tags.join(" ") << "\n";
    }
}

void VocabPanel::loadVocabFile() {
    QString path = QFileDialog::getOpenFileName(
        this, "Load Vocabulary File", "",
        "JSON files (*.json);;All files (*)");
    if (!path.isEmpty())
        m_store->loadFromFile(path);
}

void VocabPanel::mousePressEvent(QMouseEvent *event) {
    if (m_header->geometry().contains(event->pos()) &&
        event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_header->setCursor(Qt::ClosedHandCursor);
        m_dragOffset = event->globalPos() - frameGeometry().topLeft();
    }
    QWidget::mousePressEvent(event);
}

void VocabPanel::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging)
        move(event->globalPos() - m_dragOffset);
    QWidget::mouseMoveEvent(event);
}

void VocabPanel::mouseReleaseEvent(QMouseEvent *event) {
    if (m_dragging) {
        m_dragging = false;
        m_header->setCursor(Qt::OpenHandCursor);
    }
    QWidget::mouseReleaseEvent(event);
}
