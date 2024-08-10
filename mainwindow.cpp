#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "lottery.h"
#include "shufflingdialog.h"
#include "combinationdialog.h"
#include "../common/common/helpers/Downloader/downloader.h"
#include "../common/common/helpers/StringHelper/stringhelper.h"
#include <QtWidgets/QGraphicsView>

#include <QtCharts>
/*
9 - 35775-(88*300) = 9375
8 - 18550-(34*300) = 8350
7 - 7950-(10*300) = 4950
6 - 3975-(4*300) = 2775
*/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
{
    _isinited = false;
    ui->setupUi(this);

    Lottery::_settings.FromIni();
    uiCombinationsSpinBoxSetMinMax(Lottery::_settings.min, Lottery::_settings.max);
    uiCombinationsSpinBoxSetValue(Lottery::_settings.K);
    //ui->label_yearweek->setText(Lottery::_settings.yearweek());

    //Lottery::_settings.date = QDate::currentDate();

    //ui->label_date->setText(Lottery::_settings.date.toString("yyyy.mm.dd."));

    CreateTicket(); // a szelvény generálása
    ClearTicket();

    chart = new QChart(); // a grafikon generálása
    chart->setTitle("gyakoriság");
    chart->setAcceptHoverEvents(true);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setParent(ui->tab_2);
    chartView->setGeometry(ui->tab_2->geometry());

    auto a = Lottery::Refresh(-1, -1);
//    QDate date = QDate::fromString("2020-01-06", Qt::DateFormat::ISODate);
//    int y = date.year();
//    int w = date.weekNumber();
    //QDate date = QDate::fromString(QString::number(y)+"-01-01", Qt::DateFormat::ISODate).addDays((w-1)*7);
    //Lottery::_settings.setDate(date);
    //int year, week;
    //auto t_txt = Lottery::_settings.yearweek(&year, &week);

    //bool isok = last.year == year && last.week == week;


    uiWeekSpinBoxSetMinMax(1, Lottery::_data.count());    
    uiWeekSpinBoxSetValue(Lottery::_data.count());

    uiFilterSpinBoxSetMinMax(1, 10);
    uiFilterSpinBoxSetValue(Lottery::_settings.filter);

    setUi(a); // beállítja a MAX, MAY értékeket is

    _shuffled_series.setName("sorsolt");
    _shuffled_series.setColor(Qt::blue);
    _shuffled_series.setMarkerSize(7.0);
    _shuffled_series.append(QPointF(0, 0));
    _shuffled_series.append(QPointF(MAX, MAY));

    chart->addSeries(&_shuffled_series);

    _all_shuffled_series.setName("összes");
    _all_shuffled_series.setColor(Qt::green);
    _all_shuffled_series.setMarkerSize(7.0);
    _all_shuffled_series.append(QPointF(0, 0));
    _all_shuffled_series.append(QPointF(MAX, MAY));

    chart->addSeries(&_all_shuffled_series);

    connect(&_all_shuffled_series, &QLineSeries::hovered, this, &MainWindow::tooltip);

    RefreshByWeek();

    _isinited = true;
}

void MainWindow::ClearTicket(){
    static const int Q = Lottery::Settings::NUMBERS;
    static const int R = Lottery::Settings::TOTAL_NUMBERS/Lottery::Settings::NUMBERS;

    QPalette pal_w = palette();
    QPalette pal_g = palette();
    pal_w.setColor(QPalette::Window, Qt::white);
    pal_g.setColor(QPalette::Background, Qt::lightGray);
    QPalette *p;

    for(int i=0;i<Lottery::Settings::TOTAL_NUMBERS;i++){
        p = ((i/R)%2)?&pal_g:&pal_w;
        frames[i]->setPalette(*p);
    }
}

