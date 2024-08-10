#include "lottery.h"
#include "../common/common/helpers/TextFileHelper/textfilehelper.h"
#include "../common/common/helpers/StringHelper/stringhelper.h"
#include "../common/common/helpers/Downloader/downloader.h"
#include "../common/common/helpers/TextFileHelper/textfilehelper.h"
#include <QDir>
#include <QSet>
#include <QThread>
#include <random>

Lottery::Lottery()
{

}

//const int Lottery::Settings::NUMBERS = 6;


Lottery::Settings Lottery::_settings;
QVector<Lottery::Data> Lottery::_data;
Lottery::Data Lottery::_next;

QFileInfoList Lottery::DataFileInfoListByWeek(){
    QString data_ffn = Lottery::_settings.data_ffn("");
    QDir dir(data_ffn);
    auto fl = dir.entryInfoList(QDir::Filter::Files);
    return fl;
}

QFileInfoList Lottery::ExclusionByWeek(){
    auto fil = DataFileInfoListByWeek();
    QString ex_fn = Lottery::_settings.data_ffn("excl.csv");

    com::helpers::FileErrors err;
    auto ex_txt = com::helpers::TextFileHelper::Load(ex_fn, &err);
    auto ex_lines = com::helpers::StringHelper::toStringList(ex_txt);

    QFileInfoList e;

    for(auto&i:fil) if(!ex_lines.contains(i.fileName())) e.append(i);

    return e;
}

// az aktuális héthez tartozó generált adatokat tölti be és frissíti
Lottery::RefreshByWeekR Lottery::RefreshByWeek(){
    Lottery::RefreshByWeekR nullelem{0, {},{},false, {}, {}, 0};
//    QString data_ffn = Lottery::_settings.data_ffn("");
//    QDir dir(data_ffn);
//    auto fl = dir.entryInfoList(QDir::Filter::Files);
    auto fl = DataFileInfoListByWeek();
    if(fl.isEmpty()) return nullelem;
    QVector<Lottery::Data> fd;
    for(auto&i:fl){
        if(!i.fileName().endsWith(".csv")) continue;
        auto fn = i.absoluteFilePath();

        com::helpers::FileErrors err;
        auto txt = com::helpers::TextFileHelper::Load(fn, &err);
        auto lines = com::helpers::StringHelper::toStringList(txt);
        for(auto&line:lines){
            auto l = line.split(",");
            if(l.count()<_settings.NUMBERS) continue;
            Lottery::Data d0;
            bool isok;
            for(int k=0;k<_settings.NUMBERS;k++) d0.num.setNumber(k+1, l[k].toInt(&isok));
            fd.append(d0);
        }
    }
    if(fd.isEmpty()) return nullelem;

    Weight(&fd);
    int maxweight;
    QVector<Numbers> n = FindByMaxWeight(fd, &maxweight);

    QVector<QVector<BestHit>> besthits;    

    if(!Lottery::_data.isEmpty())
    {
        if(_next.num.number(1))
            besthits = FindBestHit(fd, Lottery::_next.num);
    }

    auto r = Lottery::Generate2(fd);
    return {fd.count(), r.num, r.comb, r.isok, besthits, n, maxweight};
}

QVector<Lottery::Numbers> Lottery::FindByMaxWeight(const QVector<Lottery::Data>& fd, int* maxweight){
    int mw =0; for(auto&i:fd)if(i.num.weight>mw) mw = i.num.weight;
    if(maxweight)*maxweight=mw;
    QVector<Numbers> n;
    if(mw>0) for(auto&i:fd)if(i.num.weight==mw) n.append(i.num);
    return n;
}

QVector<QVector<Lottery::BestHit>> Lottery::FindBestHit(const QVector<Lottery::Data>& fd,const Numbers& numbers){
    QVector<QVector<BestHit>> e(_settings.NUMBERS);
    int h, ix=0;
    Lottery::BestHit bhit;

    auto wh = WeightsByParity();
    auto wp = WeightsByPentilis();

    for(auto&i:fd){
        ix++;
        h = 0;
        for(int j=0;j<_settings.NUMBERS;j++){
            int n = i.num.number(j+1);
            bhit.ix = ix;
            //bhit.numbers = i.num;
            bhit.numbers.setNumber(j+1,n);
            for(int k=0;k<_settings.NUMBERS;k++){
                if(n==numbers.number(k+1)) h++;
            }
        }

        if(h>1){
            bool o=false;
            for(auto&i:e[h-1]) if(i.numbers==bhit.numbers) {o=true;break;}

            bhit.numbers.WeightByParity(wh);
            bhit.numbers.WeightByPentilis(wp);

            if(!o){
                bhit.numbers.sort();
                e[h-1].append(bhit);                                
            }
        }
    }
    return e;
}

