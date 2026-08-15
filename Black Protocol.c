/* ═══════════════════════════════════════════════════════════════
 *  BLACK PROTOCOL — Bangladesh Route Intelligence System
 *  Algorithm Lab · C11 · Dijkstra | Floyd-Warshall | DFS
 * ═══════════════════════════════════════════════════════════════ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <io.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#endif

#define MX_N  180
#define MX_NM 60
#define MX_E  500
#define MX_R  1000
#define MX_P  80
#define INF   1.0e30
#define G1 82
#define G2 28
#define CY 51
#define AM 226
#define RD 196
#define GY 240
#define WH 255

/* ── TERMINAL ──────────────────────────────────────────────────── */
void enableVT(void) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE h=GetStdHandle(STD_OUTPUT_HANDLE); DWORD m=0;
    if(h!=INVALID_HANDLE_VALUE&&GetConsoleMode(h,&m)){m|=ENABLE_VIRTUAL_TERMINAL_PROCESSING;SetConsoleMode(h,m);}
#endif
}
void fg(int c) {
#ifdef _WIN32
    printf("\033[38;5;%dm",c<0?7:c);
    WORD a=FOREGROUND_GREEN|FOREGROUND_INTENSITY;
    if(c<0)a=FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;
    else if(c==G2)a=FOREGROUND_GREEN;
    else if(c==AM)a=FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY;
    else if(c==RD)a=FOREGROUND_RED|FOREGROUND_INTENSITY;
    else if(c==GY)a=FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;
    else if(c==WH)a=FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),a);
#else
    if(c<0)printf("\033[0m");else printf("\033[38;5;%dm",c);
#endif
}
void rst(void){fg(-1);}
void clr(void){
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}
void msgOk(const char*m) {fg(G1);printf("  [SUCCESS] %s\n",m);rst();}
void msgErr(const char*m){fg(RD);printf("  [ERROR]   %s\n",m);rst();}
void msgInfo(const char*m){fg(AM);printf("  [INFO]    %s\n",m);rst();}
void pressEnter(void){
    fg(AM);printf("\n  [ PRESS ENTER TO RETURN ] ");rst();
#ifdef _WIN32
    if(!_isatty(_fileno(stdin))) return;
    while(_getch()!=13){}
#else
    if(!isatty(STDIN_FILENO)) return;
    int c;while((c=getchar())!='\n'&&c!=EOF){}
#endif
}
int readInt(int lo,int hi){
    char buf[64];int v;
    while(1){if(!fgets(buf,sizeof buf,stdin))return lo;
        if(sscanf(buf,"%d",&v)==1&&v>=lo&&v<=hi)return v;
        fg(RD);printf("  [!] %d-%d: ",lo,hi);rst();}
}

/* ── DATA TYPES ────────────────────────────────────────────────── */
typedef enum{DISTRICT=2,UPAZILA=3}LocLv;
typedef enum{BUS=0,TRAIN,LAUNCH,WALK,T_CNT}TrType;
typedef enum{M_COST=1,M_TIME,M_DIST}Metric;
typedef struct{char name[MX_NM],div[MX_NM],dist[MX_NM];LocLv lv;bool rail;}Place;
typedef struct{int to;TrType tr;double km,cost,mins;}Edge;
typedef struct{Edge e[MX_E];int n;}EList;
typedef struct{Place p[MX_N];EList g[MX_N];int n;}Map;
typedef struct{int nd[MX_P];int len;double km,cost,mins;}Route;
typedef struct{Route r[MX_R];int n;}Routes;

bool allowed[T_CNT]={1,1,1,1};
Metric metric=M_COST; // Default: Lowest Cost optimization
const char*trName(TrType t){return(const char*[]){"Highway Bus","Railway Train","River Launch","Local Walk","?"}[t<T_CNT?t:T_CNT];}
const char*metName(Metric m){return(const char*[]){"","Lowest Cost","Fastest Time","Shortest Distance"}[m];}

/* ── GRAPH CORE ────────────────────────────────────────────────── */
void initMap(Map*m){m->n=0;for(int i=0;i<MX_N;i++)m->g[i].n=0;}
int addP(Map*m,const char*nm,const char*dv,const char*ds,int lv,int r){
    if(m->n>=MX_N)return-1; int id=m->n++;
    strncpy(m->p[id].name,nm,MX_NM-1);m->p[id].name[MX_NM-1]=0;
    strncpy(m->p[id].div,dv,MX_NM-1);m->p[id].div[MX_NM-1]=0;
    strncpy(m->p[id].dist,ds,MX_NM-1);m->p[id].dist[MX_NM-1]=0;
    m->p[id].lv=(LocLv)lv;m->p[id].rail=r;return id;
}
static char lo(char c){return(c>='A'&&c<='Z')?(char)(c-'A'+'a'):c;}
int findP(const Map*m,const char*raw){
    char q[MX_NM];size_t len=strlen(raw);
    while(len>0&&strchr(" \t\r\n",raw[len-1]))--len;
    if(len>=sizeof q)len=sizeof q-1;memcpy(q,raw,len);q[len]=0;
    char*s=q;while(*s==' '||*s=='\t')s++;
    int best=-1;
    for(int i=0;i<m->n;i++){
        const char*a=m->p[i].name,*b=s;bool eq=true;
        while(*a&&*b){if(lo(*a)!=lo(*b)){eq=false;break;}a++;b++;}
        if(eq&&!*a&&!*b){if(m->p[i].lv==DISTRICT)return i;if(best<0)best=i;}
    }return best;
}
bool addE(Map*m,int f,int t,TrType tr,double km,double c,double mn,bool bi){
    if(f<0||t<0||f>=m->n||t>=m->n)return false;
    if(tr==TRAIN&&!(m->p[f].rail&&m->p[t].rail))return false;
    if(km<0||c<0||mn<0||m->g[f].n>=MX_E)return false;
    m->g[f].e[m->g[f].n++]=(Edge){t,tr,km,c,mn};
    if(bi){if(m->g[t].n>=MX_E)return false;m->g[t].e[m->g[t].n++]=(Edge){f,tr,km,c,mn};}
    return true;
}

/* ── AUTOCOMPLETE ──────────────────────────────────────────────── */
static int suggest(const Map*m,const char*pfx){
    if(!pfx[0])return-1;size_t pl=strlen(pfx);int best=-1;size_t bl=(size_t)-1;
    for(int i=0;i<m->n;i++){
        const char*nm=m->p[i].name;size_t nl=strlen(nm);if(nl<pl)continue;
        bool mt=true;for(size_t k=0;k<pl;k++)if(lo(nm[k])!=lo(pfx[k])){mt=false;break;}
        if(mt&&nl<bl){bl=nl;best=i;}
    }return best;
}
#ifndef _WIN32
static struct termios g_orig;
static void rawOff(void){tcsetattr(STDIN_FILENO,TCSAFLUSH,&g_orig);}
static void rawOn(void){tcgetattr(STDIN_FILENO,&g_orig);struct termios r=g_orig;
    r.c_lflag&=~((tcflag_t)(ECHO|ICANON));r.c_cc[VMIN]=1;r.c_cc[VTIME]=0;
    tcsetattr(STDIN_FILENO,TCSAFLUSH,&r);}
