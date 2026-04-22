// ╔══════════════════════════════════════════════════════════════════╗
// ║  music_player.cpp  —  Terminal Music Player  (Linux, C++17)     ║
// ║  Circular Doubly Linked List  |  Raw-mode keyboard  |  CSV      ║
// ║  Improved UI/UX — Monotone grayscale theme (FIXED)              ║
// ╚══════════════════════════════════════════════════════════════════╝
//
//  Build:  g++ -std=c++17 -O2 -Wall -o music_player music_player.cpp
//  Run:    ./music_player  [playlist.csv]
//
//  Keymap (live, no Enter needed):
//    n/p   - next / prev song          q   - quit (auto-save)
//    SPACE - play / pause toggle       s   - shuffle
//    +/-   - volume up / down          r   - cycle repeat mode
//    a     - add song (wizard)         d   - delete current song
//    /     - search                    e   - edit current song
//    l     - show full playlist        h   - history (last 20)
//    z     - random song               i   - song info detail
//    S     - sort menu                 ?   - help screen
// ===================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <cassert>
#include <csignal>

// POSIX / Linux
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>

// -----------------------------------------------------------------------
//  ANSI Monotone Theme — Pure grayscale hierarchy
// -----------------------------------------------------------------------
namespace A {
    // Reset
    constexpr const char* R   = "\033[0m";
    
    // Text styles
    constexpr const char* B   = "\033[1m";      // bold
    constexpr const char* DIM = "\033[2m";      // dim
    constexpr const char* UL  = "\033[4m";      // underline
    constexpr const char* REV = "\033[7m";      // reverse video
    
    // Grayscale levels (monotone)
    constexpr const char* WHITE = "\033[97m";   // brightest (primary)
    constexpr const char* LG    = "\033[37m";   // light gray (secondary)
    constexpr const char* GRAY  = "\033[90m";   // medium gray (tertiary)
    constexpr const char* DG    = "\033[30m";   // dark gray (background elements)
    constexpr const char* INV   = "\033[40;37m"; // inverted (selected)
    
    // Screen control
    constexpr const char* CLS = "\033[2J\033[H";
    constexpr const char* HC  = "\033[?25l";    // hide cursor
    constexpr const char* SC  = "\033[?25h";    // show cursor

    inline std::string mv(int r, int c) {
        return "\033[" + std::to_string(r) + ";" + std::to_string(c) + "H";
    }
}

// -----------------------------------------------------------------------
//  Terminal raw-mode RAII guard
// -----------------------------------------------------------------------
class RawMode {
public:
    RawMode() {
        tcgetattr(STDIN_FILENO, &orig_);
        struct termios raw = orig_;
        raw.c_iflag &= ~(BRKINT|ICRNL|INPCK|ISTRIP|IXON);
        raw.c_oflag &= ~(OPOST);
        raw.c_cflag |=  (CS8);
        raw.c_lflag &= ~(ECHO|ICANON|IEXTEN|ISIG);
        raw.c_cc[VMIN]  = 0;
        raw.c_cc[VTIME] = 1;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        std::cout << A::HC << std::flush;
    }
    ~RawMode() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_);
        std::cout << A::SC << std::flush;
    }
    const struct termios& orig() const { return orig_; }
private:
    struct termios orig_;
};

static RawMode* gRaw = nullptr;

static void clearScreen() {
    const char* seq = "\033[2J\033[H";
    ssize_t r = write(STDOUT_FILENO, seq, 7); (void)r;
}

// -----------------------------------------------------------------------
//  Terminal size
// -----------------------------------------------------------------------
static int termCols() {
    struct winsize w{}; ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col > 40 ? w.ws_col : 80;
}
static int termRows() {
    struct winsize w{}; ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_row > 10 ? w.ws_row : 24;
}

// -----------------------------------------------------------------------
//  Key read with timeout
// -----------------------------------------------------------------------
static int readKey(int timeoutMs = 0) {
    if (timeoutMs > 0) {
        struct timeval tv = { timeoutMs / 1000, (timeoutMs % 1000) * 1000 };
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0)
            return 0;
    }
    unsigned char c = 0;
    if (read(STDIN_FILENO, &c, 1) == 1) return (int)c;
    return 0;
}

// Blocking line input (temporarily restores cooked mode)
static std::string readLine(const std::string& prompt = "") {
    if (!prompt.empty()) {
        std::cout << A::GRAY << prompt << A::LG << " " << A::R << std::flush;
    }
    if (!gRaw) { std::string s; std::getline(std::cin, s); return s; }
    struct termios cooked = gRaw->orig();
    cooked.c_lflag |= (ECHO|ICANON);
    cooked.c_oflag |= (OPOST);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &cooked);
    std::cout << A::SC << std::flush;
    std::string s;
    std::getline(std::cin, s);
    // Restore raw
    struct termios raw = gRaw->orig();
    raw.c_iflag &= ~(BRKINT|ICRNL|INPCK|ISTRIP|IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |=  (CS8);
    raw.c_lflag &= ~(ECHO|ICANON|IEXTEN|ISIG);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    std::cout << A::HC << std::flush;
    // trim
    while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && std::isspace((unsigned char)s.back()))  s.pop_back();
    return s;
}

// -----------------------------------------------------------------------
//  Misc helpers
// -----------------------------------------------------------------------
static std::string toLower(const std::string& s) {
    std::string r = s;
    for (char& c : r) c = (char)std::tolower((unsigned char)c);
    return r;
}

