#ifndef LEARNINGMODE_H
#define LEARNINGMODE_H

#include <QObject>
#include <QString>
#include <QTimer>

class LearningMode : public QObject {
    Q_OBJECT
public:
    enum State {
        Idle,        // playing (or learning mode off)
        Hidden,      // paused, translation hidden, counting down
        HintShowing, // think time elapsed, "press T" hint visible
        Revealed     // user pressed T, translation visible
    };
    Q_ENUM(State)

    explicit LearningMode(QObject *parent = nullptr);

    bool isActive() const { return m_active; }
    void setActive(bool active);

    State state() const { return m_state; }

    int thinkTime() const { return m_thinkTime; }
    void setThinkTime(int seconds) { m_thinkTime = seconds; }

    int autoHideDelay() const { return m_autoHideDelay; }
    void setAutoHideDelay(int seconds) { m_autoHideDelay = seconds; }

    void onPause(const QString &subtitleText);
    void onPlay();
    void onReveal();
    void onSubtitleChanged(const QString &newText);
    void reset();

signals:
    void stateChanged(LearningMode::State state);
    void recallNeeded(const QString &subtitleText);

private slots:
    void onThinkTimerExpired();
    void onAutoHideTimerExpired();

private:
    void setState(State s);

    bool m_active = true;
    State m_state = Idle;
    int m_thinkTime = 2;
    int m_autoHideDelay = 0;
    QString m_currentSubtitle;
    QTimer *m_thinkTimer;
    QTimer *m_autoHideTimer;
};

#endif // LEARNINGMODE_H