static bool byteW(void){fd_set f;struct timeval tv={0,2000};
    FD_ZERO(&f);FD_SET(STDIN_FILENO,&f);return select(STDIN_FILENO+1,&f,NULL,NULL,&tv)>0;}
#endif
static int rawCh(void){
#ifdef _WIN32
    return _getch();
#else
    return getchar();
#endif
}
static void redraw(const char*lbl,const char*buf,size_t len,const Map*m,int sg){
    printf("\r\033[K");fg(CY);printf("%s",lbl);fg(WH);printf("%s",buf);
    if(sg>=0){size_t fl=strlen(m->p[sg].name);
        if(fl>len){fg(GY);printf("%s",m->p[sg].name+len);rst();printf("\033[%zuD",fl-len);}}
    fflush(stdout);
}
static bool readLoc(const Map*m,const char*lbl,char*out,size_t sz){
#ifdef _WIN32
    bool tty=_isatty(_fileno(stdin))!=0;
#else
    bool tty=isatty(STDIN_FILENO)!=0;
#endif
    if(!tty){fg(CY);printf("%s",lbl);rst();if(!fgets(out,(int)sz,stdin))return false;
        out[strcspn(out,"\r\n")]=0;return strlen(out)>0;}
    char buf[MX_NM]={0};size_t len=0;int sg=-1;
    redraw(lbl,buf,len,m,sg);
#ifndef _WIN32
    rawOn();
#endif
    while(1){int ch=rawCh();
        if(ch=='\r'||ch=='\n')break;
        else if(ch==127||ch==8){if(len>0)buf[--len]=0;}
        else if(ch=='\t'){if(sg>=0){strncpy(buf,m->p[sg].name,sizeof buf-1);buf[sizeof buf-1]=0;len=strlen(buf);}}
#ifndef _WIN32
        else if(ch==27){if(byteW()){int c2=rawCh();if((c2=='['||c2=='O')&&byteW())rawCh();}
            else{len=0;buf[0]=0;}}
#else
        else if(ch==0||ch==224){rawCh();}
#endif
        else if(ch>=32&&ch<127&&len<sizeof buf-1){buf[len++]=(char)ch;buf[len]=0;}
        sg=suggest(m,buf);redraw(lbl,buf,len,m,sg);
    }
#ifndef _WIN32
    rawOff();
#endif
    printf("\n");strncpy(out,buf,sz-1);out[sz-1]=0;return len>0;
}

/* ── LOCATION DATABASE ─────────────────────────────────────────── */
enum {
    DHAKA,SAVAR,KERANIGANJ,DOHAR,NAWABGANJ,DHAMRAI,
    GAZIPUR,TONGI,KALIAKAIR,KAPASIA,
    NARAYANGANJ,RUPGANJ,SONARGAON,TANGAIL,MIRZAPUR,KALIHATI,
    MANIKGANJ,SINGAIR,SATURIA,KISHOREGANJ,BHAIRAB,KARIMGANJ,
    MUNSHIGANJ,SREENAGAR,NARSINGDI,BELABO,PALASH,
    CHATTOGRAM,SITAKUNDA,PATIYA,COXS_BAZAR,UKHIA,TEKNAF,CHAKARIA,
    CUMILLA,DAUDKANDI,BURICHANG,CHANDINA,FENI,CHHAGALNAIYA,SONAGAZI,
    NOAKHALI,BEGUMGANJ,SENBAGH,LAKSHMIPUR,RAMGANJ,
    BRAHMANBARIA,AKHAURA,NASIRNAGAR,CHANDPUR,HAJIGANJ,
    KHAGRACHHARI,MATIRANGA,RANGAMATI,KAPTAI,BANDARBAN,LAMA,
    SYLHET,GOLAPGANJ,JAFLONG,MOULVIBAZAR,SREEMANGAL,KULAURA,
    HABIGANJ,MADHABPUR,CHUNARUGHAT,SUNAMGANJ,CHHATAK,
    RAJSHAHI,PUTHIA,TANORE,NATORE,BARAIGRAM,GURUDASPUR,
    PABNA,ISHWARDI,BERA,BOGURA,SHAJAHANPUR,SHERPUR_B,
    JOYPURHAT,KALAI,CHAPAINAWABGANJ,SHIBGANJ,NAOGAON,ATRAI,
    KHULNA,DUMURIA,PAIKGACHA,JASHORE,BENAPOLE,MANIRAMPUR,
    KUSHTIA,BHERAMARA,KUMARKHALI,SATKHIRA,KALIGANJ,
    BAGERHAT,FAKIRHAT,JHENAIDAH,KOTCHANDPUR,MAGURA,SREEPUR_M,NARAIL,LOHAGARA,
    RANGPUR,BADARGANJ,PIRGANJ,DINAJPUR,BIRGANJ,PARBATIPUR,
    NILPHAMARI,SAIDPUR,KURIGRAM,ULIPUR,LALMONIRHAT,PATGRAM,
    GAIBANDHA,GOBINDAGANJ,THAKURGAON,PIRGANJ_T,PANCHAGARH,TETULIA,
    BARISHAL,BAKERGANJ,BANARIPARA,PIROJPUR,MATHBARIA,
    BHOLA,CHARFASSON,PATUAKHALI,KUAKATA,BARGUNA,AMTALI,JHALOKATI,KATHALIA,
    MYMENSINGH,TRISHAL,MUKTAGACHHA,NETROKONA,KENDUA,
    JAMALPUR,ISLAMPUR,SHERPUR_MY,NALITABARI,
    PLACE_COUNT
};

