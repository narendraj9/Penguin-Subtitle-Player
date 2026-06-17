#ifndef TESTVOCABSTORE_H
#define TESTVOCABSTORE_H

#include <QtTest>

class TestVocabStore : public QObject {
    Q_OBJECT
private slots:
    void testAddWord();
    void testMarkKnown();
    void testMarkRecallNeeded();
    void testWordsForSubtitle();
    void testLoadFromJson();
    void testSaveAndReload();
    void testDuplicateRejected();
    void testBaseForm();
    void testKnownWordsPersist();
};

#endif // TESTVOCABSTORE_H
