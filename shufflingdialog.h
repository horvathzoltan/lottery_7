#ifndef SHUFFLINGDIALOG_H
#define SHUFFLINGDIALOG_H

#include <QDialog>
#include <QThread>
#include "lottery.h"

namespace Ui {
class ShufflingDialog;
}

class ShufflingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShufflingDialog(QWidget *parent = nullptr);
    ~ShufflingDialog();
    const Lottery::ShuffleR& result(){return _result;}

    void closeEvent(QCloseEvent *) override;
private slots:
        void on_timeout();
        void handleResults();
private:
    Ui::ShufflingDialog *ui;
    QTimer *t;
    int p;
    Lottery::ShuffleR _result;
    class WorkerThread* _workerThread;
};

class WorkerThread : public QThread
{
    Q_OBJECT

public:
    int *p;
    int k=7;
    int max = Lottery::_settings.shuff_max;
    Lottery::ShuffleR r;
    void run() override
    {
        r = Lottery::Generate(p, k, max);
        emit resultReady();
    }
signals:
    void resultReady();
};

#endif // SHUFFLINGDIALOG_H