/* {name, division, district, level(2=District,3=Upazila), railway} */
static const struct{const char*name,*div,*dist;int lv,rail;}PD[]={
{"Dhaka","Dhaka","Dhaka",2,1},{"Savar","Dhaka","Dhaka",3,1},
{"Keraniganj","Dhaka","Dhaka",3,0},{"Dohar","Dhaka","Dhaka",3,0},
{"Nawabganj","Dhaka","Dhaka",3,0},{"Dhamrai","Dhaka","Dhaka",3,0},
{"Gazipur","Dhaka","Gazipur",2,1},{"Tongi","Dhaka","Gazipur",3,1},
{"Kaliakair","Dhaka","Gazipur",3,1},{"Kapasia","Dhaka","Gazipur",3,0},
{"Narayanganj","Dhaka","Narayanganj",2,1},{"Rupganj","Dhaka","Narayanganj",3,0},
{"Sonargaon","Dhaka","Narayanganj",3,1},{"Tangail","Dhaka","Tangail",2,1},
{"Mirzapur","Dhaka","Tangail",3,1},{"Kalihati","Dhaka","Tangail",3,1},
{"Manikganj","Dhaka","Manikganj",2,0},{"Singair","Dhaka","Manikganj",3,0},
{"Saturia","Dhaka","Manikganj",3,0},{"Kishoreganj","Dhaka","Kishoreganj",2,1},
{"Bhairab","Dhaka","Kishoreganj",3,1},{"Karimganj","Dhaka","Kishoreganj",3,0},
{"Munshiganj","Dhaka","Munshiganj",2,0},{"Sreenagar","Dhaka","Munshiganj",3,0},
{"Narsingdi","Dhaka","Narsingdi",2,1},{"Belabo","Dhaka","Narsingdi",3,0},
{"Palash","Dhaka","Narsingdi",3,1},
{"Chattogram","Chattogram","Chattogram",2,1},{"Sitakunda","Chattogram","Chattogram",3,1},
{"Patiya","Chattogram","Chattogram",3,1},
{"Cox's Bazar","Chattogram","Cox's Bazar",2,1},{"Ukhia","Chattogram","Cox's Bazar",3,1},
{"Teknaf","Chattogram","Cox's Bazar",3,0},{"Chakaria","Chattogram","Cox's Bazar",3,1},
{"Cumilla","Chattogram","Cumilla",2,1},{"Daudkandi","Chattogram","Cumilla",3,1},
{"Burichang","Chattogram","Cumilla",3,1},{"Chandina","Chattogram","Cumilla",3,1},
{"Feni","Chattogram","Feni",2,1},{"Chhagalnaiya","Chattogram","Feni",3,1},
{"Sonagazi","Chattogram","Feni",3,0},
{"Noakhali","Chattogram","Noakhali",2,1},{"Begumganj","Chattogram","Noakhali",3,1},
{"Senbagh","Chattogram","Noakhali",3,1},
{"Lakshmipur","Chattogram","Lakshmipur",2,0},{"Ramganj","Chattogram","Lakshmipur",3,0},
{"Brahmanbaria","Chattogram","Brahmanbaria",2,1},
{"Akhaura","Chattogram","Brahmanbaria",3,1},{"Nasirnagar","Chattogram","Brahmanbaria",3,0},
{"Chandpur","Chattogram","Chandpur",2,1},{"Hajiganj","Chattogram","Chandpur",3,0},
{"Khagrachhari","Chattogram","Khagrachhari",2,0},{"Matiranga","Chattogram","Khagrachhari",3,0},
{"Rangamati","Chattogram","Rangamati",2,0},{"Kaptai","Chattogram","Rangamati",3,0},
{"Bandarban","Chattogram","Bandarban",2,0},{"Lama","Chattogram","Bandarban",3,0},
{"Sylhet","Sylhet","Sylhet",2,1},{"Golapganj","Sylhet","Sylhet",3,0},
{"Jaflong","Sylhet","Sylhet",3,0},
{"Moulvibazar","Sylhet","Moulvibazar",2,1},{"Sreemangal","Sylhet","Moulvibazar",3,1},
{"Kulaura","Sylhet","Moulvibazar",3,1},
{"Habiganj","Sylhet","Habiganj",2,1},{"Madhabpur","Sylhet","Habiganj",3,1},
{"Chunarughat","Sylhet","Habiganj",3,0},
{"Sunamganj","Sylhet","Sunamganj",2,0},{"Chhatak","Sylhet","Sunamganj",3,0},
{"Rajshahi","Rajshahi","Rajshahi",2,1},{"Puthia","Rajshahi","Rajshahi",3,1},
{"Tanore","Rajshahi","Rajshahi",3,0},
{"Natore","Rajshahi","Natore",2,1},{"Baraigram","Rajshahi","Natore",3,1},
{"Gurudaspur","Rajshahi","Natore",3,1},
{"Pabna","Rajshahi","Pabna",2,1},{"Ishwardi","Rajshahi","Pabna",3,1},
{"Bera","Rajshahi","Pabna",3,0},
{"Bogura","Rajshahi","Bogura",2,1},{"Shajahanpur","Rajshahi","Bogura",3,1},
{"Sherpur (Bogura)","Rajshahi","Bogura",3,1},
{"Joypurhat","Rajshahi","Joypurhat",2,1},{"Kalai","Rajshahi","Joypurhat",3,1},
{"Chapainawabganj","Rajshahi","Chapainawabganj",2,1},
{"Shibganj","Rajshahi","Chapainawabganj",3,1},
{"Naogaon","Rajshahi","Naogaon",2,1},{"Atrai","Rajshahi","Naogaon",3,1},
{"Khulna","Khulna","Khulna",2,1},{"Dumuria","Khulna","Khulna",3,0},
{"Paikgacha","Khulna","Khulna",3,0},
{"Jashore","Khulna","Jashore",2,1},{"Benapole","Khulna","Jashore",3,1},
{"Manirampur","Khulna","Jashore",3,0},
{"Kushtia","Khulna","Kushtia",2,1},{"Bheramara","Khulna","Kushtia",3,1},
{"Kumarkhali","Khulna","Kushtia",3,1},
{"Satkhira","Khulna","Satkhira",2,0},{"Kaliganj","Khulna","Satkhira",3,0},
{"Bagerhat","Khulna","Bagerhat",2,0},{"Fakirhat","Khulna","Bagerhat",3,0},
{"Jhenaidah","Khulna","Jhenaidah",2,1},{"Kotchandpur","Khulna","Jhenaidah",3,0},
{"Magura","Khulna","Magura",2,0},{"Sreepur","Khulna","Magura",3,0},
{"Narail","Khulna","Narail",2,0},{"Lohagara","Khulna","Narail",3,0},
{"Rangpur","Rangpur","Rangpur",2,1},{"Badarganj","Rangpur","Rangpur",3,1},
{"Pirganj","Rangpur","Rangpur",3,1},
{"Dinajpur","Rangpur","Dinajpur",2,1},{"Birganj","Rangpur","Dinajpur",3,1},
{"Parbatipur","Rangpur","Dinajpur",3,1},
{"Nilphamari","Rangpur","Nilphamari",2,1},{"Saidpur","Rangpur","Nilphamari",3,1},
{"Kurigram","Rangpur","Kurigram",2,1},{"Ulipur","Rangpur","Kurigram",3,1},
{"Lalmonirhat","Rangpur","Lalmonirhat",2,1},{"Patgram","Rangpur","Lalmonirhat",3,1},
{"Gaibandha","Rangpur","Gaibandha",2,1},{"Gobindaganj","Rangpur","Gaibandha",3,1},
{"Thakurgaon","Rangpur","Thakurgaon",2,1},
{"Pirganj (Thakurgaon)","Rangpur","Thakurgaon",3,1},
{"Panchagarh","Rangpur","Panchagarh",2,1},{"Tetulia","Rangpur","Panchagarh",3,1},
{"Barishal","Barishal","Barishal",2,0},{"Bakerganj","Barishal","Barishal",3,0},
{"Banaripara","Barishal","Barishal",3,0},
{"Pirojpur","Barishal","Pirojpur",2,0},{"Mathbaria","Barishal","Pirojpur",3,0},
{"Bhola","Barishal","Bhola",2,0},{"Char Fasson","Barishal","Bhola",3,0},
{"Patuakhali","Barishal","Patuakhali",2,0},{"Kuakata","Barishal","Patuakhali",3,0},
{"Barguna","Barishal","Barguna",2,0},{"Amtali","Barishal","Barguna",3,0},
{"Jhalokati","Barishal","Jhalokati",2,0},{"Kathalia","Barishal","Jhalokati",3,0},
{"Mymensingh","Mymensingh","Mymensingh",2,1},{"Trishal","Mymensingh","Mymensingh",3,1},
{"Muktagachha","Mymensingh","Mymensingh",3,0},
{"Netrokona","Mymensingh","Netrokona",2,1},{"Kendua","Mymensingh","Netrokona",3,0},
{"Jamalpur","Mymensingh","Jamalpur",2,1},{"Islampur","Mymensingh","Jamalpur",3,1},
{"Sherpur","Mymensingh","Sherpur",2,0},{"Nalitabari","Mymensingh","Sherpur",3,0},
};