static std::string trimStr(const std::string& s) {
    const char* ws = " \t\r\n";
    size_t a = s.find_first_not_of(ws);
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(ws);
    return s.substr(a, b - a + 1);
}

static std::string pad(const std::string& s, int w) {
    if (w <= 0) return "";
    if ((int)s.size() >= w) return s.substr(0, (size_t)w);
    return s + std::string((size_t)(w - (int)s.size()), ' ');
}

static std::string formatTime(int secs) {
    if (secs < 0) secs = 0;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", secs/60, secs%60);
    return buf;
}

// Smooth progress bar with gradient-like effect using different densities
static std::string progressBar(float fill, int width) {
    if (width < 4) return std::string((size_t)width, '.');
    int filled = (int)(fill * (float)width);
    filled = std::max(0, std::min(filled, width));
    std::string bar;
    for (int i = 0; i < width; ++i) {
        if (i < filled) {
            // Different densities for visual interest
            if (i % 3 == 0) bar += "#";
            else if (i % 3 == 1) bar += "=";
            else bar += "-";
        } else {
            bar += ".";
        }
    }
    return bar;
}

static std::string volBar(int vol, int width) {
    int filled = (int)((float)vol / 100.0f * (float)width);
    std::string b;
    for (int i = 0; i < width; ++i)
        b += (i < filled ? "#" : ".");
    return b;
}

// -----------------------------------------------------------------------
//  Song
// -----------------------------------------------------------------------
struct Song {
    std::string title;
    std::string artist;
    std::string genre;
    int         year      = 0;
    std::string duration;   // "MM:SS"
    int         playCount = 0;

    int toSeconds() const {
        int m = 0, s = 0;
        std::sscanf(duration.c_str(), "%d:%d", &m, &s);
        return m * 60 + s;
    }
    bool valid() const { return !title.empty() && !artist.empty(); }
};

// -----------------------------------------------------------------------
//  Node  (Circular Doubly Linked List)
// -----------------------------------------------------------------------
struct Node {
    Song  song;
    Node* prev = nullptr;
    Node* next = nullptr;
    explicit Node(const Song& s) : song(s) {}
};

// -----------------------------------------------------------------------
//  Repeat mode
// -----------------------------------------------------------------------
enum class RepeatMode { NONE, ONE, ALL };
static const char* repeatLabel(RepeatMode m) {
    switch (m) {
        case RepeatMode::NONE: return "O";   // empty circle
        case RepeatMode::ONE:  return "1";   // single
        case RepeatMode::ALL:  return "A";   // all
    }
    return "?";
}

static const char* repeatDescription(RepeatMode m) {
    switch (m) {
        case RepeatMode::NONE: return "Off";
        case RepeatMode::ONE:  return "Single";
        case RepeatMode::ALL:  return "All";
    }
    return "?";
}

// -----------------------------------------------------------------------
//  MusicPlayer  — all data + logic
// -----------------------------------------------------------------------
class MusicPlayer {
public:
    enum class SortKey { TITLE, ARTIST, GENRE, YEAR, PLAYS };

    MusicPlayer() { srand((unsigned)time(nullptr)); }
    ~MusicPlayer() { freeList(); }

