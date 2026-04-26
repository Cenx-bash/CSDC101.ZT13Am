// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║   PHANTOM  —  Terminal Music Player  v3.2  (Linux, C++17)               ║
// ║   Fixed: elapsed display, status padding, ANSI-aware width,             ║
// ║           playlist row count, waveBar underflow, boxSide ANSI          ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <functional>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>

// ═══════════════════════════════════════════════════════════════
//  ANSI — strict monotone greyscale palette
// ═══════════════════════════════════════════════════════════════
namespace A {
    static const char* R    = "\033[0m";
    static const char* DIM  = "\033[2m";
    static const char* W    = "\033[97m";
    static const char* LG   = "\033[37m";
    static const char* MG   = "\033[90m";
    static const char* BW   = "\033[1;97m";
    static const char* DG   = "\033[2;37m";
    static const char* INV  = "\033[30;47m";
    static const char* HC   = "\033[?25l";
    static const char* SC   = "\033[?25h";
    static const char* CLS  = "\033[2J\033[H";
    static inline std::string go(int r, int c) {
        return "\033[" + std::to_string(r) + ";" + std::to_string(c) + "H";
    }
    static inline std::string clrEOL() { return "\033[K"; }
    static inline std::string clrLine() { return "\033[2K\r"; }
}

// ═══════════════════════════════════════════════════════════════
//  RawMode RAII
// ═══════════════════════════════════════════════════════════════
class RawMode {
public:
    RawMode() {
        tcgetattr(STDIN_FILENO, &saved_);
        struct termios t = saved_;
        t.c_iflag &= ~(BRKINT|ICRNL|INPCK|ISTRIP|IXON);
        t.c_oflag &= ~OPOST;
        t.c_cflag |=  CS8;
        t.c_lflag &= ~(ECHO|ICANON|IEXTEN|ISIG);
        t.c_cc[VMIN]  = 0;
        t.c_cc[VTIME] = 1;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
        std::cout << A::HC << std::flush;
        active_ = true;
    }
    ~RawMode() { restore(); }
    void restore() {
        if (!active_) return;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_);
        std::cout << A::SC << std::flush;
        active_ = false;
    }
    const struct termios& saved() const { return saved_; }
private:
    struct termios saved_{};
    bool active_ = false;
};
static RawMode* gRaw = nullptr;

// ═══════════════════════════════════════════════════════════════
//  Terminal helpers
// ═══════════════════════════════════════════════════════════════
static int tCols() { struct winsize w{}; ioctl(1,TIOCGWINSZ,&w); return w.ws_col>40?w.ws_col:80; }
static int tRows() { struct winsize w{}; ioctl(1,TIOCGWINSZ,&w); return w.ws_row>10?w.ws_row:24; }
static void scls() { const char* s="\033[2J\033[H"; ssize_t x=write(1,s,7);(void)x; }

// ═══════════════════════════════════════════════════════════════
//  Input
// ═══════════════════════════════════════════════════════════════
static int readKey() { unsigned char c=0; return (read(0,&c,1)==1)?(int)c:0; }

