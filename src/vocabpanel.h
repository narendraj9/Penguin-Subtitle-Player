#ifndef VOCABPANEL_H
#define VOCABPANEL_H

#include "vocabstore.h"
#include <QFrame>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

class VocabGroup;

class VocabPanel : public QWidget {
    Q_OBJECT
public:
    explicit VocabPanel(VocabStore *store, QWidget *parent = nullptr);

    void setCurrentSubtitle(const QString &subtitle);
    void adjustOpacity(double delta);

public slots:
    void refresh();
    void exportToAnki();
    void loadVocabFile();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void buildGroups();
    void clearGroups();
    QWidget *buildGroupWidget(const QString &sentence,
                              const QVector<VocabWord> &words, bool active,
                              bool recent);

    VocabStore *m_store;
    QWidget *m_header;
    QLabel *m_titleLabel;
    QScrollArea *m_scroll;
    QWidget *m_content;
    QVBoxLayout *m_contentLayout;

    QString m_currentSubtitle;
    QStringList m_recentSubtitles;
    QWidget *m_firstActiveWidget = nullptr;

    bool m_dragging = false;
    QPoint m_dragOffset;
    double m_opacity = 0.92;
};

#endif // VOCABPANEL_H
