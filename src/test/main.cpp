#include "tests.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Tests::run();
    return 0;
}