    // ---- CSV I/O ---------------------------------------------------
    void loadCSV(const std::string& file) {
        csvFile_ = file;
        std::ifstream f(file);
        if (!f.is_open()) return;
        std::string line;
        int dupes = 0;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            Song s = parseLine(line);
            if (!s.valid()) continue;
            if (!insertBack(s, true)) ++dupes;
        }
        dupesOnLoad_ = dupes;
    }
    void saveCSV() const { saveCSV(csvFile_); }
    void saveCSV(const std::string& file) const {
        if (file.empty()) return;
        std::ofstream f(file, std::ios::trunc);
        if (!f.is_open()) return;
        Node* n = head_;
        if (!n) return;
        do {
            f << csvEsc(n->song.title)    << ','
              << csvEsc(n->song.artist)   << ','
              << csvEsc(n->song.genre)    << ','
              << n->song.year             << ','
              << csvEsc(n->song.duration) << ','
              << n->song.playCount        << '\n';
            n = n->next;
        } while (n != head_);
    }

    // ---- Insert / Delete -------------------------------------------
    bool insertBack(const Song& s, bool silent = false) {
        if (isDupe(s)) {
            if (!silent) lastMsg_ = "! Duplicate: " + s.title + " - " + s.artist;
            return false;
        }
        Node* node = new Node(s);
        if (!head_) {
            head_ = cur_ = node;
            node->next = node->prev = node;
        } else {
            Node* tail = head_->prev;
            tail->next  = node;
            node->prev  = tail;
            node->next  = head_;
            head_->prev = node;
        }
        ++size_;
        return true;
    }

    bool deleteCurrent() {
        if (!cur_) return false;
        Node* v = cur_;
        if (size_ == 1) { delete v; head_ = cur_ = nullptr; size_ = 0; return true; }
        cur_ = v->next;
        if (v == head_) head_ = v->next;
        v->prev->next = v->next;
        v->next->prev = v->prev;
        delete v; --size_;
        elapsed_ = 0;
        return true;
    }

    // ---- Navigation ------------------------------------------------
    void next() {
        if (!cur_) return;
        pushHistory(cur_->song);
        cur_ = cur_->next;
        elapsed_ = 0; playing_ = true;
    }
    void prev() {
        if (!cur_) return;
        if (!history_.empty()) {
            Song h = history_.back(); history_.pop_back();
            Node* n = findNode(h.title, h.artist);
            if (n) { cur_ = n; elapsed_ = 0; playing_ = true; return; }
        }
        cur_ = cur_->prev;
        elapsed_ = 0; playing_ = true;
    }
    void jumpRandom() {
        if (!head_ || size_ < 2) return;
        int skip = rand() % (size_ - 1) + 1;
        Node* n = cur_ ? cur_ : head_;
        for (int i = 0; i < skip; ++i) n = n->next;
        if (cur_) pushHistory(cur_->song);
        cur_ = n; elapsed_ = 0; playing_ = true;
    }

    // ---- Tick (call every ~100ms) -----------------------------------
    void tick(int ms = 100) {
        if (!playing_ || !cur_) return;
        elapsed_ += ms;
        int total = cur_->song.toSeconds() * 1000;
        if (total <= 0) total = 30000;
        if (elapsed_ >= total) {
            cur_->song.playCount++;
            pushHistory(cur_->song);
            if (repeat_ == RepeatMode::ONE) {
                elapsed_ = 0;
            } else {
                cur_ = cur_->next;
                elapsed_ = 0;
                if (repeat_ == RepeatMode::NONE && cur_ == head_)
                    playing_ = false;
            }
        }
    }

    // ---- Shuffle ----------------------------------------------------
    void shuffle() {
        if (size_ < 2) return;
        std::vector<Song> arr;
        arr.reserve(size_);
        Node* n = head_;
        do { arr.push_back(n->song); n = n->next; } while (n != head_);
        for (int i = (int)arr.size()-1; i > 0; --i) {
            int j = rand() % (i+1);
            std::swap(arr[i], arr[j]);
        }
        std::string ct = cur_ ? cur_->song.title  : "";
        std::string ca = cur_ ? cur_->song.artist : "";
        freeList();
        for (const auto& s : arr) insertBack(s, true);
        if (!ct.empty()) { Node* nc = findNode(ct, ca); if (nc) cur_ = nc; }
        lastMsg_ = "~ Shuffled " + std::to_string(size_) + " songs";
    }

    // ---- Sort ------------------------------------------------------
    void sortBy(SortKey key) {
        if (size_ < 2) return;
        std::vector<Song> arr;
        arr.reserve(size_);
        Node* n = head_;
        do { arr.push_back(n->song); n = n->next; } while (n != head_);
        switch (key) {
            case SortKey::TITLE:  std::sort(arr.begin(),arr.end(),[](const Song&a,const Song&b){ return toLower(a.title)<toLower(b.title); }); break;
            case SortKey::ARTIST: std::sort(arr.begin(),arr.end(),[](const Song&a,const Song&b){ return toLower(a.artist)<toLower(b.artist); }); break;
            case SortKey::GENRE:  std::sort(arr.begin(),arr.end(),[](const Song&a,const Song&b){ return toLower(a.genre)<toLower(b.genre); }); break;
            case SortKey::YEAR:   std::sort(arr.begin(),arr.end(),[](const Song&a,const Song&b){ return a.year<b.year; }); break;
            case SortKey::PLAYS:  std::sort(arr.begin(),arr.end(),[](const Song&a,const Song&b){ return a.playCount>b.playCount; }); break;
        }
        std::string ct = cur_ ? cur_->song.title  : "";
        std::string ca = cur_ ? cur_->song.artist : "";
        freeList();
        for (const auto& s : arr) insertBack(s, true);
        if (!ct.empty()) { Node* nc = findNode(ct, ca); if (nc) cur_ = nc; }
    }

    // ---- Edit ------------------------------------------------------
    void editField(const std::string& field, const std::string& value) {
        if (!cur_) return;
        if      (field=="title")    cur_->song.title    = value;
        else if (field=="artist")   cur_->song.artist   = value;
        else if (field=="genre")    cur_->song.genre    = value;
        else if (field=="duration") cur_->song.duration = value;
        else if (field=="year")     { try { cur_->song.year = std::stoi(value); } catch(...){} }
    }

    // ---- Search ----------------------------------------------------
    std::vector<Node*> search(const std::string& q) const {
        std::vector<Node*> res;
        if (!head_) return res;
        std::string ql = toLower(q);
        Node* n = head_;
        do {
            if (toLower(n->song.title).find(ql) != std::string::npos ||
                toLower(n->song.artist).find(ql)!= std::string::npos ||
                toLower(n->song.genre).find(ql) != std::string::npos)
                res.push_back(n);
            n = n->next;
        } while (n != head_);
        return res;
    }

    // ---- Accessors -------------------------------------------------
    const Song* currentSong()  const { return cur_ ? &cur_->song : nullptr; }
    Node*       currentNode()        { return cur_; }
    const Node* currentNode()  const { return cur_; }
    bool        isPlaying()    const { return playing_; }
    void        togglePlay()         { playing_ = !playing_; }
    void        setPlaying(bool v)   { playing_ = v; }
    int         elapsedSec()   const { return elapsed_ / 1000; }
    float       progress()     const {
        if (!cur_) return 0.f;
        int t = cur_->song.toSeconds();
        return (t > 0) ? std::min(1.0f, (float)elapsed_ / ((float)t * 1000.f)) : 0.f;
    }
    int         volume()       const { return vol_; }
    void        volUp()              { vol_ = std::min(100, vol_+5); }
    void        volDown()            { vol_ = std::max(0,   vol_-5); }
    RepeatMode  repeat()       const { return repeat_; }
    void        cycleRepeat()        {
        repeat_ = (repeat_==RepeatMode::NONE) ? RepeatMode::ONE  :
                  (repeat_==RepeatMode::ONE)  ? RepeatMode::ALL  : RepeatMode::NONE;
    }
    int         size()         const { return size_; }
    bool        empty()        const { return !head_; }
    Node*       headNode()           { return head_; }
    const Node* headNode()     const { return head_; }
    int         dupesOnLoad()  const { return dupesOnLoad_; }
    const std::deque<Song>& history() const { return history_; }

    std::string lastMsg() const { return lastMsg_; }
    void        clearMsg()      { lastMsg_.clear(); }

    int currentIndex() const {
        if (!head_ || !cur_) return -1;
        Node* n = head_; int i = 0;
        do { if (n == cur_) return i; n = n->next; ++i; } while (n != head_);
        return -1;
    }

