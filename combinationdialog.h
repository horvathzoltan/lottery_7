#ifndef COMBINATIONDIALOG_H
#define COMBINATIONDIALOG_H

#include "lottery.h"

#include <QDialog>

namespace Ui {
class CombinationDialog;
}

class CombinationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CombinationDialog(QWidget *parent = nullptr);
    ~CombinationDialog();

    void setUi(const Lottery::ShuffleR& m);
private:
    Ui::CombinationDialog *ui;
};

#endif // COMBINATIONDIALOG_H
