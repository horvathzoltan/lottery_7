#ifndef LOTTERY_H
#define LOTTERY_H

#include <QDate>
//#include <QList>
#include <QApplication>
#include <QDir>
#include <QSet>
#include <QString>
#include <QVarLengthArray>
#include <QVector>
#include "../common/common/helpers/IniHelper/inihelper.h"
#include "../common/common/helpers/TextFileHelper/textfilehelper.h"

class Lottery
{
public:

    struct Settings{
        private:
            QDate _date = QDate::currentDate();

            const QString appname = "lottery_7_data";
            const QString download_dir = "download";
            const QString data_dir = "data";
            const QDir home = QDir(QDir::homePath());

            const QString home_fn = home.filePath(appname);

            const QString download_path = QDir(home_fn).filePath(download_dir);
            const QString data_root = QDir(home_fn).filePath(data_dir);
            QString data_path() { return QDir(data_root).filePath(yearweek());}
            QDir path(const QString& fn){
                auto p = QDir(fn);
                if(!p.exists()) p.mkpath(fn);
                return p;
            };


        public:
            static const int NUMBERS = 7;
            static const int TOTAL_NUMBERS = 35;

            qreal filter = 5;
            int min = 7; //min számok
            int max = 30; //max számok
            int shuff_max = 900;//250000;//25000;//900; // ennyit generál egyszerre
            int c_min = 10;
            int c_max = 1000000;
            int K = 7;
            int ticket_price = 400;
            QString ticket_curr = "Ft";
            QString url2= "http://www.lottoszamok.net/skandinav-lotto/";
            QString url = "https://bet.szerencsejatek.hu/cmsfiles/skandi.csv";
            QString download_ffn(){ return path(download_path).filePath("skandi.csv");};
            QString settings_ffn(){ return path(home_fn).filePath("settings.ini");};
            QString data_ffn(const QString &fn){ return path(data_path()).filePath(fn);};

            void setDate(QDate d){_date = d;};
            void datemm(){_date=_date.addDays(-7);};
            void datepp(){_date=_date.addDays(7);};

//            int year(){return _date.year();}
//            int week(){return _date.weekNumber();}
            QString yearweek(int *y = nullptr, int *w = nullptr){
                auto t = _date;
                auto t_y = t.year();
                auto t_w = t.weekNumber();

                if(y) *y = t_y;
                if(w) *w = t_w;
                return QString::number(t_y)+"-"+QString::number(t_w);
            }

            //label_f : Lottery::_settings.filter
            //labelc : Lottery::_settings.K
            // label_w : yearweek
            //TODO Toini Fromini - inihelper
            void ToIni(){
                QMap<QString, QString> imap;
                static const QString KEY_filter = "filter";
                static const QString KEY_K = "K";
                static const QString KEY_date = "date";

                imap.insert(KEY_filter, QString::number(Lottery::_settings.filter));
                imap.insert(KEY_K, QString::number(Lottery::_settings.K));
                imap.insert(KEY_date, Lottery::_settings._date.toString());

                auto txt = com::helpers::IniHelper::toString(imap, "cirmos");

                QString fn = Lottery::_settings.settings_ffn();

                com::helpers::FileErrors err;
                com::helpers::TextFileHelper::Save(txt, fn, &err);
            }

            void FromIni(){
                QString fn = Lottery::_settings.settings_ffn();

                com::helpers::FileErrors err;
                auto lines = com::helpers::TextFileHelper::LoadLines(fn, &err);

                auto m = com::helpers::IniHelper::Parse(lines);

                if(m.contains("filter")){
                    bool isok;
                    int v = m.value("filter").toInt(&isok);
                    if(isok) Lottery::_settings.filter = v;
                }

                if(m.contains("K")){
                    bool isok;
                    int v = m.value("K").toInt(&isok);
                    if(isok) Lottery::_settings.K = v;
                }

                if(m.contains("date")){
                    //bool isok;
                    auto v = QDate::fromString(m.value("date"));
                    //Lottery::_settings._date = v;
                }
            }

            // igaz, ha 2-es későbbi mint az 1-es azaz 1<2
            static bool isDateEquals(int y1, int w1, int y2, int w2){
                return y1==y2 && w1==w2;
            }

            static bool isAfter(int y1, int w1, int y2, int w2){
                if(y1<y2) return true;
                if(y1>y2) return false;
                return w2>w1;
            }

            static bool isAfterOrThis(int y1, int w1, int y2, int w2){
                if(y1<y2) return true;
                if(y1>y2) return false;
                return w2>=w1;
            }

    };

    static Settings _settings;    

    struct Hit{
        int count;
        int prize;
        QString currency;
        QString desc;

        static Hit FromCsv(const QStringList&, const QString &desc);
    };

    struct Numbers{
        private:
            int numbers[Lottery::Settings::NUMBERS];
        public:
            qreal weight = 1;

        QString ToString() const {
            QString e;
            for(auto&i:numbers){
                if(!e.isEmpty())e+=",";
                e+=QString::number(i);
            }

            return e;
        }

        QString ToString(const Numbers& n2) const {
            QString e;
            for(auto&i:numbers){
                if(!e.isEmpty())e+=",";
                e+=QString::number(i);
                if(n2.contains(i)) e+='*';
            }

            return e;
        }

        bool contains(int i) const {auto a = std::find(numbers, numbers+_settings.NUMBERS,i);
            return a!=numbers+_settings.NUMBERS;}

        void WeightByParity(const QVector<qreal>& w)
        {
            weight *= w[NumbersEven()];
        }

        void WeightByPentilis(const QVector<qreal>& w)
        {
            weight *= w[NumbersPentilis()];
        }

