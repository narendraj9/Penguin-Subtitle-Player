#ifndef VOCABWORD_H
#define VOCABWORD_H

#include <QString>

struct VocabWord {
    QString word;       // e.g. "die Möglichkeit, -en"
    QString surface;    // exact word/phrase as it appears in the subtitle
    QString type;       // "f", "m", "n", "v", "adj", …
    QString meaning;    // English meaning
    QString exampleDe;  // German example sentence
    QString exampleEn;  // English example translation
    int colorIndex = 0; // 0-7 for palette
    bool known = false;
    bool recallNeeded = false;

    QString baseForm() const {
        QString w = word;
        int comma = w.indexOf(',');
        if (comma > 0)
            w = w.left(comma);
        if (w.startsWith("die ") || w.startsWith("der ") ||
            w.startsWith("das ") || w.startsWith("Die ") ||
            w.startsWith("Der ") || w.startsWith("Das "))
            w = w.mid(4);
        return w.trimmed();
    }

    QString matchText() const {
        QString s = surface.trimmed();
        return s.isEmpty() ? baseForm() : s;
    }
};

static const QString VOCAB_COLORS[] = {
    "#FF6B6B", // coral
    "#FFD93D", // yellow
    "#6BCB77", // green
    "#4D96FF", // blue
    "#FF9F1C", // orange
    "#C77DFF", // purple
    "#2EC4B6", // teal
    "#FF70A6"  // pink
};

static const int VOCAB_COLOR_COUNT = 8;

#endif // VOCABWORD_H