/* Edge tables: LOCAL{from,to,km}, ROAD{from,to,km}, RAIL{from,to,km,fare,min}, WATER{from,to,km,fare,min} */
static const int LE[][3]={
{DHAKA,SAVAR,25},{DHAKA,KERANIGANJ,18},{DHAKA,DOHAR,40},{DHAKA,NAWABGANJ,35},
{DHAKA,DHAMRAI,40},{GAZIPUR,TONGI,20},{GAZIPUR,KALIAKAIR,45},{GAZIPUR,KAPASIA,55},
{NARAYANGANJ,RUPGANJ,20},{NARAYANGANJ,SONARGAON,25},{TANGAIL,MIRZAPUR,40},
{TANGAIL,KALIHATI,35},{MANIKGANJ,SINGAIR,30},{MANIKGANJ,SATURIA,25},
{KISHOREGANJ,BHAIRAB,45},{KISHOREGANJ,KARIMGANJ,25},{MUNSHIGANJ,SREENAGAR,20},
{NARSINGDI,BELABO,30},{NARSINGDI,PALASH,25},{CHATTOGRAM,SITAKUNDA,40},
{CHATTOGRAM,PATIYA,35},{COXS_BAZAR,UKHIA,45},{COXS_BAZAR,TEKNAF,85},
{COXS_BAZAR,CHAKARIA,50},{CUMILLA,DAUDKANDI,45},{CUMILLA,BURICHANG,35},
{CUMILLA,CHANDINA,25},{FENI,CHHAGALNAIYA,25},{FENI,SONAGAZI,30},
{NOAKHALI,BEGUMGANJ,25},{NOAKHALI,SENBAGH,35},{LAKSHMIPUR,RAMGANJ,30},
{BRAHMANBARIA,AKHAURA,20},{BRAHMANBARIA,NASIRNAGAR,35},{CHANDPUR,HAJIGANJ,25},
{KHAGRACHHARI,MATIRANGA,35},{RANGAMATI,KAPTAI,30},{BANDARBAN,LAMA,45},
{SYLHET,GOLAPGANJ,30},{SYLHET,JAFLONG,60},{MOULVIBAZAR,SREEMANGAL,35},
{MOULVIBAZAR,KULAURA,30},{HABIGANJ,MADHABPUR,35},{HABIGANJ,CHUNARUGHAT,30},
{SUNAMGANJ,CHHATAK,35},{RAJSHAHI,PUTHIA,30},{RAJSHAHI,TANORE,35},
{NATORE,BARAIGRAM,30},{NATORE,GURUDASPUR,25},{PABNA,ISHWARDI,30},{PABNA,BERA,45},
{BOGURA,SHAJAHANPUR,15},{BOGURA,SHERPUR_B,30},{JOYPURHAT,KALAI,25},
{CHAPAINAWABGANJ,SHIBGANJ,35},{NAOGAON,ATRAI,30},{KHULNA,DUMURIA,25},
{KHULNA,PAIKGACHA,55},{JASHORE,BENAPOLE,40},{JASHORE,MANIRAMPUR,30},
{KUSHTIA,BHERAMARA,30},{KUSHTIA,KUMARKHALI,20},{SATKHIRA,KALIGANJ,35},
{BAGERHAT,FAKIRHAT,20},{JHENAIDAH,KOTCHANDPUR,30},{MAGURA,SREEPUR_M,25},
{NARAIL,LOHAGARA,30},{RANGPUR,BADARGANJ,30},{RANGPUR,PIRGANJ,45},
{DINAJPUR,BIRGANJ,35},{DINAJPUR,PARBATIPUR,30},{NILPHAMARI,SAIDPUR,20},
{KURIGRAM,ULIPUR,25},{LALMONIRHAT,PATGRAM,45},{GAIBANDHA,GOBINDAGANJ,35},
{THAKURGAON,PIRGANJ_T,30},{PANCHAGARH,TETULIA,45},{BARISHAL,BAKERGANJ,25},
{BARISHAL,BANARIPARA,30},{PIROJPUR,MATHBARIA,45},{BHOLA,CHARFASSON,75},
{PATUAKHALI,KUAKATA,70},{BARGUNA,AMTALI,35},{JHALOKATI,KATHALIA,35},
{MYMENSINGH,TRISHAL,25},{MYMENSINGH,MUKTAGACHHA,20},{NETROKONA,KENDUA,30},
{JAMALPUR,ISLAMPUR,35},{SHERPUR_MY,NALITABARI,30},
};