        void WeightByPrev1(const QVector<qreal>& w, Numbers *prev)
        {
            auto n = this->HitNum(*prev);
            weight *= w[n];
        }


        bool operator== (const Numbers& r){
            int o=0;
            for(auto&i:numbers){
                for(auto&j:r.numbers) if(i==j) {o++;break;}
            }
            return o==_settings.NUMBERS;
        };

        void sort(){std::sort(numbers, numbers+Settings::NUMBERS);}

        // páros számok a húzásban
        int NumbersEven()const{int p=0;for(auto&i:numbers)if(!(i%2))p++;return p;}


        // külömböző pentilisek száma a húzásban
        int NumbersPentilis()const{
            QSet<int> pen;
            static const int r = Lottery::Settings::TOTAL_NUMBERS/Settings::NUMBERS;//18
            for(auto&i:numbers) pen.insert(i/r); // pentilis

            return pen.count(); // a külömbözők száma
        }

        // ennyies a találat
        int HitNum(const Numbers& d) const{
            int x = 0; //ennyi találat
            for(auto&i:numbers){
                for(auto&j:d.numbers) if(j==i) x++;
                //for(int j=0;j<5;j++) if(d.numbers[j]==i) x++;
            }
            if(x<1 || x>Settings::NUMBERS) return 0;
            return x-1;
        }

        int number(int i) const{
            int ix = i-1;
            if(ix<0 || ix>=Settings::NUMBERS) return 0;
            return numbers[ix];
        }



        void setNumber(int i, int n){
            int ix = i-1;
            if(ix<0 || ix>=Settings::NUMBERS) return;
            numbers[ix] = n;
        };
    };



    struct Data{ //lottery
        int year;
        int week;
        QDate datetime;
        Numbers num;
    private:        
        Hit hits[Settings::NUMBERS]; // melyik találat hogy fizet a héten

    public:
        void setHit(int i, Hit h){
            int ix = i-1;
            if(ix<0 || ix>=Settings::NUMBERS) return;
            hits[ix] = h;
        }

        // a this díjazása alapján a d kombináció mennyi találatt illetve pénz
        int prizeCur(const Data& d, QString* curr, int* pixe = nullptr) const{
            auto pix = num.HitNum(d.num);
            if(pixe) *pixe = pix;
            auto h = hits[pix];
            auto p = h.prize;
            if(p<1) return 0;
            if(curr) *curr = h.currency;
            return p;
        }


        static bool AscByDate( const Data& l, const Data& r )
        {
            if(l.year == r.year) return (l.week<r.week);
            return l.year<r.year;
        }

    };
    static Data _next;

    static QVector<Data> _data;

    Lottery();
    static bool FromFile(const QString& fp, int y, int w, bool* b);
    static QStringList CsvSplit(const QString& s);

    struct RefreshR
    {
        bool isOk = false;
        QVector<qreal> histogram;
        QVector<qreal> histograms[Settings::NUMBERS];
        //QVector<Numbers> last;
        int min_y;
        int max_y;
        bool isExistInFile; // ha az adott hét benne volt a fájlban
    };

    static RefreshR Refresh(int year, int week);

    struct Occurence{
        int num;
        qreal hist;

        bool operator < (const Occurence& r){ return num < r.num;};
    };

    struct ShuffleR
    {
        QVector<Occurence> num; // a húzás leggyakoribb számai 5-10
        QVector<Data> comb; // a leggyakoribb számokból képzett kombinációk
        //QVector<qreal> weight;
        bool isok;
    };

    static QVector<Data> Shuffle(int *, int max);

    static void Save(const QVector<Data>&);

    static QVector<qreal> Histogram(const QVector<Data>& d, int x);

    static QVector<QVector<int>> SelectByCombination(const QVector<Occurence>& p, int k);
    static QVector<QVector<int>> Combination(int N, int K);
    static QVector<Data> ToData(QVector<QVector<int>>& p);


    static QVector<Occurence> SelectByOccurence(QVector<Data> &d, int i);
    static ShuffleR Generate(int *p, int k, int max);
    static ShuffleR Generate2(QVector<Lottery::Data>& d);
    struct BestHit{
        int ix;
        Numbers numbers;
    };

    struct RefreshByWeekR{
        int shuffnum;
        QVector<Occurence> num; // a húzás leggyakoribb számai 5-10
        QVector<Data> comb; // a leggyakoribb számokból képzett kombinációk
        bool isok;
        QVector<QVector<struct Lottery::BestHit>> besthits;
        QVector<Numbers> mweights;
        int mweight;

        QString ToString() const {
            return QString::number(shuffnum);//QString("%2 - %1 db").arg(comb.count()).arg(shuffnum);
        }
    };
    static RefreshByWeekR RefreshByWeek();
    static QFileInfoList ExclusionByWeek();
    static QFileInfoList DataFileInfoListByWeek();


    static QVector<QVector<BestHit>> FindBestHit(const QVector<Lottery::Data> &fd,const Numbers& numbers);
    //static QVector<qreal> SumWeight(const QVector<QVector<qreal>>&w);

    static void Weight(QVector<Data>* d);
    static void WeightClear(QVector<Data> *d);
    static void WeightByParity(QVector<Data>* d);
    static void WeightByPentilis(QVector<Data>* d);

    static QVector<qreal> WeightsByParity();
    static QVector<qreal> WeightsByPentilis();
    static QVector<Lottery::Numbers> FindByMaxWeight(const QVector<Lottery::Data> &fd, int *maxweight);
    static void WeightByPrev(QVector<Data> *d, int);
    static QVector<qreal> WeightsByPrev(int i);
    //static void WeightByPrev2(QVector<Data> *d);
    //static QVector<qreal> WeightsByPrev2();

};

#endif // LOTTERY_H