private:
    Node*      head_       = nullptr;
    Node*      cur_        = nullptr;
    int        size_       = 0;
    bool       playing_    = false;
    int        elapsed_    = 0;   // ms
    int        vol_        = 80;
    RepeatMode repeat_     = RepeatMode::NONE;
    std::string csvFile_;
    std::string lastMsg_;
    int        dupesOnLoad_ = 0;
    std::deque<Song> history_;

    void pushHistory(const Song& s) {
        if (history_.size() >= 20) history_.pop_front();
        history_.push_back(s);
    }
    bool isDupe(const Song& s) const {
        if (!head_) return false;
        Node* n = head_;
        do {
            if (toLower(n->song.title) == toLower(s.title) &&
                toLower(n->song.artist)== toLower(s.artist))
                return true;
            n = n->next;
        } while (n != head_);
        return false;
    }
    Node* findNode(const std::string& title, const std::string& artist) const {
        if (!head_) return nullptr;
        Node* n = head_;
        do {
            if (toLower(n->song.title) == toLower(title) &&
                toLower(n->song.artist)== toLower(artist))
                return n;
            n = n->next;
        } while (n != head_);
        return nullptr;
    }
    void freeList() {
        if (!head_) return;
        Node* n = head_;
        do { Node* t = n->next; delete n; n = t; } while (n != head_);
        head_ = cur_ = nullptr; size_ = 0;
    }
    // CSV
    static Song parseLine(const std::string& line) {
        Song s;
        std::vector<std::string> f;
        std::string field; bool inQ = false;
        for (char c : line) {
            if (c=='"') { inQ=!inQ; continue; }
            if (c==',' && !inQ) { f.push_back(trimStr(field)); field.clear(); continue; }
            field += c;
        }
        f.push_back(trimStr(field));
        if (f.size() < 5) return s;
        s.title    = f[0]; s.artist = f[1]; s.genre = f[2];
        try { s.year = std::stoi(f[3]); } catch(...) {}
        s.duration = f[4];
        if (f.size() >= 6) { try { s.playCount = std::stoi(f[5]); } catch(...){} }
        return s;
    }
    static std::string csvEsc(const std::string& v) {
        if (v.find(',')==std::string::npos && v.find('"')==std::string::npos) return v;
        std::string r="\"";
        for (char c:v) { if(c=='"') r+='"'; r+=c; }
        return r+'"';
    }
};

// -----------------------------------------------------------------------
//  UI — Monotone theme with improved layout
// -----------------------------------------------------------------------
class UI {
public:
    explicit UI(MusicPlayer& mp) : mp_(mp) {}

    void draw() {
        int cols = termCols();
        int rows = termRows();
        buf_.clear();

        drawTopBar(cols);
        drawNowPlaying(cols);
        drawProgressSection(cols);
        drawStatusBar(cols);
        drawSeparator(cols);
        drawPlaylist(cols, rows);
        drawFooter(cols);

        clearScreen();
        fwrite(buf_.c_str(), 1, buf_.size(), stdout);
        fflush(stdout);
    }

    // -- Modals -------------------------------------------------------
    void modalAdd() {
        flushCls();
        drawModalHeader("+ ADD NEW SONG");
        
        Song s;
        std::cout << "\n";
        s.title    = readLineWithDefault("  Title", "");
        if (s.title.empty()) { setStatus("X Add cancelled"); return; }
        s.artist   = readLineWithDefault("  Artist", "");
        if (s.artist.empty()) { setStatus("X Add cancelled - artist required"); return; }
        s.genre    = readLineWithDefault("  Genre", "Unknown");
        std::string yr = readLineWithDefault("  Year", "0");
        try { s.year = std::stoi(yr); } catch(...) { s.year = 0; }
        s.duration = readLineWithDefault("  Duration (MM:SS)", "03:00");

        if (!s.valid()) {
            setStatus("X Title and Artist required.");
        } else if (mp_.insertBack(s)) {
            setStatus("+ Added: " + s.title + " - " + s.artist);
            mp_.saveCSV();
        } else {
            setStatus(mp_.lastMsg()); mp_.clearMsg();
        }
    }