void MainWindow::CreateTicket(){
    static const int s = 48;

    static auto fi = ui->frame->fontInfo();
    static QFont font(fi.family(), s/3, fi.weight()*2, fi.italic());
    static QFont font2(fi.family(), s/5.00, fi.weight()/2, fi.italic());


    QPalette pal_blue;
    pal_blue.setColor(QPalette::WindowText, Qt::blue);

    for(int i=0;i<Lottery::Settings::TOTAL_NUMBERS;i++){
        int y = i/10;
        int x = i%10;

        QFrame *w = new QFrame(ui->frame);
        w->setGeometry(x*(s+8), y*(s+8), s, s);

        w->setFrameStyle(QFrame::Panel | QFrame::Plain);
        w->setLineWidth(1);

        QLabel *l = new QLabel(w);
        l->setFont(font);
        l->setText(QString::number(i+1));

        l->adjustSize();

        auto ax = (w->width()-l->width())/2;
        auto ay = (w->height()-l->height())/2;
        l->move(ax, ay);


        QLabel *l2 = new QLabel(w);
        l2->setFont(font2);
        l2->setPalette(pal_blue);
        l2->colorCount();

        l2->setGeometry(2, 0, 48-(4*2), 16);
        l2->setAlignment(Qt::AlignmentFlag::AlignRight);

        frames.append(w);
        labels.append(l);
        labels2.append(l2);

        w->setAutoFillBackground(true);
    }
}

MainWindow::~MainWindow()
{
    //for(auto i:frames){delete i;}
    delete ui;
    //labels[0]->setText("a");
}

// https://code.qt.io/cgit/qt/qtcharts.git/tree/examples/charts/callout?h=5.15
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
  /*
    m_coordX->setText(QString("X: %1").arg(chart->mapToValue(event->pos()).x()));
    m_coordY->setText(QString("Y: %1").arg(chart->mapToValue(event->pos()).y()));
*/
    //QGraphicsView::mouseMoveEvent(event);

}

void MainWindow::keepCallout(){
    m_callouts.append(m_tooltip);
    m_tooltip = new Callout(chart);
}

void MainWindow::tooltip(QPointF point, bool state){
    if (!m_tooltip) m_tooltip = new Callout(chart);

    if (state) {
        auto txt = "kombináció:\n"+QString::number(point.x()+.5)+"\n"+QString::number(point.y(),'f',0);
        m_tooltip->setText(txt);
        m_tooltip->setAnchor(point);
        m_tooltip->setZValue(11);
        m_tooltip->updateGeometry();
        m_tooltip->show();
    } else {
        m_tooltip->hide();
    }
}

void MainWindow::tooltip2(bool status, int index, QBarSet *barset){
    if (!m_tooltip) m_tooltip = new Callout(chart);

    if (status) {
        auto a = barset->at(index);
        auto point = QPointF(index, a);

        auto txt = "összes:\n"+QString::number(index+1)+"\n"+QString::number(a,'f',0);

        m_tooltip->setText(txt);
        m_tooltip->setAnchor(point);
        m_tooltip->setZValue(11);
        m_tooltip->updateGeometry();
        m_tooltip->show();
    } else {
        m_tooltip->hide();
    }
}
//https://bet.szerencsejatek.hu/cmsfiles/otos.csv
//
/*
 * Év;Hét;Húzásdátum;
 * 5 találat (db);5 találat (Ft);
 * 4 találat (db);4 találat (Ft);
 * 3 találat (db);3 találat (Ft);
 * 2 találat (db);2 találat (Ft);
 * Számok
 * */
//2020;46;2020.11.14.;0;0 Ft;41;1 533 055 Ft;3707;18 260 Ft;107291;1 645 Ft;8;13;30;61;68

// TODO ha a héten le lett töltve akkor nem kell megint leszedni
// ha a sorsolás már megvolt, akkor le lehet szedni, sorsolás előtt nem, ha már le lett töltve.
// ha még nem lett letöltve, bármikor le lehet tölteni

// TODO új fül: sonogramszerűen mutatni a húzásokat - jobra az újabb
// TODO http://www.lottoszamok.net/otoslotto/
// mutatni a következő sorsolást, a várható főnyereményt és a részvételi határidőt
void MainWindow::on_pushButton_download_clicked()
{
    auto ffn = Lottery::_settings.download_ffn();
    bool isok = com::helpers::Downloader::Wget(
        Lottery::_settings.url,
        ffn);
    if(!isok) return;
    //Lottery::_data.clear();
    auto a = Lottery::Refresh(-1, -1);
    setUi(a);
    auto b = Lottery::RefreshByWeek();
    setUi(b);
}

