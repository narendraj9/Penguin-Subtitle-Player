#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>
#include <QKeyEvent>

class HelpDialog : public QDialog {
    Q_OBJECT
public:
    explicit HelpDialog(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;
};

#endif // HELPDIALOG_H