    void modalEdit() {
        const Song* s = mp_.currentSong();
        if (!s) { setStatus("X No song selected."); return; }
        flushCls();
        drawModalHeader("* EDIT SONG");
        
        std::cout << "\n";
        std::string newTitle = readLineWithDefault("  Title", s->title);
        if (!newTitle.empty()) mp_.editField("title", newTitle);
        
        std::string newArtist = readLineWithDefault("  Artist", s->artist);
        if (!newArtist.empty()) mp_.editField("artist", newArtist);
        
        std::string newGenre = readLineWithDefault("  Genre", s->genre);
        if (!newGenre.empty()) mp_.editField("genre", newGenre);
        
        std::string newYear = readLineWithDefault("  Year", std::to_string(s->year));
        if (!newYear.empty()) mp_.editField("year", newYear);
        
        std::string newDuration = readLineWithDefault("  Duration", s->duration);
        if (!newDuration.empty()) mp_.editField("duration", newDuration);
        
        setStatus("+ Song updated");
        mp_.saveCSV();
    }

    void modalDelete() {
        const Song* s = mp_.currentSong();
        if (!s) { setStatus("X No song selected."); return; }
        flushCls();
        drawModalHeader("- DELETE SONG");
        std::cout << A::GRAY << "\n  " << s->title << A::DIM << " - " << A::LG << s->artist << "\n\n";
        std::string yn = readLineWithDefault("  Confirm delete? (y/N)", "n");
        if (yn == "y" || yn == "Y") {
            std::string t = s->title;
            mp_.deleteCurrent();
            setStatus("- Deleted: " + t);
            mp_.saveCSV();
        } else {
            setStatus("Delete cancelled");
        }
    }

    void modalSearch() {
        flushCls();
        drawModalHeader("? SEARCH");
        std::string q = readLineWithDefault("  Query", "");
        if (q.empty()) return;
        
        auto results = mp_.search(q);
        std::cout << "\n";
        if (results.empty()) {
            std::cout << A::DIM << "  No results found for \"" << q << "\"\n" << A::R;
        } else {
            int cols = termCols();
            drawHLine(cols, '-');
            std::cout << A::GRAY << "  " << pad("#",4) << pad("TITLE",30) << pad("ARTIST",22) << "DUR   PLAYS\n" << A::R;
            drawHLine(cols, '-');
            for (size_t i = 0; i < results.size(); ++i) {
                Node* n = results[i];
                bool isCur = (n == mp_.currentNode());
                std::string row = "  " + pad(std::to_string(i+1)+".",4) + pad(n->song.title,30) + pad(n->song.artist,22)
                                + pad(n->song.duration,6) + std::to_string(n->song.playCount);
                if (isCur) std::cout << A::INV << row << A::R << "\n";
                else       std::cout << A::WHITE << row << A::R << "\n";
            }
            drawHLine(cols, '-');
            std::cout << A::GRAY << "  " << results.size() << " result(s) found\n" << A::R;
        }
        std::cout << A::DIM << "\n  Press Enter to continue" << A::R << std::flush;
        readLine();
    }

    void modalPlaylist() {
        int offset = std::max(0, mp_.currentIndex() - 6);
        while (true) {
            flushCls();
            int cols = termCols();
            int rows = termRows();
            int visible = rows - 12;
            if (visible < 5) visible = 5;
            int total  = mp_.size();
            int curIdx = mp_.currentIndex();

            if (offset > curIdx)              offset = std::max(0, curIdx);
            if (offset + visible <= curIdx)   offset = curIdx - visible + 1;
            offset = std::max(0, std::min(offset, total - visible));

            drawModalHeader("# PLAYLIST BROWSER");
            std::cout << A::GRAY
                      << "  " << pad("#",5) << pad("TITLE",30)
                      << pad("ARTIST",22)     << pad("DUR",7) << "PLAYS\n" << A::R;
            drawHLine(cols, '-');

            if (mp_.empty()) {
                std::cout << A::DIM << "  Empty playlist\n" << A::R;
            } else {
                Node* n = mp_.headNode(); int idx = 0;
                do {
                    if (idx >= offset && idx < offset + visible) {
                        bool isCur = (idx == curIdx);
                        std::string row = "  " + pad(std::to_string(idx+1)+".",5)
                            + pad(n->song.title,30) + pad(n->song.artist,22)
                            + pad(n->song.duration,7) + std::to_string(n->song.playCount);
                        if (isCur) std::cout << A::INV << row << A::R << "\n";
                        else       std::cout << A::WHITE << row << A::R << "\n";
                    }
                    n = n->next; ++idx;
                } while (n != mp_.headNode());
            }
            drawHLine(cols, '-');
            std::cout << A::GRAY << "  " << total << " songs total  |  showing "
                      << (offset+1) << "-" << std::min(offset+visible,total) << "\n" << A::R;
            std::cout << A::DIM << "\n  up/dn or j/k to scroll  |  q to return" << A::R << std::flush;

            int k = 0;
            while (k == 0) k = readKey(50);

            if (k == 'q' || k == 'Q' || k == 27) break;
            else if (k == 'k' || k == 'K') { if (offset > 0) --offset; }
            else if (k == 'j' || k == 'J') { if (offset+visible < total) ++offset; }
            else if (k == 27) {
                int k2 = readKey(50);
                if (k2 == '[') {
                    int k3 = readKey(50);
                    if      (k3 == 'A') { if (offset>0) --offset; }
                    else if (k3 == 'B') { if (offset+visible<total) ++offset; }
                }
            }
        }
    }