void MainWindow::setUi(const Lottery::RefreshR& m){
    if(!m.isOk) return;
    auto last = Lottery::_data.last();
    QString txt = last.datetime.toString();

   if(m.isExistInFile)
        txt += com::helpers::StringHelper::NewLine+last.num.ToString();

    this->ui->label_data->setText(txt);

//    txt = QString::number(Lottery::_data.size()) + com::helper::StringHelper::NewLine;
//    this->ui->label_w->setText(txt);

    //this->ui->label_date->setText(d.datetime.toString());

    MAX = Lottery::Settings::TOTAL_NUMBERS;
    MAY = m.max_y;

    //chart->series().clear();
    //chart->axes().detach();

    if(!chart->series().isEmpty()) for(auto&s:chart->series()) chart->removeSeries(s);
    for(auto& a:chart->axes()) chart->removeAxis(a);

    QBarSeries* barseries = new QBarSeries();
    //barseries->setBarWidth(1);
    QBarSet* set0 = new QBarSet("összes");
    barseries->append(set0);

    QScatterSeries *scatterseries = new QScatterSeries();
    scatterseries->setName("utolsó");
    scatterseries->setColor(Qt::red);
    scatterseries->setMarkerSize(7.0);

    scatterseries->append(QPointF(0, 0));
    scatterseries->append(QPointF(MAX, MAY));

    QScatterSeries *scatterseries2 = nullptr;

    if(Lottery::_next.num.number(1)){
    scatterseries2 = new QScatterSeries();
    scatterseries2->setName("következő");
    scatterseries2->setColor(Qt::yellow);
    scatterseries2->setMarkerSize(7.0);

    scatterseries2->append(QPointF(0, 0));
    scatterseries2->append(QPointF(MAX, MAY));
    }
    //set0->append(0);

    ClearTicket();

    QPalette pal = palette();
    pal.setColor(QPalette::Background, Qt::red);

    bool frames_isok = !frames.isEmpty();
    // az utolsót kirakni

    auto j=Lottery::_data.last().num;
    //for(auto&j:Lottery::_data.last().num)
    //{
       for(int i=0;i<Lottery::Settings::NUMBERS;i++){
           int n = j.number(i+1);
           qreal y = m.histogram[n-1];
           //qreal y = 0;
           qreal x = n-.5;
           scatterseries->append(QPointF(x, y));


           if(frames_isok && m.isExistInFile)
              frames[n-1]->setPalette(pal);


           if(scatterseries2){
               n = Lottery::_next.num.number(i+1);
               y = m.histogram[n-1];
               x = n-.5;
               scatterseries2->append(QPointF(x, y));
           }


        }
    //}


    QLineSeries *lineseries[Lottery::Settings::NUMBERS];
    for(int n=0;n<Lottery::Settings::NUMBERS;n++)
    {
        auto l = new QLineSeries();
        lineseries[n] = l;
        l->setName(QString::number(n+1));
        lineseries[0]->append(QPointF(0, 0));

        //auto s = new QBarSet(QString::number(n+1));
    }

    /*lineseries[0]->append(QPointF(0, 0));
    lineseries[1]->append(QPointF(0, 0));
    lineseries[2]->append(QPointF(0, 0));
    lineseries[3]->append(QPointF(0, 0));
    lineseries[4]->append(QPointF(0, 0));*/


    QStringList categories;

    categories.append(QString::number(0)); //1-90

    bool labels2_isok = !labels2.isEmpty();
    for(int i=0;i<MAX;i++){
        categories.append(QString::number(i+1)); //1-90

        auto x = m.histogram[i];
        auto a = QString::number(x);

        if(labels2_isok) labels2[i]->setText(a); // labelek a táblában

//        int R = 90/5;
//        int o = (i/R);

        set0->append(x); //bars

        for(int n=0;n<Lottery::Settings::NUMBERS;n++) // lines
        {
            qreal x = (qreal)(i)+.5;
            qreal y = m.histograms[n][i];
            lineseries[n]->append(QPointF(x, y));
        }

    }

    categories.append(QString::number(MAX+1));
    //set0->append(MAY);

    for(int i=0;i<Lottery::Settings::NUMBERS;i++){
        lineseries[i]->append(QPointF(MAX, MAY));
    }
    // lineseries[0]->append(QPointF(MAX, MAY));
    // lineseries[1]->append(QPointF(MAX, MAY));
    // lineseries[2]->append(QPointF(MAX, MAY));
    // lineseries[3]->append(QPointF(MAX, MAY));
    // lineseries[4]->append(QPointF(MAX, MAY));
    // lineseries[5]->append(QPointF(MAX, MAY));

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    //QValueAxis *axisX = new QValueAxis();

    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);

    for(int i=0;i<Lottery::Settings::NUMBERS;i++){
        lineseries[0]->attachAxis(axisX);
    }

    // lineseries[0]->attachAxis(axisX);
    // lineseries[1]->attachAxis(axisX);
    // lineseries[2]->attachAxis(axisX);
    // lineseries[3]->attachAxis(axisX);
    // lineseries[4]->attachAxis(axisX);
    // lineseries[5]->attachAxis(axisX);

    scatterseries->attachAxis(axisX);
    if(scatterseries2) scatterseries2->attachAxis(axisX);
    barseries->attachAxis(axisX);

    axisX->setRange(QString("1"), QString::number(MAX));
    //axisX->setRange(1, MAX);

    //axisX->setTickCount(18);
    //axisX->setMinorTickCount(1);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTickCount(10);
    axisY->setMax(MAY);
    axisY->setRange(0, MAY);
    //chartView->chart()->setAxisY(axisY, scatterseries);

    for(int i=0;i<Lottery::Settings::NUMBERS;i++){
        lineseries[0]->attachAxis(axisY);
    }

    // lineseries[0]->attachAxis(axisY);
    // lineseries[1]->attachAxis(axisY);
    // lineseries[2]->attachAxis(axisY);
    // lineseries[3]->attachAxis(axisY);
    // lineseries[4]->attachAxis(axisY);
    // lineseries[5]->attachAxis(axisY);

    scatterseries->attachAxis(axisY);
    if(scatterseries2)scatterseries2->attachAxis(axisY);
    barseries->attachAxis(axisY);

    QLinearGradient plotAreaGradient;
    plotAreaGradient.setStart(QPointF(0, 0));
    plotAreaGradient.setFinalStop(QPointF(1, 0));

    for(int i=0;i<Lottery::Settings::NUMBERS;i++){
        QRgb r = (i%2)?QRgb(0xc0c0c0):QRgb(0xefebe7);
        plotAreaGradient.setColorAt(((qreal)i+0.0)/Lottery::Settings::NUMBERS, r);
        plotAreaGradient.setColorAt(((qreal)i+.9999)/Lottery::Settings::NUMBERS, r);
    }
    // plotAreaGradient.setColorAt(0.0, QRgb(0xefebe7));
    // plotAreaGradient.setColorAt(.9999/6, QRgb(0xefebe7));

    // plotAreaGradient.setColorAt(1.0/6, QRgb(0xc0c0c0));
    // plotAreaGradient.setColorAt(1.9999/6, QRgb(0xc0c0c0));
    // plotAreaGradient.setColorAt(2.0/6, QRgb(0xefebe7));
    // plotAreaGradient.setColorAt(2.9999/6, QRgb(0xefebe7));
    // plotAreaGradient.setColorAt(3.0/6, QRgb(0xc0c0c0));
    // plotAreaGradient.setColorAt(3.9999/6, QRgb(0xc0c0c0));
    // plotAreaGradient.setColorAt(4.0/6, QRgb(0xefebe7));
    // plotAreaGradient.setColorAt(4.9999/6, QRgb(0xefebe7));
    // plotAreaGradient.setColorAt(5.0/6, QRgb(0xc0c0c0));

    plotAreaGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    chart->setPlotAreaBackgroundBrush(plotAreaGradient);
    chart->setPlotAreaBackgroundVisible(true);



    chart->addAxis(axisY, Qt::AlignLeft);

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignTop);

   //chart->createDefaultAxes();


    chart->addSeries(barseries);
    for(int n=0;n<Lottery::Settings::NUMBERS;n++)
    {
        chart->addSeries(lineseries[n]);
    }
    chart->addSeries(scatterseries);
    if(scatterseries2) chart->addSeries(scatterseries2);

    connect(barseries, &QBarSeries::hovered, this, &MainWindow::tooltip2);

}