static std::string readLine(const std::string& defaultVal = "") {
    if (!gRaw) {
        std::string s;
        std::getline(std::cin,s);
        return s.empty()?defaultVal:s;
    }
    struct termios t = gRaw->saved();
    t.c_lflag |= (ECHO|ICANON);
    t.c_oflag |= OPOST;
    tcsetattr(0, TCSAFLUSH, &t);
    std::cout << A::SC << std::flush;
    std::string s;
    std::getline(std::cin, s);
    struct termios r = gRaw->saved();
    r.c_iflag &= ~(BRKINT|ICRNL|INPCK|ISTRIP|IXON);
    r.c_oflag &= ~OPOST;
    r.c_cflag |= CS8;
    r.c_lflag &= ~(ECHO|ICANON|IEXTEN|ISIG);
    r.c_cc[VMIN]=0;
    r.c_cc[VTIME]=1;
    tcsetattr(0, TCSAFLUSH, &r);
    std::cout << A::HC << std::flush;
    while (!s.empty()&&std::isspace((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty()&&std::isspace((unsigned char)s.back()))  s.pop_back();
    return s.empty() ? defaultVal : s;
}

// ═══════════════════════════════════════════════════════════════
//  String helpers
// ═══════════════════════════════════════════════════════════════
static std::string lo(const std::string& s) {
    std::string r=s; for(char& c:r) c=(char)std::tolower((unsigned char)c); return r;
}
static std::string trimS(const std::string& s) {
    const char* w=" \t\r\n";
    size_t a=s.find_first_not_of(w); if(a==std::string::npos) return "";
    size_t b=s.find_last_not_of(w); return s.substr(a,b-a+1);
}

// FIX 1: ANSI-aware visible length — strips escape sequences before measuring
static int visLen(const std::string& s) {
    int len = 0;
    bool inEsc = false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\033') { inEsc = true; continue; }
        if (inEsc) { if (std::isalpha((unsigned char)s[i])) inEsc = false; continue; }
        ++len;
    }
    return len;
}

// FIX 2: padR/padL now use visLen so ANSI codes don't corrupt column widths
static std::string padR(const std::string& s, int w) {
    if (w <= 0) return "";
    int vl = visLen(s);
    if (vl > w) {
        // Truncate at visible boundary, appending "..."
        std::string out;
        int vis = 0;
        bool inEsc = false;
        for (size_t i = 0; i < s.size() && vis < w-3; ++i) {
            if (s[i] == '\033') { inEsc = true; out += s[i]; continue; }
            if (inEsc) { out += s[i]; if (std::isalpha((unsigned char)s[i])) inEsc = false; continue; }
            out += s[i]; ++vis;
        }
        out += "...";
        vis += 3;
        if (vis < w) out += std::string((size_t)(w - vis), ' ');
        return out;
    }
    return s + std::string((size_t)(w - vl), ' ');
}

static std::string padL(const std::string& s, int w) {
    if (w <= 0) return "";
    int vl = visLen(s);
    if (vl > w) {
        // Just truncate, no ellipsis for left-pad
        std::string out;
        int vis = 0;
        bool inEsc = false;
        for (size_t i = 0; i < s.size() && vis < w; ++i) {
            if (s[i] == '\033') { inEsc = true; out += s[i]; continue; }
            if (inEsc) { out += s[i]; if (std::isalpha((unsigned char)s[i])) inEsc = false; continue; }
            out += s[i]; ++vis;
        }
        return out;
    }
    return std::string((size_t)(w - vl), ' ') + s;
}

static std::string fmtTime(int sec) {
    if(sec<0) sec=0;
    char b[16];
    std::snprintf(b,sizeof(b),"%02d:%02d",sec/60,sec%60);
    return b;
}
static std::string fmtTimeLong(int sec) {
    char b[32];
    std::snprintf(b,sizeof(b),"%dh %02dm %02ds",sec/3600,(sec%3600)/60,sec%60);
    return b;
}
static int parseSecs(const std::string& dur) {
    int m=0,s=0;
    std::sscanf(dur.c_str(),"%d:%d",&m,&s);
    return m*60+s;
}

static std::string rep(const std::string& s, int n) {
    std::string r;
    if (n <= 0) return r;
    r.reserve(s.size()*(size_t)n);
    for(int i=0;i<n;i++) r+=s;
    return r;
}

// ═══════════════════════════════════════════════════════════════
//  Visual primitives
// ═══════════════════════════════════════════════════════════════

// FIX 3: waveBar — guard width <= 0 (was passing unchecked int as size_t)
static std::string waveBar(int tick, int width, bool playing) {
    if (width <= 0) return "";
    if (!playing) {
        return std::string((size_t)width, '-');
    }
    std::string bar;
    bar.reserve((size_t)width * 3);
    const char* blocks[] = {" ","▁","▂","▃","▄","▅","▆","▇","█"};
    for (int i = 0; i < width; i++) {
        float phase = (float)(tick*3 + i*7) * 0.15f;
        float v = (std::sin(phase) + std::sin(phase*1.7f+1.0f) + std::sin(phase*3.1f)) / 3.0f;
        v = (v + 1.0f) * 0.5f;
        int idx = (int)(v * 8);
        if(idx>8) idx=8;
        if(idx<0) idx=0;
        bar += blocks[idx];
    }
    return bar;
}

static std::string pbar(float fill, int width) {
    if(width<2) return "";
    fill = std::max(0.0f, std::min(1.0f, fill));
    int filled = (int)(fill * width);
    std::string bar;
    for(int i=0;i<width;i++) {
        if(i<filled)       bar+="█";
        else if(i==filled) bar+="▌";
        else               bar+="░";
    }
    return bar;
}

static std::string vbar(int vol, int w) {
    if(w<=0) return "";
    int f=(int)((float)vol/100.0f*(float)w);
    std::string b;
    for(int i=0;i<w;i++) b+=(i<f?"█":"░");
    return b;
}

static std::string starStr(int rating) {
    std::string s;
    for(int i=1;i<=5;i++) s += (i<=rating ? "★" : "☆");
    return s;
}

static std::string boxTop(int w)  { return "╔"+rep("═",w-2)+"╗"; }
static std::string boxBot(int w)  { return "╚"+rep("═",w-2)+"╝"; }

// FIX 4: boxSide — use visLen so embedded ANSI codes don't break column width
static std::string boxSide(const std::string& content, int w) {
    // Available visible chars inside: w-4 (║ <space> ... <space> ║)
    int avail = w - 4;
    int vl = visLen(content);
    std::string inner;
    if (vl <= avail) {
        inner = content + std::string((size_t)(avail - vl), ' ');
    } else {
        // Truncate at visible boundary
        int vis = 0;
        bool inEsc = false;
        for (size_t i = 0; i < content.size() && vis < avail; ++i) {
            if (content[i] == '\033') { inEsc = true; inner += content[i]; continue; }
            if (inEsc) { inner += content[i]; if(std::isalpha((unsigned char)content[i])) inEsc=false; continue; }
            inner += content[i]; ++vis;
        }
        inner += A::R; // reset any open escape
    }
    return std::string("║ ") + inner + " ║";
}

// ═══════════════════════════════════════════════════════════════
//  Song model
// ═══════════════════════════════════════════════════════════════
struct Song {
    std::string title;
    std::string artist;
    std::string genre;
    int         year       = 0;
    std::string duration;
    int         playCount  = 0;
    int         rating     = 0;
    std::string tags;
    std::string addedDate;

    int toSecs()    const { return parseSecs(duration); }
    bool valid()    const { return !title.empty() && !artist.empty(); }
};

// ═══════════════════════════════════════════════════════════════
//  Node — Circular Doubly Linked List
// ═══════════════════════════════════════════════════════════════
struct Node {
    Song  song;
    Node* prev = nullptr;
    Node* next = nullptr;
    explicit Node(const Song& s) : song(s) {}
};

enum class RepeatMode { NONE, ONE, ALL };
static const char* rLabel(RepeatMode m) {
    switch(m){case RepeatMode::NONE:return"OFF";case RepeatMode::ONE:return" 1 ";default:return"ALL";}
}

struct Bookmark {
    std::string songTitle;
    std::string songArtist;
    int         positionSec = 0;
    std::string label;
};

struct HistEntry {
    std::string title;
    std::string artist;
    std::string playedAt;
    int         listenSec = 0;
};

// ═══════════════════════════════════════════════════════════════
//  MusicPlayer — all data and logic
// ═══════════════════════════════════════════════════════════════
class MusicPlayer {
public:
    enum class SortKey { TITLE, ARTIST, GENRE, YEAR, PLAYS, RATING, DURATION };

    MusicPlayer() { srand((unsigned)time(nullptr)); updateDate(); }
    ~MusicPlayer() { freeList(); }

    void loadCSV(const std::string& file) {
        csvFile_ = file;
        std::ifstream f(file);
        if(!f.is_open()) return;
        std::string line; int dupes=0;
        while(std::getline(f,line)){
            if(line.empty()) continue;
            Song s = parseLine(line);
            if(!s.valid()) continue;
            if(!insertBack(s,true)) ++dupes;
        }
        dupesOnLoad_ = dupes;
    }

    void saveCSV() const { saveCSV(csvFile_); }
    void saveCSV(const std::string& file) const {
        if(file.empty()) return;
        std::ofstream f(file, std::ios::trunc);
        if(!f.is_open()) return;
        Node* n=head_; if(!n) return;
        do {
            f << ce(n->song.title)    <<','<< ce(n->song.artist)  <<','
              << ce(n->song.genre)    <<','<< n->song.year         <<','
              << ce(n->song.duration) <<','<< n->song.playCount    <<','
              << n->song.rating       <<','<< ce(n->song.tags)     <<','
              << ce(n->song.addedDate)<<'\n';
            n=n->next;
        } while(n!=head_);
    }

    void saveBookmarks(const std::string& file) const {
        std::ofstream f(file, std::ios::trunc);
        for(const auto& b : bookmarks_)
            f<<ce(b.songTitle)<<','<<ce(b.songArtist)<<','<<b.positionSec<<','<<ce(b.label)<<'\n';
    }

    void loadBookmarks(const std::string& file) {
        std::ifstream f(file);
        if(!f.is_open()) return;
        std::string line;
        while(std::getline(f,line)){
            if(line.empty()) continue;
            auto flds = splitCSV(line);
            if(flds.size()<4) continue;
            Bookmark b;
            b.songTitle=flds[0];
            b.songArtist=flds[1];
            try{b.positionSec=std::stoi(flds[2]);}catch(...){}
            b.label=flds[3];
            bookmarks_.push_back(b);
        }
    }

    bool insertBack(const Song& s, bool silent=false) {
        if(isDupe(s)){
            if(!silent) lastMsg_="[!] Duplicate: "+s.title+" - "+s.artist;
            return false;
        }
        Node* nd=new Node(s);
        if(!head_){ head_=cur_=nd; nd->next=nd->prev=nd; }
        else {
            Node* tail=head_->prev;
            tail->next=nd; nd->prev=tail; nd->next=head_; head_->prev=nd;
        }
        ++size_;
        return true;
    }

    bool deleteCurrent() {
        if(!cur_) return false;
        Node* v=cur_;
        if(size_==1){ delete v; head_=cur_=nullptr; size_=0; return true; }
        cur_=v->next;
        if(v==head_) head_=v->next;
        v->prev->next=v->next;
        v->next->prev=v->prev;
        delete v; --size_; elapsedMs_=0; sessionListenSec_=0;
        return true;
    }

    void next(bool autoplay=false) {
        if(!cur_) return;
        commitListen(); pushHistory();
        cur_=cur_->next; elapsedMs_=0;
        if(!autoplay) playing_=true;
    }

    void prev() {
        if(!cur_) return;
        commitListen();
        if(elapsedMs_>3000){ elapsedMs_=0; return; }
        if(!histQ_.empty()){
            HistEntry h=histQ_.back(); histQ_.pop_back();
            Node* n=findNode(h.title,h.artist);
            if(n){ cur_=n; elapsedMs_=0; playing_=true; return; }
        }
        cur_=cur_->prev; elapsedMs_=0; playing_=true;
    }

    void jumpRandom() {
        if(!head_||size_<2) return;
        commitListen(); pushHistory();
        int skip=rand()%(size_-1)+1;
        Node* n=cur_?cur_:head_;
        for(int i=0;i<skip;i++) n=n->next;
        cur_=n; elapsedMs_=0; playing_=true;
    }

    void seekSec(int delta) {
        if(!cur_) return;
        elapsedMs_ += delta*1000;
        int maxMs = cur_->song.toSecs()*1000;
        if(maxMs<=0) maxMs=30000;
        if(elapsedMs_<0) elapsedMs_=0;
        if(elapsedMs_>maxMs) elapsedMs_=maxMs-1000;
    }

    void warpTo(float pct) {
        if(!cur_) return;
        int maxMs = cur_->song.toSecs()*1000;
        if(maxMs<=0) maxMs=30000;
        pct = std::max(0.0f,std::min(1.0f,pct));
        elapsedMs_ = (int)(pct*(float)maxMs);
    }

    void advance(int ms=100) {
        if(!playing_||!cur_) return;
        elapsedMs_+=ms; sessionListenMs_+=ms; tick_++;
        int total=cur_->song.toSecs()*1000;
        if(total<=0) total=30000;
        if(elapsedMs_>=total){
            cur_->song.playCount++;
            totalListenMs_ += total;
            commitListen(); pushHistory();
            switch(repeat_){
                case RepeatMode::ONE: elapsedMs_=0; break;
                case RepeatMode::ALL:
                    cur_=cur_->next; elapsedMs_=0; break;
                case RepeatMode::NONE:
                    cur_=cur_->next; elapsedMs_=0;
                    if(cur_==head_) playing_=false;
                    break;
            }
        }
    }

    void shuffle() {
        if(size_<2) return;
        std::vector<Song> arr; arr.reserve(size_);
        Node* n=head_; do{ arr.push_back(n->song); n=n->next; }while(n!=head_);
        for(int i=(int)arr.size()-1;i>0;i--){ int j=rand()%(i+1); std::swap(arr[i],arr[j]); }
        std::string ct=cur_?cur_->song.title:"";
        std::string ca=cur_?cur_->song.artist:"";
        freeList();
        for(const auto& s:arr) insertBack(s,true);
        if(!ct.empty()){ Node* nc=findNode(ct,ca); if(nc) cur_=nc; }
        lastMsg_="Shuffled "+std::to_string(size_)+" songs.";
    }

    void sortBy(SortKey key, bool descending=false) {
        if(size_<2) return;
        std::vector<Song> arr; arr.reserve(size_);
        Node* n=head_; do{ arr.push_back(n->song); n=n->next; }while(n!=head_);
        auto cmp = [&](const Song& a, const Song& b) -> bool {
            bool r = false;
            switch(key){
                case SortKey::TITLE:    r=(lo(a.title)<lo(b.title)); break;
                case SortKey::ARTIST:   r=(lo(a.artist)<lo(b.artist)); break;
                case SortKey::GENRE:    r=(lo(a.genre)<lo(b.genre)); break;
                case SortKey::YEAR:     r=(a.year<b.year); break;
                case SortKey::PLAYS:    r=(a.playCount>b.playCount); break;
                case SortKey::RATING:   r=(a.rating>b.rating); break;
                case SortKey::DURATION: r=(a.toSecs()<b.toSecs()); break;
            }
            return descending ? !r : r;
        };
        std::stable_sort(arr.begin(),arr.end(),cmp);
        std::string ct=cur_?cur_->song.title:"";
        std::string ca=cur_?cur_->song.artist:"";
        freeList();
        for(const auto& s:arr) insertBack(s,true);
        if(!ct.empty()){ Node* nc=findNode(ct,ca); if(nc) cur_=nc; }
    }

    void editField(const std::string& field, const std::string& val) {
        if(!cur_) return;
        if(field=="title")         cur_->song.title=val;
        else if(field=="artist")   cur_->song.artist=val;
        else if(field=="genre")    cur_->song.genre=val;
        else if(field=="duration") cur_->song.duration=val;
        else if(field=="tags")     cur_->song.tags=val;
        else if(field=="year")     { try{cur_->song.year=std::stoi(val);}catch(...){} }
        else if(field=="rating")   { try{int r=std::stoi(val);cur_->song.rating=std::max(0,std::min(5,r));}catch(...){} }
    }

    std::vector<Node*> search(const std::string& q) const {
        std::vector<Node*> res;
        if(!head_) return res;
        std::string ql=lo(q);
        Node* n=head_;
        do {
            if(lo(n->song.title).find(ql)!=std::string::npos ||
               lo(n->song.artist).find(ql)!=std::string::npos||
               lo(n->song.genre).find(ql)!=std::string::npos ||
               lo(n->song.tags).find(ql)!=std::string::npos)
                res.push_back(n);
            n=n->next;
        } while(n!=head_);
        return res;
    }

    std::vector<Node*> byGenre(const std::string& genre) const {
        std::vector<Node*> res;
        if(!head_) return res;
        std::string gl=lo(genre);
        Node* n=head_;
        do {
            if(lo(n->song.genre)==gl) res.push_back(n);
            n=n->next;
        } while(n!=head_);
        return res;
    }

    std::vector<std::string> genres() const {
        std::set<std::string> s;
        if(!head_) return {};
        Node* n=head_;
        do{ if(!n->song.genre.empty()) s.insert(n->song.genre); n=n->next; }while(n!=head_);
        return std::vector<std::string>(s.begin(),s.end());
    }

    void addBookmark(const std::string& label) {
        if(!cur_) return;
        Bookmark b;
        b.songTitle=cur_->song.title; b.songArtist=cur_->song.artist;
        b.positionSec=elapsedMs_/1000; b.label=label;
        bookmarks_.push_back(b);
        lastMsg_="Bookmark: "+label+" @ "+fmtTime(b.positionSec);
    }

    void jumpToBookmark(int idx) {
        if(idx<0||(size_t)idx>=bookmarks_.size()) return;
        const Bookmark& b=bookmarks_[idx];
        Node* n=findNode(b.songTitle,b.songArtist);
        if(n){ cur_=n; elapsedMs_=b.positionSec*1000; playing_=true; }
    }

    const std::vector<Bookmark>& bookmarks() const { return bookmarks_; }
    void removeBookmark(int idx) {
        if(idx>=0&&(size_t)idx<bookmarks_.size()) bookmarks_.erase(bookmarks_.begin()+idx);
    }

    void rateCurrent(int stars) {
        if(!cur_) return;
        cur_->song.rating=std::max(0,std::min(5,stars));
        lastMsg_="Rated: "+starStr(cur_->song.rating);
    }

    int  totalListenSec()  const { return (int)(totalListenMs_/1000); }
    int  sessionListenSec()const { return (int)(sessionListenMs_/1000); }
    int  tick()            const { return tick_; }
    int  tickVal()         const { return tick_; }

    std::string topArtist() const {
        std::map<std::string,int> m;
        if(!head_) return "-";
        Node* n=head_; do{ m[n->song.artist]+=n->song.playCount; n=n->next; }while(n!=head_);
        auto best=std::max_element(m.begin(),m.end(),[](const auto&a,const auto&b){return a.second<b.second;});
        return best!=m.end()?best->first:"-";
    }

    std::string topGenre() const {
        std::map<std::string,int> m;
        if(!head_) return "-";
        Node* n=head_; do{ m[n->song.genre]+=n->song.playCount; n=n->next; }while(n!=head_);
        auto best=std::max_element(m.begin(),m.end(),[](const auto&a,const auto&b){return a.second<b.second;});
        return best!=m.end()?best->first:"-";
    }

    int totalPlayCount() const {
        int t=0; if(!head_) return 0;
        Node* n=head_; do{ t+=n->song.playCount; n=n->next; }while(n!=head_); return t;
    }

    int totalDurationSec() const {
        int t=0; if(!head_) return 0;
        Node* n=head_; do{ t+=n->song.toSecs(); n=n->next; }while(n!=head_); return t;
    }

    const Song* mostPlayed() const {
        if(!head_) return nullptr;
        Node* best=head_; Node* n=head_->next;
        while(n!=head_){ if(n->song.playCount>best->song.playCount) best=n; n=n->next; }
        return &best->song;
    }

    const Song* highestRated() const {
        if(!head_) return nullptr;
        Node* best=head_; Node* n=head_->next;
        while(n!=head_){ if(n->song.rating>best->song.rating) best=n; n=n->next; }
        return best->song.rating>0?&best->song:nullptr;
    }

    const Song* currentSong()  const { return cur_?&cur_->song:nullptr; }
    Node*       currentNode()        { return cur_; }
    bool  isPlaying()          const { return playing_; }
    void  togglePlay()               { playing_=!playing_; }
    void  setPlaying(bool v)         { playing_=v; }
    int   elapsedSec()         const { return elapsedMs_/1000; }
    int   elapsedMs()          const { return elapsedMs_; }
    float progress()           const {
        if(!cur_) return 0.f;
        int t=cur_->song.toSecs();
        return t>0?std::min(1.0f,(float)elapsedMs_/((float)t*1000.f)):0.f;
    }
    int   volume()             const { return vol_; }
    void  volUp(int step=5)          { vol_=std::min(100,vol_+step); }
    void  volDown(int step=5)        { vol_=std::max(0,vol_-step); }
    RepeatMode repeat()        const { return repeat_; }
    void  cycleRepeat()              {
        repeat_=(repeat_==RepeatMode::NONE)?RepeatMode::ONE:
                (repeat_==RepeatMode::ONE)?RepeatMode::ALL:RepeatMode::NONE;
    }
    int   size()               const { return size_; }
    bool  empty()              const { return !head_; }
    Node* headNode()                 { return head_; }
    const Node* headNode()     const { return head_; }
    int   dupesOnLoad()        const { return dupesOnLoad_; }
    const std::deque<HistEntry>& fullHistory() const { return histQ_; }
    std::string lastMsg()      const { return lastMsg_; }
    void  clearMsg()                 { lastMsg_.clear(); }

    int currentIndex() const {
        if(!head_||!cur_) return -1;
        Node* n=head_; int i=0;
        do{ if(n==cur_) return i; n=n->next; ++i; }while(n!=head_);
        return -1;
    }
    std::string today() const { return today_; }

private:
    Node*      head_     = nullptr;
    Node*      cur_      = nullptr;
    int        size_     = 0;
    bool       playing_  = false;
    int        elapsedMs_  = 0;
    int        vol_      = 80;
    RepeatMode repeat_   = RepeatMode::NONE;
    std::string csvFile_;
    std::string lastMsg_;
    int        dupesOnLoad_ = 0;
    int        tick_     = 0;
    int64_t    sessionListenMs_ = 0;
    int64_t    totalListenMs_=0;
    int        sessionListenSec_=0;
    std::string today_;
    std::deque<HistEntry> histQ_;
    std::vector<Bookmark> bookmarks_;

    void updateDate() {
        time_t now=time(nullptr); struct tm* t=localtime(&now);
        char buf[16]; strftime(buf,sizeof(buf),"%Y-%m-%d",t); today_=buf;
    }

    void pushHistory() {
        if(!cur_) return;
        time_t now=time(nullptr); struct tm* t=localtime(&now);
        char buf[16]; strftime(buf,sizeof(buf),"%H:%M:%S",t);
        HistEntry e;
        e.title=cur_->song.title; e.artist=cur_->song.artist;
        e.playedAt=buf; e.listenSec=elapsedMs_/1000;
        if(histQ_.size()>=50) histQ_.pop_front();
        histQ_.push_back(e);
    }

    void commitListen() {}

    bool isDupe(const Song& s) const {
        if(!head_) return false;
        Node* n=head_;
        do{ if(lo(n->song.title)==lo(s.title)&&lo(n->song.artist)==lo(s.artist)) return true; n=n->next; }while(n!=head_);
        return false;
    }

    Node* findNode(const std::string& title, const std::string& artist) const {
        if(!head_) return nullptr;
        Node* n=head_;
        do{ if(lo(n->song.title)==lo(title)&&lo(n->song.artist)==lo(artist)) return n; n=n->next; }while(n!=head_);
        return nullptr;
    }

    void freeList() {
        if(!head_) return;
        Node* n=head_; Node* next;
        do{ next=n->next; delete n; n=next; }while(n!=head_);
        head_=cur_=nullptr; size_=0;
    }

    static Song parseLine(const std::string& line) {
        auto flds=splitCSV(line);
        Song s;
        if(flds.size()<5) return s;
        s.title=flds[0]; s.artist=flds[1]; s.genre=flds[2];
        try{s.year=std::stoi(flds[3]);}catch(...){}
        s.duration=flds[4];
        if(flds.size()>5){try{s.playCount=std::stoi(flds[5]);}catch(...){}}
        if(flds.size()>6){try{s.rating=std::stoi(flds[6]);}catch(...){}}
        if(flds.size()>7) s.tags=flds[7];
        if(flds.size()>8) s.addedDate=flds[8];
        return s;
    }

    static std::vector<std::string> splitCSV(const std::string& line) {
        std::vector<std::string> f; std::string field; bool inQ=false;
        for(char c:line){
            if(c=='"'){inQ=!inQ;continue;}
            if(c==','&&!inQ){f.push_back(trimS(field));field.clear();continue;}
            field+=c;
        }
        f.push_back(trimS(field)); return f;
    }

    static std::string ce(const std::string& v) {
        if(v.find(',')==std::string::npos&&v.find('"')==std::string::npos) return v;
        std::string r="\"";
        for(char c:v){if(c=='"')r+='"';r+=c;}
        return r+'"';
    }
};

// ═══════════════════════════════════════════════════════════════
//  UI — the full-screen TUI
// ═══════════════════════════════════════════════════════════════
class UI {
public:
    explicit UI(MusicPlayer& mp) : mp_(mp) {}

    void draw() {
        cols_ = tCols();
        rows_ = tRows();
        std::cout << A::CLS << std::flush;
        if(mode_==Mode::MAIN)      drawMain();
        else if(mode_==Mode::WARP) drawWarpMode();
        std::cout << std::flush;
    }

    void setStatus(const std::string& m) { statusMsg_ = m; statusAge_ = 0; }

    enum class Mode { MAIN, WARP };
    void setMode(Mode m) { mode_=m; }
    Mode mode() const { return mode_; }

    std::string& warpInput() { return warpInput_; }

    void commitWarp() {
        try {
            float pct = std::stof(warpInput_) / 100.0f;
            mp_.warpTo(pct);
            setStatus("Warped to "+warpInput_+"%");
        } catch(...) { setStatus("[!] Invalid position."); }
        warpInput_.clear(); mode_=Mode::MAIN;
    }

    void modalAdd() {
        scls(); modalHeader("ADD SONG");
        Song s; s.addedDate=mp_.today();
        s.title    = fp("  Title              ");
        if(s.title.empty()){setStatus("[!] Title required.");return;}
        s.artist   = fp("  Artist             ");
        if(s.artist.empty()){setStatus("[!] Artist required.");return;}
        s.genre    = fp("  Genre              ");
        std::string yr=fp("  Year               ");
        try{s.year=std::stoi(yr);}catch(...){}
        s.duration = fp("  Duration (MM:SS)    ");
        if(s.duration.empty()) s.duration="00:00";
        s.tags     = fp("  Tags (comma-sep)    ");
        std::string rat=fp("  Rating (0-5)        ");
        try{s.rating=std::max(0,std::min(5,(int)std::stoi(rat)));}catch(...){}
        if(mp_.insertBack(s)){
            setStatus("[+] Added: "+s.title+" - "+s.artist);
            mp_.saveCSV();
        } else { setStatus(mp_.lastMsg()); mp_.clearMsg(); }
    }

    void modalEdit() {
        const Song* s=mp_.currentSong();
        if(!s){setStatus("[!] No song.");return;}
        scls(); modalHeader("EDIT: "+s->title);
        auto ef=[&](const std::string& lbl, const std::string& cur, const std::string& key){
            std::cout<<A::MG<<"  "<<lbl<<A::LG<<cur<<A::R<<"\n";
            std::cout<<A::MG<<"  New (Enter=keep): "<<A::R;
            std::string v=readLine(cur);
            if(v!=cur) mp_.editField(key,v);
        };
        ef("Title    : ",s->title,              "title");
        ef("Artist   : ",s->artist,             "artist");
        ef("Genre    : ",s->genre,              "genre");
        ef("Year     : ",std::to_string(s->year),"year");
        ef("Duration : ",s->duration,           "duration");
        ef("Tags     : ",s->tags,               "tags");
        ef("Rating   : ",std::to_string(s->rating),"rating");
        setStatus("Song updated."); mp_.saveCSV();
    }

    void modalDelete() {
        const Song* s=mp_.currentSong();
        if(!s){setStatus("[!] No song.");return;}
        scls(); modalHeader("DELETE SONG");
        std::cout<<A::W<<"\n  "<<s->title<<" - "<<s->artist<<"\n"
                 <<A::MG<<"\n  Confirm? [y/N]: "<<A::R<<std::flush;
        std::string yn=readLine();
        if(!yn.empty()&&(yn[0]=='y'||yn[0]=='Y')){
            std::string t=s->title;
            mp_.deleteCurrent(); setStatus("[-] Deleted: "+t); mp_.saveCSV();
        } else setStatus("Cancelled.");
    }

    void modalSearch() {
        scls(); modalHeader("SEARCH");
        std::string q=fp("  Query             ");
        if(q.empty()) return;
        auto results=mp_.search(q);
        std::cout<<"\n";
        if(results.empty()){
            std::cout<<A::MG<<"  No results.\n"<<A::R;
        } else {
            line(cols_);
            std::cout<<A::MG<<"  "<<padR("TITLE",30)<<padR("ARTIST",22)<<"DUR   ★\n"<<A::R;
            line(cols_);
            for(Node* n:results){
                bool ic=(n==mp_.currentNode());
                std::string row="  "+padR(n->song.title,30)+padR(n->song.artist,22)
                               +padR(n->song.duration,6)+std::to_string(n->song.rating);
                if(ic) std::cout<<A::INV<<row<<A::R<<"\n";
                else   std::cout<<A::LG <<row<<A::R<<"\n";
            }
            line(cols_);
            std::cout<<A::MG<<"  "<<results.size()<<" result(s)\n"<<A::R;
        }
        waitPrompt();
    }

    void modalPlaylist() {
        int offset=std::max(0,mp_.currentIndex()-5);
        while(true){
            scls();
            int visible=rows_-12;
            if(visible<3) visible=3;
            int total=mp_.size(); int curIdx=mp_.currentIndex();
            if(offset>curIdx)          offset=std::max(0,curIdx);
            if(offset+visible<=curIdx) offset=curIdx-visible+1;
            offset=std::max(0,std::min(offset,std::max(0,total-visible)));
            modalHeader("PLAYLIST  [j/k scroll · ENTER jump · q back]");
            std::cout<<A::MG
                <<"  "<<padR("#",5)<<padR("TITLE",28)<<padR("ARTIST",20)
                <<padR("DUR",7)<<"★  PLAYS\n"<<A::R;
            line(cols_);
            if(mp_.empty()){
                std::cout<<A::MG<<"  Empty.\n"<<A::R;
            } else {
                Node* n=mp_.headNode(); int idx=0;
                do {
                    if(idx>=offset&&idx<offset+visible){
                        bool ic=(idx==curIdx);
                        std::string row="  "+padR(std::to_string(idx+1)+".",5)
                            +padR(n->song.title,28)+padR(n->song.artist,20)
                            +padR(n->song.duration,7)+std::to_string(n->song.rating)
                            +"  "+std::to_string(n->song.playCount);
                        if(ic) std::cout<<A::INV<<row<<A::R<<"\n";
                        else   std::cout<<A::LG <<row<<A::R<<"\n";
                    }
                    n=n->next; ++idx;
                } while(n!=mp_.headNode());
            }
            line(cols_);
            std::cout<<A::MG<<"  "<<total<<" songs  "
                     <<(offset+1)<<"-"<<std::min(offset+visible,total)<<"\n"<<A::R;
            int k=0; while(k==0) k=readKey();
            if(k=='q'||k=='Q'||k==27) break;
            else if(k=='k'||k=='K'){ if(offset>0)--offset; }
            else if(k=='j'||k=='J'){ if(offset+visible<total)++offset; }
            else if(k==27){
                int k2=readKey();
                if(k2=='['){
                    int k3=readKey();
                    if(k3=='A'&&offset>0)--offset;
                    else if(k3=='B'&&offset+visible<total)++offset;
                }
            }
        }
    }

    void modalHistory() {
        scls(); modalHeader("PLAYBACK HISTORY");
        const auto& h=mp_.fullHistory();
        if(h.empty()){
            std::cout<<A::MG<<"  No history.\n"<<A::R;
        } else {
            line(cols_);
            std::cout<<A::MG<<"  "<<padR("TIME",10)<<padR("TITLE",28)
                     <<padR("ARTIST",20)<<"LISTENED\n"<<A::R;
            line(cols_);
            int shown=0;
            for(auto it=h.rbegin();it!=h.rend()&&shown<30;++it,++shown){
                std::cout<<A::LG<<"  "<<padR(it->playedAt,10)
                         <<padR(it->title,28)<<padR(it->artist,20)
                         <<fmtTime(it->listenSec)<<A::R<<"\n";
            }
            line(cols_);
        }
        waitPrompt();
    }

    void modalStats() {
        scls(); modalHeader("STATISTICS DASHBOARD");
        std::cout<<"\n";
        auto row=[&](const std::string& k, const std::string& v){
            std::cout<<A::MG<<"  "<<padR(k,24)<<A::W<<v<<A::R<<"\n";
        };
        std::cout<<A::BW<<"  ── Library ──\n"<<A::R;
        row("Total songs",      std::to_string(mp_.size()));
        row("Total duration",   fmtTimeLong(mp_.totalDurationSec()));
        row("Total play count", std::to_string(mp_.totalPlayCount()));
        row("Top artist",       mp_.topArtist());
        row("Top genre",        mp_.topGenre());
        const Song* best=mp_.mostPlayed();
        if(best) row("Most played", best->title+" - "+best->artist+" ("+std::to_string(best->playCount)+")");
        const Song* rated=mp_.highestRated();
        if(rated) row("Highest rated", rated->title+" - "+starStr(rated->rating));
        std::cout<<"\n";
        std::cout<<A::BW<<"  ── This Session ──\n"<<A::R;
        row("Session listen time", fmtTimeLong(mp_.sessionListenSec()));
        row("Bookmarks saved",    std::to_string(mp_.bookmarks().size()));
        row("History entries",    std::to_string(mp_.fullHistory().size()));
        std::cout<<"\n"<<A::BW<<"  ── Genre Breakdown ──\n"<<A::R;
        std::map<std::string,int> gmap;
        if(!mp_.empty()){
            Node* n=mp_.headNode();
            do{ gmap[n->song.genre]++; n=n->next; }while(n!=mp_.headNode());
        }
        int maxG=1;
        for(auto& kv:gmap) maxG=std::max(maxG,kv.second);
        int barW=std::min(30,cols_-30);
        for(auto& kv:gmap){
            int filled=(int)((float)kv.second/(float)maxG*(float)barW);
            std::cout<<A::MG<<"  "<<padR(kv.first,16)
                     <<A::W<<rep("█",filled)
                     <<A::MG<<rep("░",barW-filled)
                     <<" "<<kv.second<<A::R<<"\n";
        }
        waitPrompt();
    }

    void modalInfo() {
        const Song* s=mp_.currentSong();
        scls(); modalHeader("SONG DETAILS");
        if(!s){
            std::cout<<A::MG<<"  No song.\n"<<A::R;
        } else {
            std::cout<<"\n";
            auto row=[&](const std::string& k,const std::string& v){
                std::cout<<A::MG<<"  "<<padR(k,16)<<A::W<<v<<A::R<<"\n";
            };
            row("Title",      s->title);
            row("Artist",     s->artist);
            row("Genre",      s->genre);
            row("Year",       std::to_string(s->year));
            row("Duration",   s->duration+" ("+std::to_string(s->toSecs())+"s)");
            row("Rating",     starStr(s->rating)+" ("+std::to_string(s->rating)+"/5)");
            row("Play Count", std::to_string(s->playCount));
            row("Tags",       s->tags.empty()?"-":s->tags);
            row("Added",      s->addedDate.empty()?"-":s->addedDate);
            row("Track",      std::to_string(mp_.currentIndex()+1)+" / "+std::to_string(mp_.size()));
        }
        waitPrompt();
    }

    void modalSort() {
        scls(); modalHeader("SORT PLAYLIST");
        std::cout<<A::MG
            <<"\n  [1]  Title (A-Z)\n  [2]  Artist (A-Z)\n  [3]  Genre\n"
            <<"  [4]  Year (oldest)\n  [5]  Most Played\n  [6]  Highest Rated\n"
            <<"  [7]  Duration (shortest)\n\n"
            <<"  Choice: "<<A::R<<std::flush;
        std::string ch=readLine();
        const char* labels[]={"title","artist","genre","year","plays","rating","duration"};
        MusicPlayer::SortKey keys[]={
            MusicPlayer::SortKey::TITLE,  MusicPlayer::SortKey::ARTIST,
            MusicPlayer::SortKey::GENRE,  MusicPlayer::SortKey::YEAR,
            MusicPlayer::SortKey::PLAYS,  MusicPlayer::SortKey::RATING,
            MusicPlayer::SortKey::DURATION
        };
        int idx=-1; try{idx=std::stoi(ch)-1;}catch(...){}
        if(idx>=0&&idx<7){
            mp_.sortBy(keys[idx]);
            setStatus(std::string("Sorted by ")+labels[idx]+".");
            mp_.saveCSV();
        } else setStatus("Cancelled.");
    }

    void modalGenreFilter() {
        scls(); modalHeader("GENRE FILTER");
        auto genres=mp_.genres();
        if(genres.empty()){
            std::cout<<A::MG<<"  No genres.\n"<<A::R; waitPrompt(); return;
        }
        std::cout<<"\n";
        for(int i=0;i<(int)genres.size();i++)
            std::cout<<A::MG<<"  ["<<(i+1)<<"]  "<<A::LG<<genres[i]<<A::R<<"\n";
        std::cout<<"\n"<<A::MG<<"  Pick genre (or Enter to cancel): "<<A::R<<std::flush;
        std::string ch=readLine();
        int idx=-1; try{idx=std::stoi(ch)-1;}catch(...){}
        if(idx>=0&&idx<(int)genres.size()){
            auto results=mp_.byGenre(genres[idx]);
            scls(); modalHeader("GENRE: "+genres[idx]);
            line(cols_);
            for(Node* n:results){
                bool ic=(n==mp_.currentNode());
                std::string row="  "+padR(n->song.title,30)+padR(n->song.artist,22)+n->song.duration;
                if(ic) std::cout<<A::INV<<row<<A::R<<"\n";
                else   std::cout<<A::LG <<row<<A::R<<"\n";
            }
            line(cols_);
            std::cout<<A::MG<<"  "<<results.size()<<" songs in genre.\n"<<A::R;
            waitPrompt();
        }
    }

    void modalBookmarks() {
        scls(); modalHeader("BOOKMARKS  [a=add  d=delete  j=jump  q=back]");
        const auto& bms=mp_.bookmarks();
        if(bms.empty()){
            std::cout<<A::MG<<"  No bookmarks.\n"<<A::R;
        } else {
            line(cols_);
            for(int i=0;i<(int)bms.size();i++){
                const auto& b=bms[i];
                std::cout<<A::MG<<"  ["<<padL(std::to_string(i+1),2)<<"]  "
                         <<A::W<<padR(b.label,20)
                         <<A::LG<<padR(b.songTitle,25)
                         <<A::MG<<"@ "<<fmtTime(b.positionSec)<<A::R<<"\n";
            }
            line(cols_);
        }
        std::cout<<A::MG<<"  Command: "<<A::R<<std::flush;
        std::string cmd=readLine();
        if(cmd.empty()) return;
        if(cmd=="a"||cmd=="A"){
            std::cout<<A::MG<<"  Label: "<<A::R<<std::flush;
            std::string lbl=readLine("mark");
            mp_.addBookmark(lbl.empty()?"mark":lbl);
            mp_.saveCSV();
        } else if(cmd[0]=='d'){
            int idx=-1; try{idx=std::stoi(cmd.substr(1))-1;}catch(...){}
            if(idx>=0&&idx<(int)bms.size()){ mp_.removeBookmark(idx); setStatus("Bookmark removed."); }
        } else if(cmd[0]=='j'){
            int idx=-1; try{idx=std::stoi(cmd.substr(1))-1;}catch(...){}
            if(idx>=0&&idx<(int)bms.size()){
                mp_.jumpToBookmark(idx);
                setStatus("Jumped to: "+bms[idx].label);
            }
        }
    }

    void modalRate() {
        scls(); modalHeader("RATE CURRENT SONG");
        const Song* s=mp_.currentSong();
        if(!s){
            std::cout<<A::MG<<"  No song.\n"<<A::R; waitPrompt(); return;
        }
        std::cout<<"\n  "<<A::W<<s->title<<A::MG<<" - "<<A::LG<<s->artist<<A::R<<"\n";
        std::cout<<"\n  Current: "<<A::W<<starStr(s->rating)<<A::R<<"\n\n";
        std::cout<<A::MG<<"  New rating [0-5]: "<<A::R<<std::flush;
        std::string r=readLine();
        try{ mp_.rateCurrent(std::stoi(r)); mp_.saveCSV(); } catch(...){ setStatus("[!] Invalid rating."); }
    }

    void modalHelp() {
        scls(); modalHeader("PHANTOM — KEYBOARD SHORTCUTS");
        struct KV{const char* k;const char* d;};
        KV h[]={
            {"SPACE",    "Play / Pause"},
            {"← →",     "Prev / Next song"},
            {"↑ ↓",     "Volume up / down"},
            {"n / p",   "Next / Prev (vi)"},
            {"+ / -",   "Volume fine (+/-5%)"},
            {"r",       "Cycle repeat  OFF→1→ALL"},
            {"s",       "Shuffle (Fisher-Yates)"},
            {"z",       "Random jump"},
            {"f / B",   "Seek ±10 seconds"},
            {"w",       "WARP mode (jump to %)"},
            {"b",       "Bookmark current position"},
            {"k",       "Bookmark manager"},
            {"m",       "Rate / mark current song"},
            {"a",       "Add song"},
            {"d",       "Delete current"},
            {"e",       "Edit current song"},
            {"i",       "Song details"},
            {"/",       "Search (title/artist/genre/tags)"},
            {"l",       "Playlist browser"},
            {"h",       "Playback history"},
            {"t",       "Stats dashboard"},
            {"S",       "Sort menu"},
            {"g",       "Genre filter"},
            {"?",       "This help screen"},
            {"q",       "Save & quit"},
            {nullptr,nullptr}
        };
        std::cout<<"\n";
        for(int i=0;h[i].k;i++)
            std::cout<<A::W<<"  "<<padR(h[i].k,12)<<A::MG<<h[i].d<<A::R<<"\n";
        waitPrompt();
    }

private:
    MusicPlayer& mp_;
    std::string  statusMsg_;
    int          statusAge_  = 0;
    int          cols_=80, rows_=24;
    Mode         mode_ = Mode::MAIN;
    std::string  warpInput_;

    void line(int c, char ch='-') {
        if(c<=0) return;
        std::cout<<A::MG<<std::string((size_t)c,ch)<<A::R<<"\n";
    }

    void lineB(int c, char ch='=') {
        if(c<=0) return;
        std::cout<<A::MG<<std::string((size_t)c,ch)<<A::R<<"\n";
    }

    static void modalHeader(const std::string& title) {
        int c=tCols();
        std::cout<<A::MG<<rep("═",c)<<A::R<<"\n";
        // FIX 5: Use visLen for centering so ANSI codes in title don't offset padding
        int tl = visLen(title);
        int p=std::max(0,(c-tl)/2);
        std::cout<<A::BW<<std::string((size_t)p,' ')<<title<<A::R<<"\n";
        std::cout<<A::MG<<rep("═",c)<<A::R<<"\n\n";
    }

    static std::string fp(const std::string& label) {
        std::cout<<A::MG<<label<<A::W<<": "<<A::R<<std::flush;
        return readLine();
    }

    static void waitPrompt() {
        std::cout<<A::MG<<"\n  [Enter to return]"<<A::R<<std::flush; readLine();
    }

    void drawWarpMode() {
        drawMain();
        int bw=40, bh=7;
        int r0=(rows_-bh)/2, c0=(cols_-bw)/2;
        if(r0<1) r0=1;
        if(c0<1) c0=1;
        std::cout << A::go(r0,c0)   << A::MG << boxTop(bw) << A::R;
        std::cout << A::go(r0+1,c0) << A::MG << boxSide(std::string(A::BW)+"  WARP TO POSITION",bw) << A::R;
        std::cout << A::go(r0+2,c0) << A::MG << boxSide("",bw) << A::R;
        std::cout << A::go(r0+3,c0) << A::MG
                  << boxSide(std::string(A::LG)+"  Enter %  (0-100):  "+std::string(A::W)+warpInput_+"_",bw) << A::R;
        std::cout << A::go(r0+4,c0) << A::MG << boxSide("",bw) << A::R;
        std::cout << A::go(r0+5,c0) << A::MG << boxSide(std::string(A::DIM)+"  Enter=jump  Esc=cancel",bw) << A::R;
        std::cout << A::go(r0+6,c0) << A::MG << boxBot(bw) << A::R;
    }

    void drawMain() {
        drawTopBar();
        drawNowPlaying();
        drawVisualizer();
        drawProgressRow();
        drawTransportRow();
        lineB(cols_);
        drawPlaylist();
        drawFooterBar();
    }

    void drawTopBar() {
        std::cout << A::MG << "╔" << rep("═",cols_-2) << "╗\n" << A::R;
        std::string appName = "  PHANTOM";
        time_t now=time(nullptr); struct tm* tm_=localtime(&now);
        char tbuf[32]; strftime(tbuf,sizeof(tbuf),"%a %d %b %Y  %H:%M:%S",tm_);
        std::string dateStr(tbuf);
        int idx=mp_.currentIndex();
        std::string trkStr=(idx>=0)
            ?" TRK "+std::to_string(idx+1)+"/"+std::to_string(mp_.size())+"  "
            :" --/--  ";
        int used=(int)(appName.size()+dateStr.size()+trkStr.size());
        int gap1=(cols_-2-used)/2; if(gap1<1) gap1=1;
        int gap2=cols_-2-(int)appName.size()-gap1-(int)dateStr.size()-(int)trkStr.size();
        if(gap2<1) gap2=1;
        std::cout << A::MG << "║" << A::R;
        std::cout << A::BW << appName;
        std::cout << std::string((size_t)gap1,' ');
        std::cout << A::DG << dateStr;
        std::cout << std::string((size_t)gap2,' ');
        std::cout << A::MG << trkStr;
        std::cout << A::MG << "║\n" << A::R;
        std::cout << A::MG << "╠" << rep("═",cols_-2) << "╣\n" << A::R;
    }

    void drawNowPlaying() {
        const Song* s=mp_.currentSong();

        // ── Row 1: play icon + title + elapsed time (RIGHT-aligned) ──
        // FIX 6: elapsed time was computed but NEVER printed — now printed on right
        std::cout << A::MG << "║ " << A::R;
        if(!s){
            std::cout << A::MG << padR("  No song loaded  — press [a] to add",cols_-4) << A::R;
        } else {
            std::string icon  = mp_.isPlaying()
                ? std::string(A::BW)+"▶ "
                : std::string(A::MG)+"‖ ";
            std::string elStr = fmtTime(mp_.elapsedSec());   // e.g. "03:42"
            // visible chars used: 2 (icon) + 2 (spaces around border) + elStr + 1 space
            int titleW = cols_ - 4 - 2 - (int)elStr.size() - 1;
            if(titleW < 10) titleW = 10;
            std::cout << icon;
            std::cout << A::BW << padR(s->title, titleW) << A::R;
            std::cout << A::MG << " " << elStr;             // ← elapsed now visible
        }
        std::cout << A::MG << " ║\n" << A::R;

        // ── Row 2: artist / genre / year / stars / tags ──
        std::cout << A::MG << "║ " << A::R;
        if(s){
            std::cout << A::MG << "  ";
            std::cout << A::LG << padR(s->artist,22);
            std::cout << A::MG << padR(s->genre,14);
            std::string yearStr = s->year>0 ? std::to_string(s->year) : "    ";
            std::cout << yearStr;
            std::string stars = starStr(s->rating);
            std::cout << "  " << stars;
            // Remaining space for tags
            int used2 = 2 + 2 + 22 + 14 + (int)yearStr.size() + 2 + (int)visLen(stars);
            int tagSpace = cols_ - 4 - used2;
            if(!s->tags.empty() && tagSpace > 5){
                std::string tagStr = "  #"+s->tags;
                if((int)tagStr.size() > tagSpace) tagStr = tagStr.substr(0,(size_t)tagSpace);
                std::cout << A::MG << tagStr;
                used2 += (int)tagStr.size();
            }
            // Pad remainder so the row fills to the border
            int remainder = cols_ - 4 - used2;
            if(remainder > 0) std::cout << std::string((size_t)remainder,' ');
        } else {
            std::cout << std::string((size_t)(cols_-4),' ');
        }
        std::cout << A::R;
        std::cout << A::MG << " ║\n" << A::R;
    }

    void drawVisualizer() {
        int innerW = cols_ - 4;
        if(innerW < 1) innerW = 1;
        std::string wave = waveBar(mp_.tickVal(), innerW, mp_.isPlaying());
        std::cout << A::MG << "║ " << A::DIM << A::W << wave << A::R << A::MG << " ║\n" << A::R;
    }

    void drawProgressRow() {
        const Song* s=mp_.currentSong();
        int el=mp_.elapsedSec();
        int tot=s?s->toSecs():0;
        std::string elStr=fmtTime(el);
        std::string totStr=fmtTime(tot);
        // Layout: ║ <space> elStr <2sp> [bar] <2sp> totStr <space> ║
        int barW = cols_ - 4 - (int)elStr.size() - (int)totStr.size() - 4;
        if(barW < 4) barW = 4;
        std::string bar = pbar(mp_.progress(), barW);
        std::cout << A::MG << "║ ";
        std::cout << A::MG << elStr << "  ";
        std::cout << A::DIM << A::W << bar << A::R;
        std::cout << A::MG << "  " << totStr;
        std::cout << A::MG << " ║\n" << A::R;
    }

    void drawTransportRow() {
        // Fixed-width segments (visible chars only):
        // "VOL[" = 4, vbar(12) = 12, "] " = 2  → 18
        // volPct up to 4 chars + "  " = 6
        // "RPT[XXX]  " = 10
        // Total fixed = 34
        std::string volBar = vbar(mp_.volume(), 12);
        std::string volPct = std::to_string(mp_.volume()) + "%";
        std::string rpt    = std::string("RPT[") + rLabel(mp_.repeat()) + "]";

        // Compute how many chars the status area can use
        // Inner width = cols_-4; minus border ║<sp> and <sp>║ = 4
        int fixedVis = 4 + 12 + 2 + (int)volPct.size() + 2 + (int)rpt.size() + 2;
        int statusW  = cols_ - 4 - fixedVis;
        if(statusW < 0) statusW = 0;

        std::cout << A::MG << "║ ";
        std::cout << A::MG << "VOL[" << A::W << volBar << A::MG << "] ";
        std::cout << A::DG << volPct << "  ";
        std::cout << A::MG << rpt << "  ";

        // FIX 7: status field now uses a correctly computed width so it never
        // overflows or underflows the border
        if(!statusMsg_.empty() && statusW > 0){
            // Truncate visible length to statusW
            std::string disp;
            int vis = 0;
            bool inEsc = false;
            for(size_t i=0; i<statusMsg_.size() && vis<statusW; ++i){
                char c=statusMsg_[i];
                if(c=='\033'){inEsc=true; disp+=c; continue;}
                if(inEsc){disp+=c; if(std::isalpha((unsigned char)c)) inEsc=false; continue;}
                disp+=c; ++vis;
            }
            if(inEsc) disp += A::R;
            std::cout << A::W << disp;
            // Pad the rest
            if(vis < statusW) std::cout << std::string((size_t)(statusW-vis),' ');
            if(++statusAge_ > 30){ statusMsg_.clear(); statusAge_=0; }
        } else {
            if(statusW > 0) std::cout << std::string((size_t)statusW,' ');
        }
        std::cout << A::R;
        std::cout << A::MG << " ║\n" << A::R;
    }

    void drawPlaylist() {
        line(cols_);
        std::cout << A::MG;
        std::cout << "  " << padR("#",5);
        std::cout << padR("TITLE",28) << padR("ARTIST",20);
        std::cout << padR("DUR",7) << "★  PLAYS\n" << A::R;
        line(cols_);

        if(mp_.empty()){
            std::cout << A::MG << "  Playlist is empty. Press [a] to add songs.\n" << A::R;
            return;
        }

        // FIX 8: Count the actual rows drawn ABOVE the playlist area so that
        // the playlist doesn't overflow or underlap the footer.
        // Rows above playlist (all in drawMain before this call):
        //   drawTopBar:       3 lines  (top border, title row, separator)
        //   drawNowPlaying:   2 lines  (title row, artist row)
        //   drawVisualizer:   1 line
        //   drawProgressRow:  1 line
        //   drawTransportRow: 1 line
        //   lineB separator:  1 line
        //   header line:      1 line   (first line() inside drawPlaylist)
        //   column labels:    1 line
        //   second line():    1 line
        // = 12 rows fixed above playlist body
        // Rows below playlist (drawn in drawFooterBar):
        //   separator:        1 line
        //   key hints:        1 line
        //   bottom border:    1 line
        // = 3 rows below
        // + 1 footer summary line at end of drawPlaylist itself
        // Total overhead = 12 + 3 + 1 = 16
        static const int kFixedOverhead = 16;
        int available = rows_ - kFixedOverhead;
        if(available < 2) available = 2;

        int total  = mp_.size();
        int curIdx = mp_.currentIndex();
        // Center the viewport on the current track
        int offset = curIdx - available/2;
        offset = std::max(0, std::min(offset, std::max(0, total-available)));

        Node* n=mp_.headNode(); int idx=0;
        do {
            if(idx >= offset && idx < offset+available){
                bool ic=(idx==curIdx);
                std::string numStr=padR(std::to_string(idx+1)+".",5);
                std::string row="  "+numStr+padR(n->song.title,28)+padR(n->song.artist,20)
                               +padR(n->song.duration,7)+std::to_string(n->song.rating)
                               +"  "+std::to_string(n->song.playCount);
                if(ic)           std::cout << A::INV << row << A::R << "\n";
                else if(idx%2==0)std::cout << A::LG  << row << A::R << "\n";
                else             std::cout << A::MG  << row << A::R << "\n";
            }
            n=n->next; ++idx;
        } while(n!=mp_.headNode());

        line(cols_);
        int pct = total>0 ? (int)((float)(curIdx+1)/(float)total*100) : 0;
        std::cout << A::MG << "  ";
        std::cout << std::to_string(total)+" songs  |  #"+std::to_string(curIdx+1);
        std::cout << "  "+std::to_string(pct)+"%\n" << A::R;
    }

    void drawFooterBar() {
        std::cout << A::MG << "╠" << rep("═",cols_-2) << "╣\n" << A::R;
        std::string keys="  SPC:play  ←→:skip  ↑↓:vol  r:rpt  s:shuf  w:warp  f/B:seek  b:bmark  m:rate  a:add  /:search  ?:help  q:quit";
        if((int)keys.size() > cols_-4) keys=keys.substr(0,(size_t)(cols_-4));
        std::cout << A::MG << "║" << A::DG << padR(keys,cols_-2) << A::MG << "║\n" << A::R;
        std::cout << A::MG << "╚" << rep("═",cols_-2) << "╝\n" << A::R;
    }
};

// ═══════════════════════════════════════════════════════════════
//  Seed data
// ═══════════════════════════════════════════════════════════════
static void seedDefaults(MusicPlayer& mp, const std::string& today) {
    Song d[]={
        {"Echoes",                 "Pink Floyd",        "Prog Rock",   1971,"23:31",3,5,"psychedelic,epic",today},
        {"The Becoming",           "Nine Inch Nails",   "Industrial",  1994,"05:26",1,4,"dark,electronic",today},
        {"Clair de Lune",          "Claude Debussy",    "Classical",   1905,"04:54",2,5,"piano,impressionist",today},
        {"Midnight Rambler",       "The Rolling Stones","Rock",        1969,"06:52",0,3,"blues,classic",today},
        {"Weightless",             "Marconi Union",     "Ambient",     2011,"08:10",0,4,"relaxing,atmospheric",today},
        {"Black",                  "Pearl Jam",         "Grunge",      1991,"05:43",1,4,"emotional,seattle",today},
        {"Retrograde",             "James Blake",       "Electronic",  2013,"04:11",0,3,"soul,bass",today},
        {"In a Sentimental Mood",  "John Coltrane",     "Jazz",        1963,"04:17",2,5,"classic,saxophone",today},
        {"Welcome to the Machine", "Pink Floyd",        "Prog Rock",   1975,"07:31",0,3,"electronic,dark",today},
        {"Hurt",                   "Nine Inch Nails",   "Industrial",  1994,"03:44",4,5,"emotional,raw",today},
        {"Gymnopédie No.1",        "Erik Satie",        "Classical",   1888,"03:04",1,5,"piano,peaceful",today},
        {"Comfortably Numb",       "Pink Floyd",        "Prog Rock",   1979,"06:22",5,5,"guitar,classic",today},
        {"So What",                "Miles Davis",       "Jazz",        1959,"09:22",1,4,"modal,cool",today},
        {"Motion Picture OST",     "Radiohead",         "Alternative", 2000,"07:09",0,4,"electronic,strings",today},
        {"A Warm Place",           "Nine Inch Nails",   "Ambient",     1994,"04:43",0,4,"peaceful,texture",today},
        {"Pyramid Song",           "Radiohead",         "Alternative", 2001,"04:49",0,5,"piano,complex",today},
        {"Shine On You Crazy Dmnd","Pink Floyd",        "Prog Rock",   1975,"13:30",0,5,"guitar,epic",today},
        {"Blue in Green",          "Miles Davis",       "Jazz",        1959,"05:26",0,4,"trumpet,modal",today},
        {"The Fragile",            "Nine Inch Nails",   "Industrial",  1999,"04:35",0,3,"orchestral,dark",today},
        {"On the Run",             "Pink Floyd",        "Prog Rock",   1973,"03:30",0,3,"electronic,synth",today},
    };
    for(const auto& s:d) mp.insertBack(s,true);
}

// ═══════════════════════════════════════════════════════════════
//  main
// ═══════════════════════════════════════════════════════════════
int main(int argc, char* argv[]) {
    const std::string CSV_FILE = (argc>1)?argv[1]:"playlist.csv";
    const std::string BM_FILE  = CSV_FILE+".bookmarks";

    MusicPlayer mp;
    {
        std::ifstream t(CSV_FILE);
        if(!t.is_open()){ seedDefaults(mp,mp.today()); mp.saveCSV(CSV_FILE); }
    }
    mp.loadCSV(CSV_FILE);
    mp.loadBookmarks(BM_FILE);

    RawMode raw; gRaw=&raw;
    UI ui(mp);

    if(mp.dupesOnLoad()>0)
        ui.setStatus("[!] "+std::to_string(mp.dupesOnLoad())+" dupe(s) removed on load.");

    if(!mp.empty()) mp.setPlaying(true);

    bool quit=false;
    while(!quit){
        ui.draw();
        mp.advance(100);

        int k=readKey();
        if(k==0) continue;

        if(ui.mode()==UI::Mode::WARP){
            if(k==27||k=='q'){ ui.setMode(UI::Mode::MAIN); ui.warpInput().clear(); }
            else if(k=='\r'||k=='\n'){ ui.commitWarp(); }
            else if(k==127||k==8){
                if(!ui.warpInput().empty()) ui.warpInput().pop_back();
            } else if((k>='0'&&k<='9')||k=='.'){
                if(ui.warpInput().size()<5) ui.warpInput()+=(char)k;
            }
            continue;
        }

        if(k==27){
            int k2=readKey();
            if(k2=='['){
                int k3=readKey();
                if(k3=='C')      { mp.next(); }
                else if(k3=='D') { mp.prev(); }
                else if(k3=='A') { mp.volUp();   ui.setStatus("Vol "+std::to_string(mp.volume())+"%"); }
                else if(k3=='B') { mp.volDown(); ui.setStatus("Vol "+std::to_string(mp.volume())+"%"); }
            }
            continue;
        }

        switch(k){
            case 'q': case 'Q': quit=true; break;
            case ' ': mp.togglePlay(); ui.setStatus(mp.isPlaying()?"▶ Playing":"‖ Paused"); break;
            case 'n': mp.next(); break;
            case 'p': mp.prev(); break;
            case '+': case '=': mp.volUp();   ui.setStatus("Vol "+std::to_string(mp.volume())+"%"); break;
            case '-': case '_': mp.volDown(); ui.setStatus("Vol "+std::to_string(mp.volume())+"%"); break;
            case 'r': case 'R': mp.cycleRepeat(); ui.setStatus(std::string("Repeat: ")+rLabel(mp.repeat())); break;
            case 's': mp.shuffle(); ui.setStatus("Shuffled!"); break;
            case 'z': case 'Z': mp.jumpRandom(); break;
            case 'f': mp.seekSec(10);  ui.setStatus("Seek +10s"); break;
            case 'B': mp.seekSec(-10); ui.setStatus("Seek -10s"); break;
            case 'w': case 'W': ui.setMode(UI::Mode::WARP); break;
            case 'b': mp.addBookmark("bm@"+fmtTime(mp.elapsedSec())); mp.saveBookmarks(BM_FILE); break;
            case 'k': case 'K': ui.modalBookmarks(); mp.saveBookmarks(BM_FILE); break;
            case 'm': case 'M': ui.modalRate(); break;
            case 'a': case 'A': ui.modalAdd(); break;
            case 'd': case 'D': ui.modalDelete(); break;
            case 'e': case 'E': ui.modalEdit(); break;
            case 'i': case 'I': ui.modalInfo(); break;
            case '/': ui.modalSearch(); break;
            case 'l': case 'L': ui.modalPlaylist(); break;
            case 'h': case 'H': ui.modalHistory(); break;
            case 't': case 'T': ui.modalStats(); break;
            case 'S': ui.modalSort(); break;
            case 'g': case 'G': ui.modalGenreFilter(); break;
            case '?': ui.modalHelp(); break;
            default: break;
        }
    }

    gRaw=nullptr;
    mp.saveCSV(CSV_FILE);
    mp.saveBookmarks(BM_FILE);

    scls();
    std::cout<<A::BW<<"\n  Playlist saved   → "<<CSV_FILE
             <<"\n  Bookmarks saved  → "<<BM_FILE
             <<"\n\n  Goodbye.\n\n"<<A::R<<std::flush;
    return 0;
}
