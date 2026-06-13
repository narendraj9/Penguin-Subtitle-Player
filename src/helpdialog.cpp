#include "helpdialog.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

static QWidget *makeSection(const QString &title,
                            const QVector<QPair<QString, QString>> &rows) {
    auto *w = new QWidget();
    auto *layout = new QVBoxLayout(w);
    layout->setContentsMargins(0, 8, 0, 8);
    layout->setSpacing(4);

    auto *header = new QLabel(title);
    header->setStyleSheet(
        "font-size: 13px; font-weight: bold; color: #2ec4b6; padding: 2px 0;");
    layout->addWidget(header);

    auto *table = new QTableWidget(rows.size(), 2);
    table->setStyleSheet(
        "QTableWidget { border: none; gridline-color: #333; background: "
        "#1a1a1a; color: #ddd; }"
        "QTableWidget::item { padding: 4px 8px; }"
        "QHeaderView::section { background: #2a2a2a; color: #aaa; padding: "
        "4px; border: none; }");
    table->setHorizontalHeaderLabels({"Shortcut", "Action"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setShowGrid(true);
    table->setAlternatingRowColors(true);

    for (int i = 0; i < rows.size(); ++i) {
        auto *keyItem = new QTableWidgetItem(rows[i].first);
        keyItem->setFont(QFont("Monospace", 11));
        keyItem->setForeground(QColor("#FFD93D"));
        auto *actItem = new QTableWidgetItem(rows[i].second);
        table->setItem(i, 0, keyItem);
        table->setItem(i, 1, actItem);
        table->setRowHeight(i, 28);
    }
    table->resizeColumnToContents(0);
    layout->addWidget(table);
    return w;
}

HelpDialog::HelpDialog(QWidget *parent)
    : QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint) {
    setWindowTitle("Keyboard Shortcuts");
    setMinimumWidth(520);
    setStyleSheet(
        "QDialog { background: #111111; }"
        "QLabel { color: #dddddd; }");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(0);

    auto *titleLabel = new QLabel("Penguin Subtitle Player – Keyboard Shortcuts");
    titleLabel->setStyleSheet(
        "font-size: 16px; font-weight: bold; color: white; padding-bottom: 8px;");
    mainLayout->addWidget(titleLabel);

    mainLayout->addWidget(makeSection(
        "Learning Mode",
        {{"T", "Reveal translation (while paused in learning mode)"},
         {"Ctrl+X, L", "Toggle learning / passive mode"}}));

    mainLayout->addWidget(makeSection(
        "Vocabulary",
        {{"Ctrl+X, Ctrl+X", "Toggle vocabulary panel"},
         {"Ctrl+X, H", "Toggle inline word highlights & legend"},
         {"Ctrl++", "Increase vocabulary panel opacity"},
         {"Ctrl+-", "Decrease vocabulary panel opacity"}}));

    mainLayout->addWidget(makeSection(
        "Navigation & Display",
        {{"Ctrl+X, ?", "Show this help overlay"},
         {"Esc", "Close help overlay"}}));

    auto *note = new QLabel(
        "Tip: Shortcuts work when the Penguin window is focused. "
        "C-x sequences can be initiated from anywhere while Penguin is running.");
    note->setStyleSheet(
        "color: #888888; font-size: 11px; padding-top: 8px;");
    note->setWordWrap(true);
    mainLayout->addWidget(note);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    auto *closeBtn = new QPushButton("Close  [Esc]");
    closeBtn->setStyleSheet(
        "QPushButton { background: #2ec4b6; color: #111; padding: 6px 18px; "
        "border: none; border-radius: 4px; font-weight: bold; } "
        "QPushButton:hover { background: #3dd4c6; }");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);
}

void HelpDialog::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape)
        accept();
    else
        QDialog::keyPressEvent(event);
}
