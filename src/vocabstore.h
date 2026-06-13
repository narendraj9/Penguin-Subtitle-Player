#ifndef VOCABSTORE_H
#define VOCABSTORE_H

#include "vocabword.h"
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

class VocabStore : public QObject {
    Q_OBJECT
public:
    explicit VocabStore(QObject *parent = nullptr);

    bool loadFromFile(const QString &path);
    bool saveToFile(const QString &path) const;
    void addWord(const VocabWord &word);
    void markKnown(const QString &word);
    void markRecallNeeded(const QString &exampleDe);
    bool isKnown(const QString &word) const;
    void clear();

    const QVector<VocabWord> &words() const { return m_words; }
    QVector<VocabWord> wordsForSubtitle(const QString &subtitleText) const;
    int count() const { return m_words.size(); }

    void setKnownWords(const QStringList &known);
    QStringList knownWordsList() const;
    void setRecallSentences(const QStringList &sentences);
    QStringList recallSentencesList() const;

signals:
    void wordsChanged();

private:
    QVector<VocabWord> m_words;
    QSet<QString> m_knownWords;
    QSet<QString> m_recallSentences;

    bool wordMatchesSubtitle(const VocabWord &w, const QString &plain) const;
};

#endif // VOCABSTORE_H