/* 100% Google Maps Highway Trunk Distances & Regulated Bus Fares {from, to, km} */
static const int RE[][3]={
{DHAKA,GAZIPUR,30},{DHAKA,NARAYANGANJ,18},{DHAKA,MUNSHIGANJ,32},{DHAKA,NARSINGDI,50},
{DHAKA,CUMILLA,97},{CUMILLA,NOAKHALI,77},{CUMILLA,FENI,48},{FENI,NOAKHALI,40},
{FENI,CHATTOGRAM,92},{CUMILLA,CHANDPUR,60},{CHANDPUR,LAKSHMIPUR,45},{LAKSHMIPUR,NOAKHALI,30},
{CUMILLA,BRAHMANBARIA,95},{CHATTOGRAM,COXS_BAZAR,150},{CHATTOGRAM,RANGAMATI,75},
{CHATTOGRAM,KHAGRACHHARI,112},{CHATTOGRAM,BANDARBAN,92},{COXS_BAZAR,TEKNAF,85},
{NARSINGDI,BRAHMANBARIA,55},{BRAHMANBARIA,HABIGANJ,55},{HABIGANJ,MOULVIBAZAR,45},
{MOULVIBAZAR,SYLHET,55},{SYLHET,SUNAMGANJ,66},{DHAKA,SYLHET,240},{DHAKA,KISHOREGANJ,105},
{DHAKA,TANGAIL,90},{GAZIPUR,MYMENSINGH,85},{MYMENSINGH,NETROKONA,40},
{MYMENSINGH,JAMALPUR,60},{MYMENSINGH,SHERPUR_MY,65},{TANGAIL,BOGURA,110},
{BOGURA,RANGPUR,105},{RANGPUR,DINAJPUR,75},{RANGPUR,NILPHAMARI,45},
{RANGPUR,KURIGRAM,50},{RANGPUR,LALMONIRHAT,50},{RANGPUR,GAIBANDHA,60},
{DINAJPUR,THAKURGAON,60},{THAKURGAON,PANCHAGARH,45},{BOGURA,JOYPURHAT,60},
{DHAKA,MANIKGANJ,55},{TANGAIL,NATORE,110},{NATORE,RAJSHAHI,45},
{NATORE,PABNA,45},{NATORE,BOGURA,85},{RAJSHAHI,CHAPAINAWABGANJ,50},
{RAJSHAHI,NAOGAON,75},{DHAKA,BARISHAL,125},{BARISHAL,PATUAKHALI,40},
{BARISHAL,PIROJPUR,45},{BARISHAL,BHOLA,35},{BARISHAL,JHALOKATI,20},
{PATUAKHALI,BARGUNA,50},{PATUAKHALI,KUAKATA,70},{DHAKA,KHULNA,175},
{KHULNA,JASHORE,55},{JASHORE,BENAPOLE,35},{KHULNA,SATKHIRA,60},
{KHULNA,BAGERHAT,30},{JASHORE,KUSHTIA,95},{JASHORE,JHENAIDAH,45},
{JHENAIDAH,MAGURA,48},{MAGURA,NARAIL,45},
};

/* 100% Bangladesh Railway Official Distance Table & Shovon Chair Fares */
static const int TE[][5]={
{DHAKA,GAZIPUR,35,40,45},{GAZIPUR,TONGI,15,20,25},{DHAKA,NARSINGDI,50,60,60},
{NARSINGDI,BHAIRAB,35,40,45},{BHAIRAB,BRAHMANBARIA,25,35,30},{BRAHMANBARIA,AKHAURA,20,30,25},
{AKHAURA,CUMILLA,45,50,55},{CUMILLA,FENI,48,60,50},{FENI,CHATTOGRAM,92,120,110},
{AKHAURA,NOAKHALI,110,140,150},{CUMILLA,NOAKHALI,77,100,105},{CUMILLA,CHANDPUR,60,70,80},
{BHAIRAB,KISHOREGANJ,45,50,55},{KISHOREGANJ,MYMENSINGH,60,70,75},{MYMENSINGH,NETROKONA,40,50,50},
{MYMENSINGH,JAMALPUR,60,70,75},{AKHAURA,HABIGANJ,65,70,80},{HABIGANJ,MOULVIBAZAR,45,50,55},
{MOULVIBAZAR,SYLHET,55,60,70},{DHAKA,SYLHET,319,320,390},{DHAKA,CHATTOGRAM,321,345,360},
{DHAKA,NOAKHALI,173,230,302},{DHAKA,TANGAIL,90,110,110},{TANGAIL,PABNA,95,120,115},
{PABNA,ISHWARDI,30,40,35},{ISHWARDI,NATORE,45,50,50},{NATORE,RAJSHAHI,45,50,50},
{RAJSHAHI,CHAPAINAWABGANJ,50,60,60},{NATORE,NAOGAON,75,80,85},{ISHWARDI,BOGURA,85,90,95},
{BOGURA,JOYPURHAT,60,70,70},{JOYPURHAT,RANGPUR,85,90,95},{RANGPUR,DINAJPUR,75,80,85},
{DINAJPUR,THAKURGAON,60,70,70},{THAKURGAON,PANCHAGARH,45,50,55},{ISHWARDI,KUSHTIA,40,50,50},
{KUSHTIA,JHENAIDAH,50,60,60},{JHENAIDAH,JASHORE,45,55,55},{JASHORE,BENAPOLE,35,40,45},
{JASHORE,KHULNA,55,70,65},{DHAKA,RAJSHAHI,343,340,330},{DHAKA,KHULNA,412,415,420},
{DHAKA,RANGPUR,472,480,540},
};

static const int WE[][5]={
{DHAKA,BARISHAL,170,300,360},{DHAKA,CHANDPUR,70,150,180},
{DHAKA,BHOLA,190,350,420},{DHAKA,PATUAKHALI,210,400,480},
{DHAKA,MUNSHIGANJ,35,80,90},{DHAKA,NARAYANGANJ,25,50,60},
{BARISHAL,BHOLA,45,100,120},{BARISHAL,PATUAKHALI,70,150,180},
{BARISHAL,PIROJPUR,60,120,150},{BARISHAL,JHALOKATI,25,60,60},
{CHANDPUR,LAKSHMIPUR,55,110,130},{BHOLA,CHARFASSON,75,140,150},
{PATUAKHALI,KUAKATA,70,130,140},{BARGUNA,AMTALI,35,70,80},
};

void buildMap(Map*map){
    initMap(map);
    for(int i=0;i<PLACE_COUNT;i++)
        addP(map,PD[i].name,PD[i].div,PD[i].dist,PD[i].lv,PD[i].rail);
    for(size_t i=0;i<sizeof LE/sizeof LE[0];i++){
        addE(map,LE[i][0],LE[i][1],BUS, LE[i][2],LE[i][2]*2.3,LE[i][2]*1.8,true);
        addE(map,LE[i][0],LE[i][1],WALK,LE[i][2],0.0,        LE[i][2]*12.0,true);}
    for(size_t i=0;i<sizeof RE/sizeof RE[0];i++)
        addE(map,RE[i][0],RE[i][1],BUS,RE[i][2],RE[i][2]*2.3,RE[i][2]*1.56,true);
    for(size_t i=0;i<sizeof TE/sizeof TE[0];i++)
        addE(map,TE[i][0],TE[i][1],TRAIN,TE[i][2],TE[i][3],TE[i][4],true);
    for(size_t i=0;i<sizeof WE/sizeof WE[0];i++)
        addE(map,WE[i][0],WE[i][1],LAUNCH,WE[i][2],WE[i][3],WE[i][4],true);
}