bool Lottery::FromFile(const QString& txt, int year, int week, bool* isExist){
    if(!isExist) return false;
    //auto txt = com::helper::TextFileHelper::load(fp);
    auto lines = com::helpers::StringHelper::toStringList(txt);
    _data.clear();
    *isExist = false;
    //auto size_orioginal = _data.size();

    static const int year_ix = 0;
    static const int week_ix = 1;
    static const int datetime_ix = 2;

    static const int hit7_ix = 3;
    static const int hit6_ix = 5;
    static const int hit5_ix = 7;
    static const int hit4_ix = 9;
   // static const int hit3_ix = 11;
  //  static const int hit2_ix = 13;

    static const int hit_len = 2;
    static const int numbers_ix = 11;
    static const int numbers_ix_2 = 18;

    for(int i=0;i<_settings.NUMBERS;i++){_next.num.setNumber(i+1, 0);}

    // elől van a legfrissebb
    //auto drop = lines.count()-maxline;
    //int linecount = 0;
    for(auto l: lines){
        if(l.isEmpty()) continue;
        if(l.startsWith('#')) continue;
        auto a = CsvSplit(l);
        if(a.length()<24) continue;
        //linecount++;
        //if(maxline>0 && linecount<drop) continue;

        //if(maxline>0 && linecount++<drop) continue;
        Data d;
        bool isok;

        d.year = a[year_ix].toInt(&isok);
        d.week = a[week_ix].toInt(&isok);

        d.datetime = QDate::fromString(a[datetime_ix], "yyyy.MM.dd.");

        d.setHit(7, Hit::FromCsv(a.mid(hit7_ix, hit_len), "7"));
        d.setHit(6, Hit::FromCsv(a.mid(hit6_ix, hit_len), "6"));
        d.setHit(5, Hit::FromCsv(a.mid(hit5_ix, hit_len), "5"));
        d.setHit(4, Hit::FromCsv(a.mid(hit4_ix, hit_len), "4"));
        d.setHit(3, {0, 0, "","3"});
        d.setHit(2, {0, 0, "","2"});
        d.setHit(1, {0, 0, "","1"});

        //gépi
        d.num.setNumber(1, a[numbers_ix].toInt(&isok));
        d.num.setNumber(2, a[numbers_ix+1].toInt(&isok));
        d.num.setNumber(3, a[numbers_ix+2].toInt(&isok));
        d.num.setNumber(4, a[numbers_ix+3].toInt(&isok));
        d.num.setNumber(5, a[numbers_ix+4].toInt(&isok));
        d.num.setNumber(6, a[numbers_ix+5].toInt(&isok));
        d.num.setNumber(7, a[numbers_ix+6].toInt(&isok));

        Data d1(d);
        //kézi
        d1.num.setNumber(1, a[numbers_ix_2].toInt(&isok));
        d1.num.setNumber(2, a[numbers_ix_2+1].toInt(&isok));
        d1.num.setNumber(3, a[numbers_ix_2+2].toInt(&isok));
        d1.num.setNumber(4, a[numbers_ix_2+3].toInt(&isok));
        d1.num.setNumber(5, a[numbers_ix_2+4].toInt(&isok));
        d1.num.setNumber(6, a[numbers_ix_2+5].toInt(&isok));
        d1.num.setNumber(7, a[numbers_ix_2+6].toInt(&isok));

        if(!(*isExist) && Lottery::Settings::isDateEquals(d.year,d.week, year, week))
            *isExist=true;
        if(year>-1){
            if(Lottery::Settings::isAfterOrThis(d.year,d.week, year, week))
            {
                _data.append(d);
                _data.append(d1);
            }
            else{
                _next = d;
            }
        }
        else{
            _data.append(d);
            _data.append(d1);
        }

        //kézi




    }
    return _data.size();
}

