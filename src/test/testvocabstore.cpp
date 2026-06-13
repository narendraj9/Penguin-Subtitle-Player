#include "testvocabstore.h"
#include "src/vocabstore.h"
#include "src/vocabword.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryFile>

static VocabWord makeWord(const QString &word, const QString &meaning,
                          const QString &exampleDe = "") {
    VocabWord w;
    w.word = word;
    w.type = "f";
    w.meaning = meaning;
    w.exampleDe = exampleDe;
    w.exampleEn = "example";
    return w;
}

void TestVocabStore::testAddWord() {
    VocabStore store;
    QCOMPARE(store.count(), 0);
    store.addWord(makeWord("die Möglichkeit, -en", "possibility"));
    QCOMPARE(store.count(), 1);
    QCOMPARE(store.words().first().word, "die Möglichkeit, -en");
    QCOMPARE(store.words().first().colorIndex, 0);
}

void TestVocabStore::testMarkKnown() {
    VocabStore store;
    store.addWord(makeWord("die Möglichkeit, -en", "possibility"));
    store.addWord(makeWord("die Chance, -n", "chance"));
    QCOMPARE(store.count(), 2);

    store.markKnown("die Möglichkeit, -en");
    QCOMPARE(store.count(), 1);
    QCOMPARE(store.words().first().word, "die Chance, -n");
    QVERIFY(store.isKnown("die Möglichkeit, -en"));
    QVERIFY(!store.isKnown("die Chance, -n"));
}

void TestVocabStore::testMarkRecallNeeded() {
    VocabStore store;
    VocabWord w = makeWord("die Möglichkeit, -en", "possibility",
                           "Das ist eine große Möglichkeit.");
    store.addWord(w);
    QVERIFY(!store.words().first().recallNeeded);
    store.markRecallNeeded("Das ist eine große Möglichkeit.");
    QVERIFY(store.words().first().recallNeeded);
}

void TestVocabStore::testWordsForSubtitle() {
    VocabStore store;
    store.addWord(makeWord("die Möglichkeit, -en", "possibility"));
    store.addWord(makeWord("der Tag, -e", "day"));

    QVector<VocabWord> matches =
        store.wordsForSubtitle("Das ist eine große Möglichkeit.");
    QCOMPARE(matches.size(), 1);
    QCOMPARE(matches.first().word, "die Möglichkeit, -en");

    QVector<VocabWord> dayMatches =
        store.wordsForSubtitle("Heute ist ein schöner Tag.");
    QCOMPARE(dayMatches.size(), 1);
    QCOMPARE(dayMatches.first().word, "der Tag, -e");

    QVector<VocabWord> noMatch =
        store.wordsForSubtitle("Hallo Welt!");
    QCOMPARE(noMatch.size(), 0);
}

void TestVocabStore::testLoadFromJson() {
    QTemporaryFile tmp;
    tmp.setAutoRemove(true);
    QVERIFY(tmp.open());

    QJsonArray arr;
    QJsonObject obj;
    obj["word"] = "die Möglichkeit, -en";
    obj["type"] = "f";
    obj["meaning"] = "possibility";
    obj["example_de"] = "Das ist eine Möglichkeit.";
    obj["example_en"] = "That is a possibility.";
    arr.append(obj);

    QJsonObject obj2;
    obj2["word"] = "der Erfolg, -e";
    obj2["type"] = "m";
    obj2["meaning"] = "success";
    obj2["example_de"] = "Das ist ein großer Erfolg.";
    obj2["example_en"] = "That is a great success.";
    arr.append(obj2);

    tmp.write(QJsonDocument(arr).toJson());
    tmp.close();

    VocabStore store;
    QVERIFY(store.loadFromFile(tmp.fileName()));
    QCOMPARE(store.count(), 2);
    QCOMPARE(store.words()[0].word, "die Möglichkeit, -en");
    QCOMPARE(store.words()[1].word, "der Erfolg, -e");
    QCOMPARE(store.words()[0].colorIndex, 0);
    QCOMPARE(store.words()[1].colorIndex, 1);
}

void TestVocabStore::testSaveAndReload() {
    QTemporaryFile tmp;
    tmp.setAutoRemove(true);
    QVERIFY(tmp.open());
    tmp.close();

    VocabStore store;
    store.addWord(makeWord("die Freude, -n", "joy", "Das ist pure Freude."));
    QVERIFY(store.saveToFile(tmp.fileName()));

    VocabStore store2;
    QVERIFY(store2.loadFromFile(tmp.fileName()));
    QCOMPARE(store2.count(), 1);
    QCOMPARE(store2.words().first().word, "die Freude, -n");
    QCOMPARE(store2.words().first().exampleDe, "Das ist pure Freude.");
}

void TestVocabStore::testDuplicateRejected() {
    VocabStore store;
    store.addWord(makeWord("die Möglichkeit, -en", "possibility"));
    store.addWord(makeWord("die Möglichkeit, -en", "chance"));
    QCOMPARE(store.count(), 1);
}

void TestVocabStore::testBaseForm() {
    VocabWord w1;
    w1.word = "die Möglichkeit, -en";
    QCOMPARE(w1.baseForm(), "Möglichkeit");

    VocabWord w2;
    w2.word = "der Tag, -e";
    QCOMPARE(w2.baseForm(), "Tag");

    VocabWord w3;
    w3.word = "das Haus, -er";
    QCOMPARE(w3.baseForm(), "Haus");

    VocabWord w4;
    w4.word = "gehen";
    QCOMPARE(w4.baseForm(), "gehen");
}

void TestVocabStore::testKnownWordsPersist() {
    VocabStore store;
    store.addWord(makeWord("die Möglichkeit, -en", "possibility"));
    store.addWord(makeWord("die Chance, -n", "chance"));
    store.markKnown("die Möglichkeit, -en");

    QStringList known = store.knownWordsList();
    QVERIFY(known.contains("die möglichkeit, -en"));

    VocabStore store2;
    store2.setKnownWords(known);
    store2.addWord(makeWord("die Möglichkeit, -en", "possibility"));
    QCOMPARE(store2.count(), 0); // should be rejected as known
}