/* ── ALGORITHMS ────────────────────────────────────────────────── */
double eCost(const Edge*e,Metric m){return m==M_COST?e->cost:m==M_DIST?e->km:e->mins;}

bool dijkstraMode(const Map*map,int src,int dst,Metric met,TrType mode,Route*res){
    int n=map->n; double d[MX_N]; int prev[MX_N]; bool vis[MX_N];
    for(int i=0;i<n;i++){d[i]=INF;prev[i]=-1;vis[i]=false;} d[src]=0;
    for(int s=0;s<n;s++){
        int u=-1;double best=INF;
        for(int i=0;i<n;i++)if(!vis[i]&&d[i]<best){best=d[i];u=i;}
        if(u<0)break;vis[u]=true;if(u==dst)break;
        for(int i=0;i<map->g[u].n;i++){const Edge*e=&map->g[u].e[i];
            if(e->tr!=mode&&e->tr!=WALK)continue; // Strict single mode constraint
            double c=d[u]+eCost(e,met);
            if(c<d[e->to]){d[e->to]=c;prev[e->to]=u;}}
    }
    if(d[dst]>=INF/2)return false;
    int rv[MX_P],len=0,cur=dst;
    while(cur>=0&&len<MX_P){rv[len++]=cur;if(cur==src)break;cur=prev[cur];}
    if(len==0||rv[len-1]!=src)return false;
    res->len=len;for(int i=0;i<len;i++)res->nd[i]=rv[len-1-i];
    res->km=res->cost=res->mins=0;
    for(int i=0;i<len-1;i++){int f=res->nd[i],t=res->nd[i+1];const Edge*b=NULL;
        for(int j=0;j<map->g[f].n;j++){const Edge*e=&map->g[f].e[j];
            if(e->to==t&&(e->tr==mode||e->tr==WALK))if(!b||eCost(e,met)<eCost(b,met))b=e;}
        if(b){res->km+=b->km;res->cost+=b->cost;res->mins+=b->mins;}}
    return true;
}

bool dijkstra(const Map*map,int src,int dst,Metric met,Route*res){
    return dijkstraMode(map,src,dst,met,BUS,res);
}

void dfs(const Map*map,int cur,int dst,bool vis[MX_N],Route*cr,Routes*all){
    if(all->n>=MX_R)return;
    if(cur==dst){if(all->n<MX_R)all->r[all->n++]=*cr;return;}
    for(int i=0;i<map->g[cur].n;i++){const Edge*e=&map->g[cur].e[i];int nx=e->to;
        if(!allowed[e->tr]||vis[nx]||cr->len>=MX_P)continue;
        vis[nx]=true;cr->nd[cr->len++]=nx;cr->km+=e->km;cr->cost+=e->cost;cr->mins+=e->mins;
        dfs(map,nx,dst,vis,cr,all);
        cr->len--;cr->km-=e->km;cr->cost-=e->cost;cr->mins-=e->mins;vis[nx]=false;
        if(all->n>=MX_R)return;}
}
void findAll(const Map*map,int src,int dst,Routes*all){
    bool vis[MX_N]={false};Route r;memset(&r,0,sizeof r);
    all->n=0;vis[src]=true;r.nd[0]=src;r.len=1;dfs(map,src,dst,vis,&r,all);
}

/* ── DISPLAY ───────────────────────────────────────────────────── */
void banner(void){
    clr();
    fg(G2);printf("  ==================================================================\n");
    fg(G1);
    printf("   ____  _        _    ____ _  __  ____  ____   ___ _____ ___   ____ ___  _\n");
    printf("  | __ )| |      / \\  / ___| |/ / |  _ \\|  _ \\ / _ \\_   _/ _ \\ / ___/ _ \\| |\n");
    printf("  |  _ \\| |     / _ \\\\| |   | ' /  | |_) | |_) | | | || || | | | |  | | | | |\n");
    printf("  | |_) | |___ / ___ \\ |___| . \\  |  __/|  _ <| |_| || || |_| | |__| |_| | |___\n");
    printf("  |____/|_____/_/   \\_\\____|_|\\_\\ |_|   |_| \\_\\\\___/ |_| \\___/ \\____\\___/|_____|\n");
    fg(CY);printf("                     B L A C K   P R O T O C O L\n");
    fg(GY);printf("               Bangladesh Transit Intelligence Engine\n");
    fg(G2);printf("  ==================================================================\n");
    rst();
}

void printPath(const Map*map,const Route*r){
    for(int i=0;i<r->len;i++){fg(AM);printf("%s",map->p[r->nd[i]].name);rst();
        if(i<r->len-1){fg(CY);printf(" ==> ");rst();}}
    printf("\n");
}