QStringList Lottery::CsvSplit(const QString &s)
{
    return s.trimmed().split(';');
}

Lottery::Hit Lottery::Hit::FromCsv(const QStringList& lines, const QString &desc)
{
    bool isok;
    int count = lines[0].toInt(&isok);
    int prize = 0;
    QString c;
    QRegularExpression re(R"(([\d\s]+)([a-zA-Z]+))");
    QRegularExpressionMatch match = re.match(lines[1]);
    if(match.hasMatch()){
        QString p = match.captured(1).simplified().remove(' ');
        prize = p.toInt(&isok);
        c = match.captured(2);
    }

    Hit h {count, prize, c, desc};//.toInt(&isok)
    return h;
}

// az adatfájlt tölti be
Lottery::RefreshR Lottery::Refresh(int year, int week){
    ////https://bet.szerencsejatek.hu/cmsfiles/otos.csv
    //com::helper::Downloader d;
    static Lottery::RefreshR nullobj{false, {0}, {{0},{0},{0},{0},{0},{0},{0}},  0, 0};
//    bool isok = com::helper::Downloader::Wget(
//        "https://bet.szerencsejatek.hu/cmsfiles/otos.csv",
//        Lottery::_settings.path);
//    if(!isok) return nullobj;
    auto ffn = Lottery::_settings.download_ffn();

    com::helpers::FileErrors err;
    auto txt = com::helpers::TextFileHelper::Load(ffn, &err);
    bool isExistInFile;
    bool isok = Lottery::FromFile(txt, year, week, &isExistInFile);
    if(!isok) return nullobj;

    Lottery::RefreshR r;
    r.isExistInFile = isExistInFile;
    r.isOk = true;
    std::sort(_data.begin(), _data.end(), Data::AscByDate);

    //auto i_min = _data.begin();
    //auto i_max = _data.end();

    r.histogram = Lottery::Histogram(_data, 0);
    for(int n=0;n<_settings.NUMBERS;n++)
        r.histograms[n] = Lottery::Histogram(_data, n+1);

//    auto last = _data.last().num;
//    r.last.append(last);

    r.min_y = *std::min_element(r.histogram.begin(), r.histogram.end());
    r.max_y = *std::max_element(r.histogram.begin(), r.histogram.end());

    return r;
}

QVector<QVector<int>> Lottery::SelectByCombination(const QVector<Occurence>&p, int k){
    QVector<QVector<int>> e;
    auto n = p.count();
    if(n<1||k<1||n<k) return e;

    auto c = Combination(n, k);

    for(auto&i:c){
        QVector<int> f;
        for(auto&j:i){
            f.append(p[j].num);
        }

        e.append(f);
    }

    return e;
}


QVector<Lottery::Data> Lottery::ToData(QVector<QVector<int>>& p){
    QVector<Data> e;
    if(p.isEmpty()) return e;

    for(auto&i:p){
        Data d0;
        if(i.count()<_settings.NUMBERS) return e;
        for(int j=0;j<_settings.NUMBERS;j++) d0.num.setNumber(j+1, i[j]);
        e.append(d0);
    }
    return e;
}

//http://rosettacode.org/wiki/Combinations#C.2B.2B
QVector<QVector<int>> Lottery::Combination(int N, int K)
{
    std::string bitmask(K, 1); // K leading 1's
    bitmask.resize(N, 0); // N-K trailing 0's
    QVector<QVector<int>> e;

    // print integers and permute bitmask
    do {
        QVector<int> f;
        for (int i = 0; i < N; ++i) // [0..N-1] integers
        {
            if (bitmask[i]) f.append(i);//std::cout << " " << i;
        }
        e.append(f);
        //std::cout << std::endl;
    } while (std::prev_permutation(bitmask.begin(), bitmask.end()));
    return e;
}

Lottery::ShuffleR Lottery::Generate(int *p, int k, int max){

    auto shuffled = Lottery::Shuffle(p, max); //sorsolás - 1000 db
    auto r = Generate2(shuffled);

    return r;
}