//TODO egy kombináció hány számja egyezik meg az előzővel - legfeljebb mennyi egyezhet meg? 1.

// generate
void MainWindow::on_pushButton_generate_clicked()
{
    ShufflingDialog d;
    d.exec();
    auto a = d.result();
    if(a.isok) setUi(a);

    auto r = Lottery::RefreshByWeek();
    setUi(r);

}

void MainWindow::setUi(const Lottery::ShuffleR& m)
{
    CombinationDialog d;
    d.setUi(m);
    d.exec();

    _shuffled_series.clear();
    for(auto&i:m.num){
        qreal x = i.num-.5;
        auto a = QPointF(x, 1);
        _shuffled_series.append(a);
    }    
}

void MainWindow::resetUi(const Lottery::RefreshByWeekR& m){
    ui->listWidget->clear();
    ui->label_comb->setText("");
    ui->label_combnum->setText("");

}

void MainWindow::setUi(const Lottery::RefreshByWeekR& m){
    ui->listWidget->clear();
    QString txt = m.ToString();    

    qreal may=0;
    for(auto&i:m.num){if(i.hist>may) may=i.hist;}
    qreal r2 = may?MAY/may:0;

//    int r;
//    if(Lottery::_data.isEmpty()) r=1;
//    else r = (m.shuffnum/Lottery::_data.size())+5;
    if(Lottery::_data.isEmpty()) return;
    auto last = Lottery::_data.last();
    ui->label_comb->setText(txt);
    int year, week;
    auto t_txt = Lottery::_settings.yearweek(&year, &week);

    bool isok = last.year <= year && last.week <= week;

    int sum_prize=0;
    int sum_n=0;
    QString sum_curr;
    int h=0;
    for(auto&i:m.comb)
    {
        qreal q = GetFilNum(Lottery::_settings.filter);
        if(i.num.weight<q) continue;
        QString txt = '('+QString::number(i.num.weight, 'g', 2)+") ";
        txt+= i.num.ToString(Lottery::_next.num);

        if(isok){
            QString curr;
            int hn;
            auto v = Lottery::_next.prizeCur(i, &curr, &hn);
            if(v>0){
                txt += ' '+QString::number(hn+1)+"-s("+ QString::number(v) + ' ' + curr+')';
                sum_prize += v;
                if(sum_curr.isEmpty()) sum_curr = curr;
                sum_n++;
            }

        }        

        ui->listWidget->addItem(txt);
        h++;
    }


    _all_shuffled_series.clear();
    _all_shuffled_series.append(QPointF(0, 0));
    _all_shuffled_series.append(QPointF(MAX, MAY));
    chart->removeSeries(&_all_shuffled_series);
    QString ctxt;
    int o = 0;

    QList<QList<int>> e0;
    QList<int> e;
    for(auto&i:m.num){ // felsorolja a kombináció számait és felrakja a zöld pettyeket
        qreal x = i.num-.5;
        _all_shuffled_series.append(QPointF(x, r2?i.hist*r2:6));

        if(o>0 && !(o%Lottery::Settings::NUMBERS))
        {
            e0.append(e);
            e.clear();
        }

        e.append(i.num);
        o++;
    }
    if(!e.isEmpty()){
        e0.append(e);
    }

    QString a0;
    for(auto&a:e0){
        std::sort(a.begin(), a.end());
        QString b0;
        for(auto&b:a){
            if(!b0.isEmpty()) b0+=',';

            // maga a szám
            b0+=QString::number(b);

            // ha kihúzták, megcsillagozzuk
            if(Lottery::_next.num.number(1)){//vannak számok
                 bool o3 = false;
                 for(int j=1;j<=Lottery::Settings::NUMBERS;j++)
                     if(Lottery::_next.num.number(j)==b)
                     {
                         o3 = true;
                         break;
                     }
                 if(o3) ctxt+='*';
            }

        }
        if(!a0.isEmpty()) a0+=com::helpers::StringHelper::NewLine;
        a0+=b0;
    }

    ctxt+= a0;

    auto sum_ticket_price = m.comb.length()*Lottery::_settings.ticket_price;
    if(sum_prize>0){ // tudjuk a nyereményt
        ctxt += '\n'+ QString::number(sum_prize)+'/' +QString::number(sum_ticket_price)+' ' + sum_curr;
        ctxt += '\n'+ QString::number(sum_prize-sum_ticket_price)+' ' + sum_curr;
        ctxt += '\n'+ QString::number(sum_n)+'/'+QString::number(m.comb.length())+' '+"db";
        ctxt += " arány:"+ QString::number(sum_n/(qreal)m.comb.length());
    }
    else{ //  még csak a szelvények árát tudjuk
        ctxt += '\n'+ QString::number(m.comb.length())+' '+"db";
        ctxt += ' '+ QString::number(sum_ticket_price)+' ' + Lottery::_settings.ticket_curr;
    }
    if(m.mweight>0)
    {
        ctxt +="\nmaxw: "+QString::number(m.mweight)+" "+QString::number(m.mweights.count())+"db";

    }
    if(Lottery::_next.num.number(1)){
        ctxt+="\nnext: "+Lottery::_next.num.ToString();
    }

    // milyen húzások értek el valamilyen találatot?
    //if(!m.besthits.isEmpty()){
        int j=0;
        for(auto&i:m.besthits){
            j++;
            if(!i.isEmpty()){
                QString ntxt =QString::number(j)+"-s";
                ctxt+='\n'+ntxt+' '+QString::number(i.count())+"db";
                ui->listWidget->addItem(ntxt);
                //i.
                for(auto&k:i){
                    qreal q = GetFilNum(Lottery::_settings.filter);
                    if(k.numbers.weight<q) continue;

                    QString mtxt = QString::number(k.ix)+". "+
                        '('+QString::number(k.numbers.weight, 'g', 2)+") "
                        +k.numbers.ToString(Lottery::_next.num);
                    ui->listWidget->addItem(mtxt);
                }
            }
        }
        //ctxt+='\n'+QString::number(m.besthit)+"-s "+QString::number(m.hitcnt)+"db";
    //}

    ui->label_combnum->setText(ctxt);
    chart->addSeries(&_all_shuffled_series);
}