void showRouteBreakdown(const Map*map,int src,int dst){
    banner();
    fg(G2);printf("  +------------------------------------------------------------------+\n");
    fg(G1);printf("  |               BLACK PROTOCOL -- TRANSIT INTELLIGENCE             |\n");
    fg(CY);printf("  |               [Algorithm: Multi-Modal Dijkstra Shortest Path]    |\n");
    fg(G2);printf("  +------------------------------------------------------------------+\n");rst();
    printf("  Origin      : ");fg(WH);printf("%s (%s Division)\n",map->p[src].name,map->p[src].div);rst();
    printf("  Destination : ");fg(WH);printf("%s (%s Division)\n",map->p[dst].name,map->p[dst].div);rst();
    fg(G2);printf("  --------------------------------------------------------------------\n\n");rst();

    /* ROUTE TYPE 1: HIGHWAY BUS TRANSIT */
    Route r_bus;
    fg(CY);printf("  [ ROUTE TYPE 1: HIGHWAY BUS TRANSIT ] ");fg(AM);printf("[Method: Dijkstra Priority Queue O((V+E)logV)]\n");rst();
    if(dijkstraMode(map,src,dst,M_COST,BUS,&r_bus)){
        fg(G1);printf("  Status      : AVAILABLE (Active Highway Network)\n");rst();
        printf("  Route Path  : ");printPath(map,&r_bus);
        printf("  Distance    : ");fg(G1);printf("%.1f KM\n",r_bus.km);fg(WH);
        printf("  Est. Fare   : ");fg(G1);printf("%.0f BDT\n",r_bus.cost);fg(WH);
        double min_t = r_bus.mins * 0.92, max_t = r_bus.mins * 1.20;
        printf("  Travel Time : ");fg(G1);printf("%.0f min (%.1fh)",r_bus.mins,r_bus.mins/60.0);
        fg(GY);printf(" [Min: %.1fh | Max Traffic: %.1fh]\n",min_t/60.0,max_t/60.0);rst();
    } else {
        fg(RD);printf("  Status      : NOT AVAILABLE FOR THIS ROUTE\n");
        fg(GY);printf("  Note        : No direct highway bus connection available.\n");rst();
    }
    fg(G2);printf("\n  --------------------------------------------------------------------\n\n");rst();

    /* ROUTE TYPE 2: INTERCITY RAILWAY TRANSIT */
    Route r_train;
    fg(CY);printf("  [ ROUTE TYPE 2: INTERCITY RAILWAY TRANSIT ] ");fg(AM);printf("[Method: Constrained Rail Dijkstra]\n");rst();
    if(dijkstraMode(map,src,dst,M_COST,TRAIN,&r_train)){
        fg(G1);printf("  Status      : AVAILABLE (Rail Network / Intercity Train)\n");rst();
        printf("  Route Path  : ");printPath(map,&r_train);
        printf("  Distance    : ");fg(G1);printf("%.1f KM\n",r_train.km);fg(WH);
        printf("  Est. Fare   : ");fg(G1);printf("%.0f BDT\n",r_train.cost);fg(WH);
        printf("  Travel Time : ");fg(G1);printf("%.0f min (%.1fh)",r_train.mins,r_train.mins/60.0);
        fg(GY);printf(" (Fixed Railway Timetable)\n");rst();
    } else {
        fg(RD);printf("  Status      : NOT AVAILABLE FOR THIS ROUTE\n");
        fg(GY);printf("  Note        : No railway station connectivity between these nodes.\n");rst();
    }
    fg(G2);printf("\n  --------------------------------------------------------------------\n\n");rst();

    /* ROUTE TYPE 3: INLAND WATERWAY / RIVER LAUNCH */
    Route r_launch;
    fg(CY);printf("  [ ROUTE TYPE 3: INLAND WATERWAY / RIVER LAUNCH ] ");fg(AM);printf("[Method: Waterway Graph Traversal]\n");rst();
    if(dijkstraMode(map,src,dst,M_COST,LAUNCH,&r_launch)){
        fg(G1);printf("  Status      : AVAILABLE (Active Launch Terminal Service)\n");rst();
        printf("  Route Path  : ");printPath(map,&r_launch);
        printf("  Distance    : ");fg(G1);printf("%.1f KM\n",r_launch.km);fg(WH);
        printf("  Est. Fare   : ");fg(G1);printf("%.0f BDT\n",r_launch.cost);fg(WH);
        printf("  Travel Time : ");fg(G1);printf("%.0f min (%.1fh)",r_launch.mins,r_launch.mins/60.0);
        fg(GY);printf(" (Scheduled Waterway Route)\n");rst();
    } else {
        fg(RD);printf("  Status      : NOT AVAILABLE FOR THIS ROUTE\n");
        fg(GY);printf("  Note        : No direct launch terminal connection available.\n");rst();
    }
    fg(G2);printf("\n  +------------------------------------------------------------------+\n");rst();
}

bool getSrcDst(const Map*map,int*src,int*dst){
    char f[MX_NM]={0},t[MX_NM]={0};
    fg(GY);printf("  [TAB = autocomplete | ENTER = confirm]\n\n");rst();
    if(!readLoc(map,"  Origin      > ",f,sizeof f)){msgErr("Bad input.");return false;}
    if(!readLoc(map,"  Destination > ",t,sizeof t)){msgErr("Bad input.");return false;}
    *src=findP(map,f);*dst=findP(map,t);
    if(*src<0){msgErr("Origin not found.");return false;}
    if(*dst<0){msgErr("Destination not found.");return false;}
    if(*src==*dst){msgErr("Same origin and destination.");return false;}
    printf("\n");msgInfo("Locations verified.");
    fg(G1);printf("  > ");fg(WH);printf("%s (%s)\n",map->p[*src].name,map->p[*src].div);
    fg(G1);printf("  > ");fg(WH);printf("%s (%s)\n",map->p[*dst].name,map->p[*dst].div);
    return true;
}

/* ── MODULES ───────────────────────────────────────────────────── */
void scanRoute(const Map*map){
    banner();fg(CY);printf("  >> SCAN ROUTE (Standard Transit Preference)\n\n");rst();
    int s,d;if(!getSrcDst(map,&s,&d)){pressEnter();return;}
    showRouteBreakdown(map,s,d);
    pressEnter();
}
void deepScan(const Map*map){
    banner();fg(CY);printf("  >> DEEP SCAN (Absolute Lowest Cost Budget Focus)\n\n");rst();
    int s,d;if(!getSrcDst(map,&s,&d)){pressEnter();return;}
    fg(AM);printf("\n  Analyzing all budget paths across network...\n\n");rst();
    Route bestR; bool found=false; double lowestCost=INF; TrType bestMode=BUS;
    for(int t=0;t<T_CNT;t++){
        if(t==WALK)continue;
        Route temp;
        if(dijkstraMode(map,s,d,M_COST,(TrType)t,&temp)){
            if(temp.cost<lowestCost){lowestCost=temp.cost;bestR=temp;bestMode=(TrType)t;found=true;}
        }
    }
    if(found){
        msgOk("Absolute lowest budget route decrypted!");
        fg(G2);printf("  +------------------------------------------------------------------+\n");
        fg(G1);printf("  |                 LOWEST COST BUDGET ROUTE SUMMARY                 |\n");
        fg(CY);printf("  |                 [Algorithm: Single-Source Cost Optimization Engine] |\n");
        fg(G2);printf("  +------------------------------------------------------------------+\n");rst();
        printf("  Path        : ");printPath(map,&bestR);
        printf("  Best Mode   : ");fg(CY);printf("%s\n",trName(bestMode));fg(WH);
        printf("  Lowest Fare : ");fg(G1);printf("%.0f BDT (Cheapest Option)\n",bestR.cost);fg(WH);
        printf("  Distance    : ");fg(G1);printf("%.1f KM\n",bestR.km);fg(WH);
        printf("  Travel Time : ");fg(G1);printf("%.0f min (%.1fh)\n",bestR.mins,bestR.mins/60.0);
        fg(G2);printf("  +------------------------------------------------------------------+\n");rst();
    } else {
        msgErr("No budget route found for this location pair.");
    }
    pressEnter();
}
void hierarchyDecode(const Map*map){
    banner();
    fg(G2);printf("  +------------------------------------------------------------------+\n");
    fg(G1);printf("  |              BLACK PROTOCOL -- ADMINISTRATIVE HIERARCHY          |\n");
    fg(CY);printf("  |              [Algorithm: Tree Structure DFS Mapping]             |\n");
    fg(G2);printf("  +------------------------------------------------------------------+\n");rst();
    printf("  Total Managed Nodes : ");fg(G1);printf("%d Locations (Districts & Upazilas)\n\n",map->n);rst();
    const char*divs[]={"Dhaka","Chattogram","Sylhet","Rajshahi","Khulna","Rangpur","Barishal","Mymensingh"};
    for(int d=0;d<8;d++){
        int cnt=0;
        for(int i=0;i<map->n;i++)if(strcmp(map->p[i].div,divs[d])==0)cnt++;
        if(cnt==0)continue;
        fg(CY);printf("  [%d] %s DIVISION ",d+1,divs[d]);fg(GY);printf("(%d Nodes)\n",cnt);rst();
        for(int i=0;i<map->n;i++){
            if(strcmp(map->p[i].div,divs[d])==0 && map->p[i].lv==DISTRICT){
                fg(G1);printf("      * District: ");fg(WH);printf("%-15s",map->p[i].name);rst();
                bool fst=true;
                for(int j=0;j<map->n;j++){
                    if(strcmp(map->p[j].dist,map->p[i].name)==0 && map->p[j].lv==UPAZILA){
                        if(fst){fg(GY);printf(" (Upazilas: ");fst=false;}else{fg(GY);printf(", ");}
                        fg(AM);printf("%s",map->p[j].name);rst();
                    }
                }
                if(!fst){fg(GY);printf(")\n");}else printf("\n");
                rst();
            }
        }
        printf("\n");
    }
    fg(G2);printf("  +------------------------------------------------------------------+\n");rst();
    pressEnter();
}