Lottery::ShuffleR Lottery::Generate2(QVector<Lottery::Data>& d){
    Lottery::ShuffleR r;
    int K = Lottery::_settings.K;
    r.num = Lottery::SelectByOccurence(d, K); // vesszük k db leggyakoribbat

    if(r.num.count()>_settings.NUMBERS){
        auto a = Lottery::SelectByCombination(r.num, _settings.NUMBERS); // permutáljuk
        r.comb = Lottery::ToData(a);
        Weight(&r.comb);
        r.isok = true;
    }
    else if(r.num.count()==_settings.NUMBERS){
        Lottery::Data d0;
        for(int j=1;j<=_settings.NUMBERS;j++) d0.num.setNumber(j, r.num[j-1].num);
        r.comb.append(d0);
        Weight(&r.comb);
        r.isok = true;
    }

    return r;
}


QVector<Lottery::Data> Lottery::Shuffle(int* ptr, int max){
    QVector<Data> d;

    if(max<Lottery::_settings.c_min) max=Lottery::_settings.c_min; else if(max>Lottery::_settings.c_max) max = Lottery::_settings.c_max;

    auto histogram = Lottery::Histogram(_data, 0);

    QVector<int> num2;
    for(int i=0;i<Lottery::Settings::TOTAL_NUMBERS;i++)
    {
        int n = histogram[i]; // i a szám, n a daramszáma
        for(int k=0;k<n;k++)
        {
            num2.append(i+1);
        }
    }

    int t=0;
    do {
        if(ptr)*ptr=t/(max/100);
        auto num = num2;
        Data d0;
        QVector<int> m0(_settings.NUMBERS);
        for(int j=1;j<=_settings.NUMBERS;j++)
        {
            std::random_shuffle(num.begin(), num.end());
            int max = num.count();
            auto ix = rand() % max;
            int n = num[ix];
            d0.num.setNumber(j, n);
            //m0[j-1]=n;
            num.removeAll(n);
        }

        //if(!d0.TestAll()) continue;

        if(QThread::currentThread()->isInterruptionRequested()) return d;

//        std::sort(m0.begin(), m0.end());
//        for(int j=1;j<=5;j++) d0.setNumber(j, m0[j-1]);

        //d0.NumbersSort();
        d.append(d0);
        t++;
    } while(t<max);

    //auto a = Lottery::Refresh(-1);
    Lottery::Save(d);

    return d;
}

// páros súly
// elméleti: 5 db páros:87, 4...:477, 3:1000, 2:1000, 1:477, 0-azaz nincs páros:87 /3128 húzás

void Lottery::Weight(QVector<Lottery::Data>* d)
{
    WeightClear(d);
    WeightByParity(d);
    WeightByPentilis(d);
    WeightByPrev(d, 1);
    WeightByPrev(d, 2);
    WeightByPrev(d, 3);
    WeightByPrev(d, 4);
    WeightByPrev(d, 5);
    WeightByPrev(d, 6);
    WeightByPrev(d, 7);
    WeightByPrev(d, 8);
    WeightByPrev(d, 9);
    WeightByPrev(d, 10);
    WeightByPrev(d, 11);
    WeightByPrev(d, 12);
}

void Lottery::WeightClear(QVector<Data>* d){
    for(auto&i:*d) i.num.weight = 1;
}

void Lottery::WeightByParity(QVector<Data>* d){  
    auto wp = WeightsByParity();

    for(int i=0;i<d->length();i++) (*d)[i].num.WeightByParity(wp);
}

// az összes húzásból a paritás szerinti súly
// 6 db súly: 5 párostól a 0 párosig
QVector<qreal> Lottery::WeightsByParity(){
    QVector<qreal> w(Lottery::Settings::NUMBERS+1); for(auto&i:w)i=0;

    for(auto&i:_data) w[i.num.NumbersEven()]++;
    auto m =0; for(auto&i:w)if(i>m)m=i;
    for(auto&i:w)i/=m;
    return w;
}

// az összes húzásból a pentilis szerinti súly
// 6 db súly,
QVector<qreal> Lottery::WeightsByPentilis(){
    QVector<qreal> w(Lottery::Settings::NUMBERS+1); for(auto&i:w)i=0;

    for (Data &i : _data){
        w[i.num.NumbersPentilis()]++;
    }
    auto m =0; for(auto&i:w)if(i>m)m=i;
    for(auto&i:w)i/=m;
    return w;
}