//delete
void MainWindow::on_pushButton_2_clicked()
{
    chart->removeSeries(&_shuffled_series);
    _shuffled_series.clear();
    _shuffled_series.append(QPointF(0, 0));
    _shuffled_series.append(QPointF(MAX, MAY));
    //Lottery::_shuffled.clear();

}

void MainWindow::RefreshByWeek(){
    auto r = Lottery::RefreshByWeek(); // ez betölti
    if(r.isok)
        setUi(r);
    else
        resetUi(r);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Lottery::_settings.ToIni();
}

// clipboard

void MainWindow::on_pushButton_clipbrd_clicked()
{
    QClipboard *c = QApplication::clipboard();
    QString e;
    e = ui->label_combnum->text();

    for(int i=0;i<ui->listWidget->count();i++)
    {
        auto txt = ui->listWidget->item(i)->text();

        if(!e.isEmpty()) e+='\n';
        e+= txt;
    }
    if(!e.isEmpty()) c->setText(e);
}

// cminus cplus

void MainWindow::on_pushButton_cminux_clicked()
{
    int v = ui->labelc->property("value").toInt();
    int min = ui->labelc->property("min").toUInt();
    if(v<=min) return; // elértük
    uiCombinationsSpinBoxSetValue(--v);
    on_spinBox_valueChanged(v);
}