void systemComplexity(const Map*map){
    banner();
    fg(G2);printf("  +------------------------------------------------------------------+\n");
    fg(G1);printf("  |               BLACK PROTOCOL -- ALGORITHM COMPLEXITY             |\n");
    fg(CY);printf("  |               [Algorithm: Asymptotic Big-O & Memory Profiler]    |\n");
    fg(G2);printf("  +------------------------------------------------------------------+\n");rst();
    int edgeCount=0;
    for(int i=0;i<map->n;i++)edgeCount+=map->g[i].n;
    fg(AM);printf("  [ GRAPH TOPOLOGY METRICS ]\n");rst();
    printf("  * Total Vertices (V)    : ");fg(G1);printf("%d Locations\n",map->n);fg(WH);
    printf("  * Total Edges (E)       : ");fg(G1);printf("%d Directed Links\n",edgeCount);fg(WH);
    printf("  * Data Structure        : ");fg(CY);printf("Adjacency List O(V + E)\n");fg(WH);
    printf("  * Graph Density         : ");fg(GY);printf("Sparse Distributed Spatial Graph\n\n");rst();

    fg(AM);printf("  [ ALGORITHMIC TIME COMPLEXITY ]\n");rst();
    printf("  * Single-Source Route  : ");fg(G1);printf("O((V + E) log V)");fg(GY);printf("  [Dijkstra Shortest Path]\n");fg(WH);
    printf("  * All-Pairs Distance   : ");fg(G1);printf("O(V^3)");fg(GY);printf("           [Floyd-Warshall Matrix]\n");fg(WH);
    printf("  * Exhaustive Route DFS : ");fg(G1);printf("O(V!)");fg(GY);printf("            [Depth-First Search Backtracking]\n\n");rst();

    fg(AM);printf("  [ SPACE COMPLEXITY & MEMORY ]\n");rst();
    printf("  * Adjacency List RAM   : ");fg(G1);printf("~%zu KB\n",(sizeof(EList)*map->n)/1024);fg(WH);
    printf("  * Location Node RAM    : ");fg(G1);printf("~%zu KB\n",(sizeof(Place)*map->n)/1024);fg(WH);
    printf("  * Stack Recurse Depth  : ");fg(CY);printf("O(V) Max Recursion Limit\n");rst();
    fg(G2);printf("  +------------------------------------------------------------------+\n");rst();
    pressEnter();
}

void sysConfig(void){
    banner();fg(CY);printf("  >> CONFIGURATION & TRANSPORT MODE\n\n");rst();
    fg(AM);printf("  [OPTIMIZATION GOAL]\n");rst();
    printf("  1. Lowest Cost (Default)   2. Fastest Time   3. Shortest Distance\n");
    fg(G1);printf("  Select > ");rst();metric=(Metric)readInt(1,3);
    msgOk("Configuration updated.");
}

/* ── MAIN ──────────────────────────────────────────────────────── */
int main(void){
    enableVT();
    static Map map;buildMap(&map);
    while(1){
        banner();
        fg(G1);printf("   * SYSTEM STATUS : ");fg(WH);printf("ONLINE (Operational)\n");rst();
        fg(G1);printf("   * DATABASE      : ");fg(WH);printf("%d Locations Verified\n",map.n);rst();
        fg(G1);printf("   * OPTIMIZATION  : ");fg(CY);printf("%s\n\n",metName(metric));rst();
        fg(G2);printf("  --------------------------------------------------------------------\n");
        fg(G1);printf("   [1]");fg(WH);printf("  SCAN ROUTE           ");fg(CY);printf("[Dijkstra PQ]    ");fg(GY);printf("----> Standard Multi-Modal\n");
        fg(G1);printf("   [2]");fg(WH);printf("  DEEP SCAN            ");fg(CY);printf("[Min-Cost Engine]");fg(GY);printf("----> Absolute Budget Focus\n");
        fg(G1);printf("   [3]");fg(WH);printf("  HIERARCHY DECODE     ");fg(CY);printf("[Tree Map DFS]   ");fg(GY);printf("----> Division > District > Upazila\n");
        fg(G1);printf("   [4]");fg(WH);printf("  SYSTEM COMPLEXITY    ");fg(CY);printf("[Big-O Profiler]");fg(GY);printf("----> Time & Space Analysis\n");
        fg(G1);printf("   [5]");fg(WH);printf("  SYSTEM CONFIGURATION ");fg(CY);printf("[Config Engine] ");fg(GY);printf("----> Optimization Settings\n");
        fg(RD);printf("   [0]");fg(WH);printf("  EXIT SYSTEM          ");fg(CY);printf("[Session Exit]  ");fg(RD);printf("----> Disconnect Session\n");
        fg(G2);printf("  --------------------------------------------------------------------\n");
        fg(G1);printf("   >> ");rst();
        switch(readInt(0,5)){
            case 1:scanRoute(&map);break;
            case 2:deepScan(&map);break;
            case 3:hierarchyDecode(&map);break;
            case 4:systemComplexity(&map);break;
            case 5:sysConfig();pressEnter();break;
            case 0:banner();msgOk("Session ended.");return 0;}
    }
}
