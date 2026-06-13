#include "vocabstore.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <algorithm>

VocabStore::VocabStore(QObject *parent) : QObject(parent) {}

bool VocabStore::loadFromFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isArray())
        return false;
    m_words.clear();
    int idx = 0;
    for (const QJsonValue &v : doc.array()) {
        QJsonObject obj = v.toObject();
        VocabWord w;
        w.word = obj["word"].toString();
        w.type = obj["type"].toString();
        w.meaning = obj["meaning"].toString();
        w.exampleDe = obj["example_de"].toString();
        w.exampleEn = obj["example_en"].toString();
        w.colorIndex = idx % VOCAB_COLOR_COUNT;
        w.known = m_knownWords.contains(w.word.toLower());
        w.recallNeeded = m_recallSentences.contains(w.exampleDe);
        if (!w.known && !w.word.isEmpty()) {
            m_words.append(w);
            ++idx;
        }
    }
    emit wordsChanged();
    return true;
}

bool VocabStore::saveToFile(const QString &path) const {
    QJsonArray arr;
    for (const VocabWord &w : m_words) {
        QJsonObject obj;
        obj["word"] = w.word;
        obj["type"] = w.type;
        obj["meaning"] = w.meaning;
        obj["example_de"] = w.exampleDe;
        obj["example_en"] = w.exampleEn;
        arr.append(obj);
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(arr).toJson());
    return true;
}

void VocabStore::addWord(const VocabWord &word) {
    if (m_knownWords.contains(word.word.toLower()))
        return;
    for (const VocabWord &w : m_words)
        if (w.word.toLower() == word.word.toLower())
            return;
    VocabWord w = word;
    w.colorIndex = m_words.size() % VOCAB_COLOR_COUNT;
    w.recallNeeded = m_recallSentences.contains(w.exampleDe);
    m_words.append(w);
    emit wordsChanged();
}

void VocabStore::markKnown(const QString &word) {
    m_knownWords.insert(word.toLower());
    m_words.erase(
        std::remove_if(m_words.begin(), m_words.end(),
                       [&](const VocabWord &w) {
                           return w.word.toLower() == word.toLower();
                       }),
        m_words.end());
    // re-assign color indices
    for (int i = 0; i < m_words.size(); ++i)
        m_words[i].colorIndex = i % VOCAB_COLOR_COUNT;
    emit wordsChanged();
}

void VocabStore::markRecallNeeded(const QString &exampleDe) {
    m_recallSentences.insert(exampleDe);
    for (VocabWord &w : m_words)
        if (w.exampleDe == exampleDe)
            w.recallNeeded = true;
    emit wordsChanged();
}

bool VocabStore::isKnown(const QString &word) const {
    return m_knownWords.contains(word.toLower());
}

void VocabStore::clear() {
    m_words.clear();
    emit wordsChanged();
}

bool VocabStore::wordMatchesSubtitle(const VocabWord &w,
                                     const QString &plain) const {
    QString base = w.baseForm();
    if (base.isEmpty())
        return false;
    QRegularExpression re("\\b" + QRegularExpression::escape(base) + "\\b",
                          QRegularExpression::CaseInsensitiveOption);
    return re.match(plain).hasMatch();
}

QVector<VocabWord> VocabStore::wordsForSubtitle(const QString &subtitleText) const {
    // Strip HTML tags
    QString plain = subtitleText;
    plain.remove(QRegularExpression("<[^>]*>"));

    QVector<VocabWord> result;
    for (const VocabWord &w : m_words)
        if (!w.known && wordMatchesSubtitle(w, plain))
            result.append(w);
    return result;
}

void VocabStore::setKnownWords(const QStringList &known) {
    for (const QString &w : known)
        m_knownWords.insert(w.toLower());
}

QStringList VocabStore::knownWordsList() const {
    return QStringList(m_knownWords.begin(), m_knownWords.end());
}

void VocabStore::setRecallSentences(const QStringList &sentences) {
    for (const QString &s : sentences)
        m_recallSentences.insert(s);
    for (VocabWord &w : m_words)
        if (m_recallSentences.contains(w.exampleDe))
            w.recallNeeded = true;
}

QStringList VocabStore::recallSentencesList() const {
    return QStringList(m_recallSentences.begin(), m_recallSentences.end());
}