    void modalHistory() {
        flushCls();
        int cols = termCols();
        drawModalHeader("@ PLAYBACK HISTORY");
        const auto& h = mp_.history();
        if (h.empty()) {
            std::cout << A::DIM << "\n  No playback history yet\n" << A::R;
        } else {
            std::cout << "\n";
            drawHLine(cols, '-');
            int i = (int)h.size();
            for (auto it = h.rbegin(); it != h.rend(); ++it, --i)
                std::cout << A::GRAY << "  " << std::right << std::setw(3) << i << ".  "
                          << A::WHITE << pad(it->title,35) << A::LG << it->artist << A::R << "\n";
            drawHLine(cols, '-');
        }
        std::cout << A::DIM << "\n  Press Enter to continue" << A::R << std::flush;
        readLine();
    }

    void modalInfo() {
        const Song* s = mp_.currentSong();
        flushCls();
        drawModalHeader("i SONG INFORMATION");
        if (!s) {
            std::cout << A::DIM << "\n  No song selected\n" << A::R;
        } else {
            std::cout << "\n";
            drawInfoRow("Title", s->title);
            drawInfoRow("Artist", s->artist);
            drawInfoRow("Genre", s->genre);
            drawInfoRow("Year", std::to_string(s->year));
            drawInfoRow("Duration", s->duration);
            drawInfoRow("Length (sec)", std::to_string(s->toSeconds()));
            drawInfoRow("Play Count", std::to_string(s->playCount));
            drawInfoRow("Track Position", std::to_string(mp_.currentIndex()+1) + " of " + std::to_string(mp_.size()));
        }
        std::cout << A::DIM << "\n  Press Enter to continue" << A::R << std::flush;
        readLine();
    }

    void modalSort() {
        flushCls();
        drawModalHeader("SORT PLAYLIST");
        std::cout << A::GRAY
            << "\n  +-----------------------------------------+\n"
            << "  |  [1]  > Sort by Title                    |\n"
            << "  |  [2]  > Sort by Artist                   |\n"
            << "  |  [3]  > Sort by Genre                    |\n"
            << "  |  [4]  > Sort by Year                     |\n"
            << "  |  [5]  > Sort by Most Played              |\n"
            << "  +-----------------------------------------+\n"
            << A::R << "\n  Choice: " << A::WHITE << std::flush;
        
        std::string ch = readLine();
        if      (ch=="1") { mp_.sortBy(MusicPlayer::SortKey::TITLE);  setStatus("+ Sorted by title"); }
        else if (ch=="2") { mp_.sortBy(MusicPlayer::SortKey::ARTIST); setStatus("+ Sorted by artist"); }
        else if (ch=="3") { mp_.sortBy(MusicPlayer::SortKey::GENRE);  setStatus("+ Sorted by genre"); }
        else if (ch=="4") { mp_.sortBy(MusicPlayer::SortKey::YEAR);   setStatus("+ Sorted by year"); }
        else if (ch=="5") { mp_.sortBy(MusicPlayer::SortKey::PLAYS);  setStatus("+ Sorted by plays"); }
        else              { setStatus("Sort cancelled"); return; }
        mp_.saveCSV();
    }

    void modalHelp() {
        flushCls();
        drawModalHeader("? KEYBOARD SHORTCUTS");
        struct KV { const char* key; const char* desc; };
        KV helps[] = {
            {"n / p",       "Next / Previous song"},
            {"Space",       "Play / Pause toggle"},
            {"+ / -",       "Volume up / down"},
            {"r",           "Cycle repeat mode"},
            {"s",           "Shuffle playlist"},
            {"z",           "Jump to random song"},
            {"a",           "Add a new song"},
            {"d",           "Delete current song"},
            {"e",           "Edit current song"},
            {"/",           "Search library"},
            {"l",           "Browse full playlist"},
            {"h",           "View playback history"},
            {"i",           "Song details"},
            {"S",           "Sort playlist"},
            {"?",           "This help screen"},
            {"q",           "Save & quit"},
            {nullptr, nullptr}
        };
        std::cout << "\n";
        for (int i = 0; helps[i].key; ++i) {
            std::cout << A::GRAY << "  " << std::left << std::setw(12) << helps[i].key
                      << A::LG << helps[i].desc << A::R << "\n";
        }
        std::cout << A::DIM << "\n  Press Enter to continue" << A::R << std::flush;
        readLine();
    }

    void setStatus(const std::string& m) { statusMsg_ = m; }

private:
    MusicPlayer& mp_;
    std::string  buf_;
    std::string  statusMsg_;
    std::string  lastModalMsg_;

    void emit(const char* s)         { buf_ += s; }
    void emit(const std::string& s)  { buf_ += s; }

    void drawHLine(int cols, char ch) {
        emit(A::GRAY);
        emit(std::string((size_t)cols, ch));
        emit(A::R);
        emit("\n");
    }

    static void flushCls() {
        clearScreen();
        fflush(stdout);
    }

    void drawModalHeader(const std::string& title) {
        int cols = termCols();
        std::cout << A::GRAY << std::string((size_t)cols, '=') << A::R << "\n";
        int p = std::max(0, (cols - (int)title.size()) / 2);
        std::cout << A::B << A::WHITE << std::string((size_t)p,' ') << title << A::R << "\n";
        std::cout << A::GRAY << std::string((size_t)cols, '=') << A::R << "\n";
    }

