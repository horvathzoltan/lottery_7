#include "shufflingdialog.h"
#include "ui_shufflingdialog.h"
#include "lottery.h"

#include <QFuture>
#include <QThread>
#include <QTimer>

ShufflingDialog::ShufflingDialog(QWidget *parent) :
                                                    QDialog(parent),
                                                    ui(new Ui::ShufflingDialog)
{
    ui->setupUi(this);
    ui->progressBar->setValue(0);
    this->setWindowTitle("1000 iteráció számítása");
    _result.isok=false;
    t= new QTimer(this);
    connect(t, SIGNAL(timeout()), this, SLOT(on_timeout()));
    t->start(200);
    _workerThread = new WorkerThread();
    _workerThread->p = &p;
    //connect(workerThread, &WorkerThread::resultReady, this, &ShufflingDialog::handleResults);

    connect(_workerThread, &WorkerThread::resultReady, this, &ShufflingDialog::handleResults);
    connect(_workerThread, &WorkerThread::finished, _workerThread, &QObject::deleteLater);
    _workerThread->start();
}

ShufflingDialog::~ShufflingDialog()
{
 //   delete _workerThread;
    delete t;
    delete ui;
}

void ShufflingDialog::closeEvent(QCloseEvent *)
{
    _workerThread->requestInterruption();
}

void ShufflingDialog::on_timeout()
{
    ui->label->setText("csinál..");
    ui->progressBar->setValue(p);
}

void ShufflingDialog::handleResults()
{
    _result = _workerThread->r;
    _result.isok=true;
    t->stop();
    ui->label->setText("kész");
    ui->progressBar->setValue(100);
    this->close();
}