void MainWindow::on_pushButton_cplus_clicked()
{
    int v = ui->labelc->property("value").toInt();
    int max = ui->labelc->property("max").toUInt();
    if(v>=max) return; // elértük
    uiCombinationsSpinBoxSetValue(++v);
    on_spinBox_valueChanged(v);
}

void MainWindow::on_spinBox_valueChanged(int arg1)
{
    if(!_isinited) return;
    static bool lock = false;
    if(lock) return;
    lock=true;
    Lottery::_settings.K = arg1;
    RefreshByWeek();
    lock=false;
}

void MainWindow::uiCombinationsSpinBoxSetValue(int i){
    ui->labelc->setProperty("value", i);
    ui->labelc->setText(QString::number(i));
    //Lottery::_settings.K;
}

void MainWindow::uiCombinationsSpinBoxSetMinMax(int min, int max){
    ui->labelc->setProperty("min", min);
    ui->labelc->setProperty("max", max);
}



// wminus wplus // homemade spinbox

void MainWindow::uiWeekSpinBoxSetValue(int i){
    //ui->label_w->setProperty("value", i);
    QString txt = Lottery::_settings.yearweek();
    ui->label_w->setText(txt);

}

void MainWindow::uiWeekSpinBoxSetMinMax(int min, int max){
    ui->label_w->setProperty("min", min);
    ui->label_w->setProperty("max", max);
}