    static std::string readLineWithDefault(const std::string& label, const std::string& defaultValue) {
        std::cout << A::GRAY << label << A::DIM << " [" << defaultValue << "]" << A::LG << ": " << A::R << std::flush;
        std::string input = readLine();
        return input.empty() ? defaultValue : input;
    }

    static void drawInfoRow(const std::string& label, const std::string& value) {
        std::cout << A::GRAY << "  " << std::left << std::setw(16) << label + ":"
                  << A::WHITE << value << A::R << "\n";
    }

    void drawTopBar(int cols) {
        // Elegant minimal header
        emit(A::GRAY);
        emit(std::string((size_t)cols, '='));
        emit(A::R); emit("\n");
        
        std::string title = "# MUSIC PLAYER #";
        int p = std::max(0, (cols - (int)title.size()) / 2);
        emit(A::B); emit(A::WHITE);
        emit(std::string((size_t)p, ' '));
        emit(title);
        emit(A::R); emit("\n");
        
        emit(A::GRAY);
        emit(std::string((size_t)cols, '='));
        emit(A::R); emit("\n");
    }

    void drawNowPlaying(int cols) {
        const Song* s = mp_.currentSong();
        emit("\n");
        if (!s) {
            emit(A::DIM); emit("  ! No songs loaded -- press [a] to add\n"); emit(A::R);
        } else {
            // Play/pause indicator
            if (mp_.isPlaying()) { emit(A::WHITE); emit(A::B); emit("  >  "); }
            else                 { emit(A::GRAY);               emit("  ||  "); }
            
            emit(A::B); emit(A::WHITE);
            int titleW = std::max(20, cols - 8);
            emit(pad(s->title, titleW));
            emit(A::R); emit("\n");

            // Artist, Genre, Year on second line
            emit(A::GRAY); emit("     ");
            emit(A::LG); emit(pad(s->artist, 28));
            emit(A::GRAY); emit(" | ");
            emit(A::LG); emit(pad(s->genre, 14));
            if (s->year > 0) {
                emit(A::GRAY); emit(" | ");
                emit(A::LG); emit(std::to_string(s->year));
            }
            emit(A::R); emit("\n");
        }
        emit("\n");
    }

    void drawProgressSection(int cols) {
        const Song* s = mp_.currentSong();
        int barW = cols - 20;
        if (barW < 10) barW = 10;
        std::string bar = progressBar(mp_.progress(), barW);
        int elapsed = mp_.elapsedSec();
        int total   = s ? s->toSeconds() : 0;

        emit(A::GRAY); emit("  ");
        emit(A::WHITE); emit(formatTime(elapsed));
        emit(A::GRAY); emit("  ");
        emit(A::WHITE); emit(bar);
        emit(A::GRAY); emit("  ");
        emit(A::WHITE); emit(formatTime(total));
        emit(A::R); emit("\n");
    }

    void drawStatusBar(int cols) {
        std::string vb  = volBar(mp_.volume(), 12);
        std::string rep = repeatLabel(mp_.repeat());
        int idx = mp_.currentIndex();
        std::string trk = (idx >= 0)
            ? (std::to_string(idx+1) + "/" + std::to_string(mp_.size()))
            : "-/-";

        emit(A::GRAY);
        emit("  VOL "); emit(A::WHITE); emit(vb); emit(A::GRAY);
        emit(" " + std::to_string(mp_.volume()) + "%");
        emit("  RPT "); emit(A::WHITE); emit(rep); emit(A::GRAY);
        emit("  TRK "); emit(A::WHITE); emit(trk);
        emit("  " + std::string(repeatDescription(mp_.repeat())));

        if (!statusMsg_.empty()) {
            emit("    "); emit(A::LG); emit(statusMsg_); emit(A::R);
            statusMsg_.clear();
        }
        emit(A::R); emit("\n");
    }

    void drawSeparator(int cols) { 
        emit(A::GRAY); emit(std::string((size_t)cols, '-')); emit(A::R); emit("\n"); 
    }

    void drawPlaylist(int cols, int rows) {
        int available = rows - 16;
        if (available < 2) available = 2;

        if (mp_.empty()) {
            emit(A::DIM); emit("  ! Playlist empty -- press [a] to add songs\n"); emit(A::R);
            return;
        }

        int total  = mp_.size();
        int curIdx = mp_.currentIndex();
        int offset = curIdx - available/2;
        offset = std::max(0, std::min(offset, total - available));

        Node* n = mp_.headNode(); int idx = 0;
        do {
            if (idx >= offset && idx < offset + available) {
                bool isCur = (idx == curIdx);
                std::string line = "  " + pad(std::to_string(idx+1)+".", 5)
                    + pad(n->song.title,  30)
                    + pad(n->song.artist, 22)
                    + pad(n->song.duration, 7)
                    + std::to_string(n->song.playCount);
                if (isCur) { emit(A::INV); emit(line); emit(A::R); emit("\n"); }
                else        { emit(A::WHITE); emit(line); emit(A::R); emit("\n"); }
            }
            n = n->next; ++idx;
        } while (n != mp_.headNode());
    }

