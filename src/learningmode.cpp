#include "learningmode.h"

LearningMode::LearningMode(QObject *parent)
    : QObject(parent), m_thinkTimer(new QTimer(this)),
      m_autoHideTimer(new QTimer(this)) {
    m_thinkTimer->setSingleShot(true);
    m_autoHideTimer->setSingleShot(true);
    connect(m_thinkTimer, &QTimer::timeout, this,
            &LearningMode::onThinkTimerExpired);
    connect(m_autoHideTimer, &QTimer::timeout, this,
            &LearningMode::onAutoHideTimerExpired);
}

void LearningMode::setActive(bool active) {
    m_active = active;
    if (!m_active) {
        m_thinkTimer->stop();
        m_autoHideTimer->stop();
        setState(Idle);
    }
}

void LearningMode::onPause(const QString &subtitleText) {
    if (!m_active || subtitleText.isEmpty()) {
        setState(Idle);
        return;
    }
    m_currentSubtitle = subtitleText;
    m_autoHideTimer->stop();
    if (m_thinkTime > 0) {
        setState(Hidden);
        m_thinkTimer->start(m_thinkTime * 1000);
    } else {
        setState(HintShowing);
    }
}

void LearningMode::onPlay() {
    m_thinkTimer->stop();
    m_autoHideTimer->stop();
    setState(Idle);
}

void LearningMode::onReveal() {
    if (m_state == Hidden || m_state == HintShowing) {
        emit recallNeeded(m_currentSubtitle);
        setState(Revealed);
        if (m_autoHideDelay > 0)
            m_autoHideTimer->start(m_autoHideDelay * 1000);
    }
}

void LearningMode::onSubtitleChanged(const QString &newText) {
    if (!m_active)
        return;
    if (newText == m_currentSubtitle)
        return;
    m_currentSubtitle = newText;
    m_thinkTimer->stop();
    m_autoHideTimer->stop();
    if (!newText.isEmpty()) {
        if (m_thinkTime > 0) {
            setState(Hidden);
            m_thinkTimer->start(m_thinkTime * 1000);
        } else {
            setState(HintShowing);
        }
    } else {
        setState(Idle);
    }
}

void LearningMode::reset() {
    m_thinkTimer->stop();
    m_autoHideTimer->stop();
    m_currentSubtitle.clear();
    setState(Idle);
}

void LearningMode::onThinkTimerExpired() { setState(HintShowing); }

void LearningMode::onAutoHideTimerExpired() {
    setState(Hidden);
    if (m_thinkTime > 0)
        m_thinkTimer->start(m_thinkTime * 1000);
    else
        setState(HintShowing);
}

void LearningMode::setState(State s) {
    if (m_state != s) {
        m_state = s;
        emit stateChanged(s);
    }
}