void MainWindow::on_pushButton_wminus_clicked()
{
    //int v = ui->label_w->property("value").toInt();
    int min = ui->label_w->property("min").toUInt();
    //if(v<=min) return; // elértük
    Lottery::_settings.datemm();
    uiWeekSpinBoxSetValue(1);
    on_week_valueChanged(1);
}

void MainWindow::on_pushButton_wplus_clicked()
{
    //int v = ui->label_w->property("value").toInt();
    int max = ui->label_w->property("max").toUInt();
    //if(v>=max) return; // elértük
    Lottery::_settings.datepp();
    uiWeekSpinBoxSetValue(1);
    on_week_valueChanged(1);

}

void MainWindow::on_week_valueChanged(int arg1)
{
    if(!_isinited) return;
    static bool lock = false;
    if(lock) return;
    lock=true;
    ClearTicket();
    //Lottery::_data.clear();
    int y, w;
    Lottery::_settings.yearweek(&y,&w);
    auto a = Lottery::Refresh(y, w);
    setUi(a);
    //auto l = Lottery::_data.last();
    //QDate date = QDate::fromString(QString::number(l.year)+"-01-01", Qt::DateFormat::ISODate).addDays(l.week*7);

    //Lottery::_settings.setDate(date);


    RefreshByWeek();

    //ui->label_yearweek->setText(Lottery::_settings.yearweek());
    lock=false;
}

// fminus fplus

void MainWindow::uiFilterSpinBoxSetValue(int i){
    ui->label_f->setProperty("value", i);
    qreal q = GetFilNum(i);
    ui->label_f->setText(QString::number(q));
}

qreal MainWindow::GetFilNum(int i){
    return 0.1*i;
}

void MainWindow::uiFilterSpinBoxSetMinMax(int min, int max){
    ui->label_f->setProperty("min", min);
    ui->label_f->setProperty("max", max);
}


void MainWindow::on_pushButton_fminus_clicked()
{
    int v = ui->label_f->property("value").toInt();
    int min = ui->label_f->property("min").toUInt();
    if(v<=min) return; // elértük
    uiFilterSpinBoxSetValue(--v);
    on_filter_valueChanged(v);
}

void MainWindow::on_pushButton_3_clicked()
{
    int v = ui->label_f->property("value").toInt();
    int max = ui->label_f->property("max").toUInt();
    if(v>=max) return; // elértük
    uiFilterSpinBoxSetValue(++v);
    on_filter_valueChanged(v);
}

void MainWindow::on_filter_valueChanged(int arg1)
{
    if(!_isinited) return;
    static bool lock = false;
    if(lock) return;
    lock=true;
    Lottery::_settings.filter = arg1;
    RefreshByWeek();
    lock=false;
}

void MainWindow::on_pushButton_combination_clicked()
{
    auto txt = ui->lineEdit_combination->text().replace(' ', ',');

    auto a = txt.split(',');
    if(a.length()<Lottery::Settings::NUMBERS) return;
    QVector<Lottery::Data> dl;
    Lottery::Data d;
    bool isok;
    for(int i=0;i<Lottery::Settings::NUMBERS;i++) d.num.setNumber(i+1, a[i].toInt(&isok));
    dl.append(d);

    Lottery::Weight(&dl);

    auto* n = &dl[0].num;

    txt= n->ToString()+' '+QString::number(n->weight, 'g', 2);
    ui->lineEdit_combination->setText(txt);

}