// az előzővel való egyezés esélye
QVector<qreal> Lottery::WeightsByPrev(int prev_n){
    QVector<qreal> w(Lottery::Settings::NUMBERS+1); for(auto&i:w)i=0;

    // az előző n húzást figyelembe véve ennyiszer ismétlődött
    for(int i=prev_n;i<_data.count();i++)// a 0-nak nincs prev-je
    {
        auto prev = _data[i-prev_n].num;
        auto p1=(_data[i]).num.HitNum(prev);
        w[p1]++;
    }
    //for(auto&i:_data) w[i.num.NumbersPrev1()]++;
    auto m =0;
    for(auto&i:w)if(i>m)m=i;
    for(auto&i:w)i/=m;
    return w;
}


// pentilis súly
// 1:3, 2:256, 3:1464, 4:1270, 5:135

void Lottery::WeightByPentilis(QVector<Data>* d){
    auto wp = WeightsByPentilis(); //kiszámolja a súlyt

    for(int i=0;i<d->length();i++) (*d)[i].num.WeightByPentilis(wp); // szoroz a találat szerint
}

void Lottery::WeightByPrev(QVector<Data>* d, int pre_n){
    auto wp = WeightsByPrev(pre_n);  // a súlyok 1, 2, ... 5 és 0 egyezésre

    // az előző num is kell, aszerint kell szorozni
    auto prev = (&_data.last())-(pre_n-1);
    for(int i=0;i<d->length();i++) (*d)[i].num.WeightByPrev1(wp, &prev->num);
}

//void Lottery::WeightByPrev2(QVector<Data>* d){
//    auto wp = WeightsByPrev2();  // a súlyok 1, 2, ... 5 és 0 egyezésre

//    // az előző num is kell, aszerint kell szorozni
//    auto prev = (&_data.last())-1;
//    for(int i=0;i<d->length();i++) (*d)[i].num.WeightByPrev1(wp, &prev->num);
//}

QVector<Lottery::Occurence> Lottery::SelectByOccurence(QVector<Data>& d, int db){
    QVector<Occurence> e;
    if(db<_settings.min || db>Lottery::_settings.max) return e;
    if(d.isEmpty()) return e;

    Weight(&d);
    auto h2 = Lottery::Histogram(d,0);

    // kiválasztjuk az n legjobbat
    for(int i=0;i<db;i++)
    {
        int ix = -1;
        int max=-1;
        for(int j=0;j<Lottery::Settings::TOTAL_NUMBERS;j++){
            int n = h2[j];
            if(n>max)
            {
                max=n;
                ix=j;
            }
        }
        e.append({ix+1, h2[ix]});
        h2[ix] = 0;
    }

    //std::sort(e.begin(), e.end());
    return e;
}


void Lottery::Save(const QVector<Lottery::Data> &d)
{
    QString fn = QDateTime::currentDateTimeUtc().toString("yyyyMMdd_hh:mm:ss")+".csv";
    QString ffn =Lottery::_settings.data_ffn(fn);
    QString txt;

    for(auto&i:d)
    {
        if(!txt.isEmpty()) txt+= com::helpers::StringHelper::NewLine;
        txt.append(i.num.ToString());
    }

    com::helpers::FileErrors err;
    com::helpers::TextFileHelper::Save(txt, ffn, &err);
}

/// az adatok 1, 2 ... 5. számára csinálja
/// az 1. 2. ... 5. nyerőszám gyakoriságát adja
/// az összes gyakoriságát adja
///
QVector<qreal> Lottery::Histogram(const QVector<Data>&d, int m)
{
    QVector<qreal> r(Lottery::Settings::TOTAL_NUMBERS);for(auto& i:r) i=0;//0-89
    if(m<0||m>_settings.NUMBERS) return r;

    for(auto&i:d){
        if(!m)//m az 0
        {
            for(int n =1;n<=_settings.NUMBERS;n++)
            {
                int x = i.num.number(n);// X: 1-90
                if(!x) continue; // 0 az hiba
                r[x-1]+= i.num.weight;
            }
        }
        else
        {
            int x = i.num.number(m);// X: 1-90
            r[x-1]+=i.num.weight;
        }
    }

    return r;
}