    void drawFooter(int cols) {
        drawHLine(cols, '-');
        emit(A::DIM);
        emit("  [n/p] < >  [SPC] play  [+/-] vol  [r] rpt  [s] shuf  [a] add  [d] del  [/] find  [l] list  [?] help  [q] quit");
        emit(A::R); emit("\n");
        emit(A::GRAY); emit(std::string((size_t)cols, '=')); emit(A::R); emit("\n");
    }
};

// -----------------------------------------------------------------------
//  Default seed data
// -----------------------------------------------------------------------
static void seedDefaults(MusicPlayer& mp) {
    Song defaults[] = {
        {"Echoes",                  "Pink Floyd",        "Prog Rock",   1971, "23:31", 0},
        {"The Becoming",            "Nine Inch Nails",   "Industrial",  1994, "05:26", 0},
        {"Clair de Lune",           "Claude Debussy",    "Classical",   1905, "04:54", 0},
        {"Midnight Rambler",        "The Rolling Stones","Rock",        1969, "06:52", 0},
        {"Weightless",              "Marconi Union",     "Ambient",     2011, "08:10", 0},
        {"Black",                   "Pearl Jam",         "Grunge",      1991, "05:43", 0},
        {"Retrograde",              "James Blake",       "Electronic",  2013, "04:11", 0},
        {"In a Sentimental Mood",   "John Coltrane",     "Jazz",        1963, "04:17", 0},
        {"Welcome to the Machine",  "Pink Floyd",        "Prog Rock",   1975, "07:31", 0},
        {"Hurt",                    "Nine Inch Nails",   "Industrial",  1994, "03:44", 0},
        {"Gymnopedie No.1",         "Erik Satie",        "Classical",   1888, "03:04", 0},
        {"Comfortably Numb",        "Pink Floyd",        "Prog Rock",   1979, "06:22", 0},
        {"So What",                 "Miles Davis",       "Jazz",        1959, "09:22", 0},
        {"Motion Picture Soundtrack","Radiohead",        "Alternative", 2000, "07:09", 0},
        {"A Warm Place",            "Nine Inch Nails",   "Ambient",     1994, "04:43", 0},
    };
    for (const auto& s : defaults) mp.insertBack(s, true);
}

// -----------------------------------------------------------------------
//  main
// -----------------------------------------------------------------------
int main(int argc, char* argv[]) {
    const std::string CSV_FILE = (argc > 1) ? argv[1] : "playlist.csv";

    MusicPlayer mp;

    // Seed on first run
    {
        std::ifstream test(CSV_FILE);
        if (!test.is_open()) { seedDefaults(mp); mp.saveCSV(CSV_FILE); }
    }
    mp.loadCSV(CSV_FILE);

    // Enter raw mode
    RawMode raw;
    gRaw = &raw;

    UI ui(mp);

    if (mp.dupesOnLoad() > 0)
        ui.setStatus("! " + std::to_string(mp.dupesOnLoad()) + " duplicate(s) cleaned on load");

    // Auto-start first song
    if (!mp.empty()) mp.setPlaying(true);

    bool quit = false;
    while (!quit) {
        ui.draw();
        mp.tick(100);

        int k = readKey(50);  // 50ms timeout for smooth UI
        if (k == 0) continue;

        // Handle ESC sequences (arrow keys)
        if (k == 27) {
            int k2 = readKey(50);
            if (k2 == '[') {
                int k3 = readKey(50);
                if      (k3 == 'C') { mp.next(); }
                else if (k3 == 'D') { mp.prev(); }
                else if (k3 == 'A') { mp.volUp();  ui.setStatus("Vol: " + std::to_string(mp.volume()) + "%"); }
                else if (k3 == 'B') { mp.volDown(); ui.setStatus("Vol: " + std::to_string(mp.volume()) + "%"); }
            }
            continue;
        }

        switch (k) {
            case 'q': case 'Q': quit = true; break;
            case ' ':
                mp.togglePlay();
                ui.setStatus(mp.isPlaying() ? "> Playing" : "|| Paused");
                break;
            case 'n': case 'N': mp.next(); break;
            case 'p': case 'P': mp.prev(); break;
            case '+': case '=': mp.volUp();   ui.setStatus("Vol: " + std::to_string(mp.volume()) + "%"); break;
            case '-': case '_': mp.volDown(); ui.setStatus("Vol: " + std::to_string(mp.volume()) + "%"); break;
            case 'r': case 'R':
                mp.cycleRepeat();
                ui.setStatus("Repeat: " + std::string(repeatDescription(mp.repeat())));
                break;
            case 's': mp.shuffle(); break;
            case 'S': ui.modalSort(); break;
            case 'z': case 'Z': mp.jumpRandom(); ui.setStatus("Random song"); break;
            case 'a': case 'A': ui.modalAdd(); break;
            case 'd': case 'D': ui.modalDelete(); break;
            case 'e': case 'E': ui.modalEdit(); break;
            case '/': ui.modalSearch(); break;
            case 'l': case 'L': ui.modalPlaylist(); break;
            case 'h': case 'H': ui.modalHistory(); break;
            case 'i': case 'I': ui.modalInfo(); break;
            case '?': ui.modalHelp(); break;
            default: break;
        }
    }

    gRaw = nullptr;
    mp.saveCSV(CSV_FILE);

    clearScreen();
    std::cout << A::B << A::WHITE
              << "\n  + Playlist saved -> " << CSV_FILE
              << "\n  # Goodbye\n\n" << A::R << std::flush;
    return 0;
}
