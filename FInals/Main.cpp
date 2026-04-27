/*
 ╔══════════════════════════════════════════════════════════════════════╗
 ║         CROSSY ROAD — C++ Terminal Edition  v4.0  🐔                ║
 ║    Pseudo-3D Isometric ASCII  |  Linked-List Architecture            ║
 ║    Persistent Leaderboard     |  Player Name System                  ║
 ║                                                                      ║
 ║  Rendering model:                                                    ║
 ║    Each lane occupies 3 terminal rows (top-face + front-face +       ║
 ║    shadow row).  Isometric skew shifts each lane left by 1 col       ║
 ║    per depth step creating the illusion of perspective.              ║
 ║    Unicode half-block chars (▀ ▄ █) produce voxel-like tiles.        ║
 ╚══════════════════════════════════════════════════════════════════════╝
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <thread>
#include <termios.h>
#include <unistd.h>
#include <csignal>
#include <cmath>
#include <algorithm>

// ═══════════════════════════════════════════════════════════════════
//  Terminal helpers
// ═══════════════════════════════════════════════════════════════════
namespace Term {
    struct termios orig_attr;
    bool raw_active = false;

    void raw() {
        tcgetattr(STDIN_FILENO, &orig_attr);
        struct termios t = orig_attr;
        t.c_lflag &= ~(ICANON | ECHO);
        t.c_cc[VMIN]  = 0;
        t.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        raw_active = true;
    }
    void restore() {
        if (raw_active) {
            tcsetattr(STDIN_FILENO, TCSANOW, &orig_attr);
            raw_active = false;
        }
    }
    void clear()       { std::cout << "\033[2J\033[H"; }
    void home()        { std::cout << "\033[H";         }
    void hide_cursor() { std::cout << "\033[?25l";      }
    void show_cursor() { std::cout << "\033[?25h";      }
    void reset_attr()  { std::cout << "\033[0m";        }
    void bold()        { std::cout << "\033[1m";        }
    void fg(int n)     { std::cout << "\033[38;5;" << n << "m"; }
    void bg(int n)     { std::cout << "\033[48;5;" << n << "m"; }
    void move(int r, int c) { std::cout << "\033[" << r << ";" << c << "H"; }

    std::string sfg(int n)   { return "\033[38;5;" + std::to_string(n) + "m"; }
    std::string sbg(int n)   { return "\033[48;5;" + std::to_string(n) + "m"; }
    std::string sreset()     { return "\033[0m"; }
    std::string sbold()      { return "\033[1m"; }

    int read_key() {
        unsigned char buf[8] = {};
        int n = (int)read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) return 0;
        if (n >= 3 && buf[0] == 27 && buf[1] == '[') {
            if (buf[2] == 'A') return 1000;
            if (buf[2] == 'B') return 1001;
            if (buf[2] == 'C') return 1002;
            if (buf[2] == 'D') return 1003;
        }
        return (int)buf[0];
    }

    // Canonical (line-buffered) string input with echo
    std::string readLine(int maxLen = 20) {
        // Temporarily restore canonical mode for clean input
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_attr);
        std::string line;
        std::getline(std::cin, line);
        // Trim whitespace
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if ((int)line.size() > maxLen) line = line.substr(0, maxLen);
        if (line.empty()) line = "ANON";
        return line;
    }
}

// ═══════════════════════════════════════════════════════════════════
//  3D Rendering Constants
// ═══════════════════════════════════════════════════════════════════
static const int BW          = 40;
static const int BH          = 12;
static const int LANE_H      = 3;
static const int PLAY_ROWS   = BH * LANE_H;
static const int ISO_SKEW    = 1;
static const int SCR_W       = BW + BH * ISO_SKEW + 4;
static const int PLAYER_LANE = BH - 3;
static const int AHEAD_BUFFER = BH + 4;
static const int BELOW_BUFFER = BH + 4;

enum LaneType { SAFE, GRASS, ROAD, WATER };

// ═══════════════════════════════════════════════════════════════════
//  Leaderboard System
// ═══════════════════════════════════════════════════════════════════
static const int  LB_MAX      = 10;
static const char LB_FILE[]   = ".crossy_leaderboard.txt";

struct LeaderEntry {
    std::string name;
    int         score;
    std::string date;  // "YYYY-MM-DD"
};

// Singly-linked list for leaderboard (keeps theme of linked-list architecture)
struct LBNode {
    LeaderEntry data;
    LBNode*     next = nullptr;
    explicit LBNode(const LeaderEntry& e) : data(e) {}
};

struct Leaderboard {
    LBNode* head  = nullptr;
    int     count = 0;

    ~Leaderboard() { clear(); }

    void clear() {
        while (head) { LBNode* t = head; head = head->next; delete t; }
        count = 0;
    }

    // Insert sorted descending by score; keep only top LB_MAX
    void insert(const LeaderEntry& e) {
        LBNode* node = new LBNode(e);

        // Find insertion point
        if (!head || e.score >= head->data.score) {
            node->next = head;
            head = node;
        } else {
            LBNode* cur = head;
            while (cur->next && cur->next->data.score > e.score)
                cur = cur->next;
            node->next = cur->next;
            cur->next  = node;
        }
        ++count;

        // Trim to LB_MAX
        if (count > LB_MAX) {
            LBNode* cur = head;
            for (int i = 1; i < LB_MAX; ++i) cur = cur->next;
            LBNode* excess = cur->next;
            cur->next = nullptr;
            while (excess) {
                LBNode* t = excess; excess = excess->next; delete t;
            }
            count = LB_MAX;
        }
    }

    void load() {
        clear();
        std::ifstream f(LB_FILE);
        if (!f) return;
        std::string line;
        while (std::getline(f, line)) {
            std::istringstream ss(line);
            LeaderEntry e;
            ss >> e.score >> e.date;
            std::getline(ss, e.name);
            if (!e.name.empty() && e.name[0] == '|') e.name = e.name.substr(1);
            if (!e.name.empty()) {
                // Append without re-trimming (already sorted file)
                LBNode* node = new LBNode(e);
                if (!head) { head = node; }
                else {
                    LBNode* cur = head;
                    while (cur->next) cur = cur->next;
                    cur->next = node;
                }
                ++count;
            }
        }
    }

    void save() const {
        std::ofstream f(LB_FILE, std::ios::trunc);
        if (!f) return;
        LBNode* cur = head;
        while (cur) {
            f << cur->data.score << " "
              << cur->data.date  << "|"
              << cur->data.name  << "\n";
            cur = cur->next;
        }
    }

    bool qualifies(int score) const {
        if (count < LB_MAX) return true;
        // Check if score beats last entry
        LBNode* cur = head;
        while (cur->next) cur = cur->next;
        return score > cur->data.score;
    }

    int rankOf(int score) const {
        int rank = 1;
        LBNode* cur = head;
        while (cur) {
            if (cur->data.score > score) ++rank;
            cur = cur->next;
        }
        return rank;
    }
};

// Get today's date as "YYYY-MM-DD"
static std::string todayStr() {
    time_t t = time(nullptr);
    struct tm* tm_info = localtime(&t);
    char buf[16];
    strftime(buf, sizeof(buf), "%Y-%m-%d", tm_info);
    return std::string(buf);
}

// Draw the leaderboard screen
void showLeaderboard(const Leaderboard& lb, const std::string& playerName, int playerScore, bool newEntry) {
    Term::clear();
    std::cout << "\n";

    // Header
    Term::fg(220); Term::bold();
    std::cout << "  ╔══════════════════════════════════════════════════════╗\n";
    std::cout << "  ║           🏆  CROSSY ROAD 3D — LEADERBOARD  🏆       ║\n";
    std::cout << "  ╠══════════════════════════════════════════════════════╣\n";
    Term::reset_attr();

    // Column headers
    Term::fg(240);
    std::cout << "  ║  ";
    Term::fg(244);
    std::cout << "RANK  NAME                    SCORE      DATE";
    Term::fg(240);
    // pad to 54 chars
    std::cout << "      ║\n";
    Term::fg(240);
    std::cout << "  ╠══════════════════════════════════════════════════════╣\n";
    Term::reset_attr();

    if (!lb.head) {
        Term::fg(240);
        std::cout << "  ║           No entries yet — be the first!             ║\n";
    } else {
        LBNode* cur = lb.head;
        int rank = 1;
        while (cur) {
            bool isPlayer = newEntry && (cur->data.name == playerName && cur->data.score == playerScore);

            std::cout << "  ║  ";

            // Rank medal
            if (rank == 1)      { Term::fg(220); Term::bold(); std::cout << " 🥇 "; }
            else if (rank == 2) { Term::fg(250); Term::bold(); std::cout << " 🥈 "; }
            else if (rank == 3) { Term::fg(166); Term::bold(); std::cout << " 🥉 "; }
            else                { Term::fg(238); std::cout << "  " << rank << "  "; }
            Term::reset_attr();

            // Name
            if (isPlayer) { Term::fg(226); Term::bold(); }
            else          { Term::fg(252); }

            std::string name = cur->data.name;
            if ((int)name.size() > 18) name = name.substr(0,18);
            std::cout << name;
            int namePad = 18 - (int)name.size();
            for (int i=0; i<namePad; ++i) std::cout << ' ';

            std::cout << "   ";

            // Score
            if (isPlayer) { Term::fg(226); }
            else          { Term::fg(255); }
            std::string sc = std::to_string(cur->data.score);
            std::cout << sc;
            int scPad = 8 - (int)sc.size();
            for (int i=0; i<scPad; ++i) std::cout << ' ';
            Term::reset_attr();

            // Date
            Term::fg(238);
            std::cout << "  " << cur->data.date;
            if (isPlayer) {
                Term::fg(226); std::cout << " ◀ YOU";
                Term::fg(240); std::cout << "  ║\n";
            } else {
                std::cout << "        ║\n";
            }
            Term::reset_attr();

            cur = cur->next;
            ++rank;
        }
    }

    Term::fg(240);
    std::cout << "  ╚══════════════════════════════════════════════════════╝\n\n";
    Term::reset_attr();
}

// ═══════════════════════════════════════════════════════════════════
//  Color palettes for 3D faces
// ═══════════════════════════════════════════════════════════════════
struct LanePalette {
    int top_fg, top_bg;
    int front_fg, front_bg;
    int shadow_bg;
    char top_ch;
    char front_ch;
};

LanePalette makePalette(LaneType t, int depthFromPlayer) {
    int d = std::min(depthFromPlayer, BH - 1);
    float fade = 1.0f - (d / (float)BH) * 0.55f;

    auto darken = [&](int base, float f) -> int {
        if (base >= 232 && base <= 255) {
            int v = (int)((base - 232) * f);
            return 232 + std::max(0, std::min(23, v));
        }
        return base;
    };

    LanePalette p;
    switch (t) {
        case SAFE:
            p.top_fg    = darken(253, fade * 0.9f);
            p.top_bg    = darken(240, fade);
            p.front_fg  = darken(245, fade * 0.7f);
            p.front_bg  = darken(236, fade);
            p.shadow_bg = 232;
            p.top_ch    = ' ';
            p.front_ch  = ' ';
            break;
        case GRASS:
            {
                int gtop  = (d < 3) ? 28 : (d < 6) ? 22 : 22;
                int gfrnt = (d < 3) ? 22 : 22;
                p.top_fg    = (d < 3) ? 34 : 28;
                p.top_bg    = gtop;
                p.front_fg  = (d < 3) ? 28 : 22;
                p.front_bg  = gfrnt;
                p.shadow_bg = 232;
                p.top_ch    = ' ';
                p.front_ch  = ' ';
            }
            break;
        case ROAD:
            {
                int rtop  = darken(241, fade);
                int rfrnt = darken(237, fade);
                p.top_fg    = darken(248, fade);
                p.top_bg    = rtop;
                p.front_fg  = darken(243, fade * 0.8f);
                p.front_bg  = rfrnt;
                p.shadow_bg = 232;
                p.top_ch    = ' ';
                p.front_ch  = ' ';
            }
            break;
        case WATER:
            {
                int wdepth = (d < 2) ? 27 : (d < 5) ? 20 : 17;
                int wfrnt  = (d < 2) ? 20 : (d < 5) ? 17 : 17;
                p.top_fg    = (d < 3) ? 51 : (d < 6) ? 39 : 27;
                p.top_bg    = wdepth;
                p.front_fg  = (d < 3) ? 27 : 20;
                p.front_bg  = wfrnt;
                p.shadow_bg = 232;
                p.top_ch    = ' ';
                p.front_ch  = ' ';
            }
            break;
    }
    return p;
}

// ═══════════════════════════════════════════════════════════════════
//  Generic singly-linked list
// ═══════════════════════════════════════════════════════════════════
template<typename T>
struct SLNode {
    T        data;
    SLNode*  next = nullptr;
    explicit SLNode(const T& d) : data(d) {}
};

template<typename T>
struct SLinkedList {
    SLNode<T>* head = nullptr;
    SLNode<T>* tail = nullptr;
    int        size = 0;

    SLinkedList()                              = default;
    SLinkedList(const SLinkedList&)            = delete;
    SLinkedList& operator=(const SLinkedList&) = delete;
    ~SLinkedList() { clear(); }

    void push_back(const T& d) {
        auto* n = new SLNode<T>(d);
        if (!tail) { head = tail = n; }
        else       { tail->next = n; tail = n; }
        ++size;
    }

    void clear() {
        while (head) {
            auto* t = head; head = head->next; delete t;
        }
        tail = nullptr; size = 0;
    }
};

// ═══════════════════════════════════════════════════════════════════
//  Obstacle  (3D aware)
// ═══════════════════════════════════════════════════════════════════
struct Obstacle {
    double x;
    int    width;
    double speed;
    int    vtype;   // 0=car, 1=log

    bool contains(double px) const {
        return px >= x && px < x + (double)width;
    }
    int iLeft()  const { return (int)x; }
    int iRight() const { return (int)(x + width); }
};

// ═══════════════════════════════════════════════════════════════════
//  Lane
// ═══════════════════════════════════════════════════════════════════
struct Lane {
    LaneType              type;
    SLinkedList<Obstacle> obs;
    int                   shade;
    Lane*                 prev = nullptr;
    Lane*                 next = nullptr;

    Lane(LaneType t, int s) : type(t), shade(s) {}
    Lane(const Lane&)            = delete;
    Lane& operator=(const Lane&) = delete;
};

// ═══════════════════════════════════════════════════════════════════
//  World — doubly-linked lane list
// ═══════════════════════════════════════════════════════════════════
struct World {
    Lane* head  = nullptr;
    Lane* tail  = nullptr;
    int   count = 0;
    int   baseY = -1;

    ~World() {
        Lane* cur = head;
        while (cur) { Lane* n = cur->next; delete cur; cur = n; }
    }

    int headAbsY() const { return (count > 0) ? baseY + (count - 1) : -1; }

    void pushTop(Lane* l) {
        l->next = head; l->prev = nullptr;
        if (head) head->prev = l; else tail = l;
        head = l; ++count;
        if (count == 1) baseY = 0;
    }

    void popBottom() {
        if (!tail) return;
        Lane* t = tail;
        tail = t->prev;
        if (tail) tail->next = nullptr; else head = nullptr;
        delete t; --count; ++baseY;
    }

    Lane* laneAt(int absY) const {
        if (count == 0) return nullptr;
        int tailAbsY = baseY;
        int headAY   = tailAbsY + (count - 1);
        if (absY < tailAbsY || absY > headAY) return nullptr;
        int fromTail = absY - tailAbsY;
        int fromHead = headAY - absY;
        if (fromTail <= fromHead) {
            Lane* cur = tail;
            for (int i = 0; i < fromTail; ++i) cur = cur->prev;
            return cur;
        } else {
            Lane* cur = head;
            for (int i = 0; i < fromHead; ++i) cur = cur->next;
            return cur;
        }
    }
};

// ═══════════════════════════════════════════════════════════════════
//  RNG
// ═══════════════════════════════════════════════════════════════════
static int    rng (int lo, int hi)       { return lo + rand() % (hi - lo + 1); }
static double rngf(double lo, double hi) { return lo + (hi-lo)*(rand()/(double)RAND_MAX); }

// ═══════════════════════════════════════════════════════════════════
//  Lane factory
// ═══════════════════════════════════════════════════════════════════
static int s_laneGen = 0;

Lane* makeLane(int idx) {
    int rel = ((idx % 16) + 16) % 16;

    LaneType type;
    int shade;
    if      (rel == 0 || rel == 7 || rel == 15) { type = SAFE;  shade = 238; }
    else if (rel == 1 || rel == 2 || rel == 8
          || rel == 13|| rel == 14)              { type = GRASS; shade = 236; }
    else if (rel >= 3 && rel <= 6)               { type = ROAD;  shade = 234; }
    else                                         { type = WATER; shade = 233; }

    Lane* l = new Lane(type, shade);

    if (type == ROAD) {
        double spd = rngf(0.20, 0.55);
        if (rand() % 2) spd = -spd;
        int numCars = rng(2, 4);
        double carW = rng(4, 6);
        double minGap = 9.0;
        double pos = rngf(0.0, 4.0);
        for (int i = 0; i < numCars; ++i) {
            Obstacle o;
            o.x     = pos;
            o.width = (int)carW;
            o.speed = spd;
            o.vtype = 0;
            l->obs.push_back(o);
            pos += carW + minGap + rngf(0, 6);
        }
    } else if (type == WATER) {
        double spd = rngf(0.12, 0.30);
        if (rand() % 2) spd = -spd;
        int numLogs = rng(3, 5);
        double logW = rngf(5.0, 9.0);
        double gap  = rngf(4.0, 7.0);
        double pos  = rngf(0.0, 4.0);
        for (int i = 0; i < numLogs; ++i) {
            Obstacle o;
            o.x     = pos;
            o.width = (int)logW;
            o.speed = spd;
            o.vtype = 1;
            l->obs.push_back(o);
            pos += logW + gap;
            if (pos > BW) pos -= BW;
        }
    }
    return l;
}

void buildInitialWorld(World& w) {
    s_laneGen = 0;
    Lane* l = makeLane(s_laneGen++);
    w.head = w.tail = l;
    w.count = 1;
    w.baseY = 0;
    for (int i = 1; i <= AHEAD_BUFFER; ++i) {
        Lane* nl = makeLane(s_laneGen++);
        nl->next = w.head; nl->prev = nullptr;
        if (w.head) w.head->prev = nl;
        w.head = nl;
        ++w.count;
    }
}

void ensureLanes(World& w, int playerAbsY) {
    int needed_high = playerAbsY + AHEAD_BUFFER;
    int needed_low  = playerAbsY - BELOW_BUFFER;
    while (w.headAbsY() < needed_high) {
        Lane* l = makeLane(s_laneGen++);
        l->next = w.head; l->prev = nullptr;
        if (w.head) w.head->prev = l; else w.tail = l;
        w.head = l; ++w.count;
    }
    while (w.count > 0 && w.baseY < needed_low) {
        w.popBottom();
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Physics
// ═══════════════════════════════════════════════════════════════════
void tickLane(Lane* l) {
    if (!l) return;
    double W = (double)BW;
    auto* cur = l->obs.head;
    while (cur) {
        Obstacle& o = cur->data;
        o.x += o.speed;
        if (o.speed > 0 && o.x >= W)               o.x -= W + o.width;
        if (o.speed < 0 && o.x + o.width <= 0.0)   o.x += W + o.width;
        cur = cur->next;
    }
}

Obstacle* findObs(Lane* l, double px) {
    if (!l) return nullptr;
    auto* cur = l->obs.head;
    while (cur) {
        if (cur->data.contains(px)) return &cur->data;
        cur = cur->next;
    }
    return nullptr;
}

// ═══════════════════════════════════════════════════════════════════
//  Player
// ═══════════════════════════════════════════════════════════════════
struct Player {
    double px       = BW / 2.0;
    int    absY     = 0;
    bool   alive    = true;
    int    score    = 0;
    double logAccum = 0.0;
    int    jumpFrame = 0;
    bool   movDir   = true;
    std::string name = "ANON";
};

// ═══════════════════════════════════════════════════════════════════
//  Movement
// ═══════════════════════════════════════════════════════════════════
void tryMove(World& w, Player& p, int dx, int dy) {
    double newPx = p.px + dx;
    int newAbsY  = p.absY + dy;

    if (newPx < 0.0)        newPx = 0.0;
    if (newPx > BW - 1.0)   newPx = BW - 1.0;

    if (dx != 0) p.movDir = (dx > 0);

    Lane* tgt = w.laneAt(newAbsY);
    if (!tgt) return;

    if (tgt->type == ROAD) {
        Obstacle* car = findObs(tgt, newPx);
        p.px = newPx; p.absY = newAbsY; p.logAccum = 0.0;
        if (car) p.alive = false;
    } else if (tgt->type == WATER) {
        Obstacle* log = findObs(tgt, newPx);
        p.px = newPx; p.absY = newAbsY; p.logAccum = 0.0;
        if (!log) p.alive = false;
    } else {
        p.px = newPx; p.absY = newAbsY; p.logAccum = 0.0;
    }

    if (p.alive) p.jumpFrame = 1;
    if (p.absY > p.score) p.score = p.absY;
}

void physTick(World& w, Player& p) {
    if (!p.alive) return;

    int hi = p.absY + PLAYER_LANE + 2;
    int lo = p.absY - (BH - PLAYER_LANE) - 2;
    for (int ay = lo; ay <= hi; ++ay) tickLane(w.laneAt(ay));

    Lane* cur = w.laneAt(p.absY);

    if (cur && cur->type == WATER) {
        Obstacle* log = findObs(cur, p.px);
        if (!log) { p.alive = false; return; }
        p.logAccum += log->speed;
        int drift   = (int)p.logAccum;
        p.logAccum -= drift;
        p.px       += drift;
        if (p.px < 0.0 || p.px >= BW) { p.alive = false; return; }
        if (!findObs(cur, p.px))       { p.alive = false; return; }
    } else {
        p.logAccum = 0.0;
    }

    if (cur && cur->type == ROAD) {
        if (findObs(cur, p.px)) p.alive = false;
    }

    if (p.jumpFrame > 0) p.jumpFrame = (p.jumpFrame + 1) % 5;
}

// ═══════════════════════════════════════════════════════════════════
//  3D Rendering Engine
// ═══════════════════════════════════════════════════════════════════
struct ScreenCell {
    std::string seq;
    bool        filled = false;
};

struct ScreenBuf {
    int rows, cols;
    std::vector<std::vector<ScreenCell>> cells;

    ScreenBuf(int r, int c) : rows(r), cols(c),
        cells(r, std::vector<ScreenCell>(c)) {}

    void set(int r, int c, const std::string& seq, bool fill=true) {
        if (r < 0 || r >= rows || c < 0 || c >= cols) return;
        cells[r][c].seq    = seq;
        cells[r][c].filled = fill;
    }

    void flush() {
        std::string out;
        out.reserve(rows * cols * 12);
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (cells[r][c].filled)
                    out += cells[r][c].seq;
                else
                    out += "\033[0m ";
            }
            out += "\033[0m\n";
        }
        std::cout << "\033[H" << out;
        std::cout.flush();
    }
};

void drawLane3D(ScreenBuf& buf,
                Lane* lane, LaneType type,
                int screenRow, int xOffset, int depth,
                const Player& p, int laneAbsY,
                bool playerOnThisLane)
{
    LanePalette pal = makePalette(type, depth);

    struct TileInfo {
        bool      hasObs    = false;
        int       obsVtype  = -1;
        bool      obsEdgeL  = false;
        bool      obsEdgeR  = false;
        bool      isPlayer  = false;
        bool      playerDead= false;
    };

    std::vector<TileInfo> tiles(BW);

    if (lane) {
        auto* cur = lane->obs.head;
        while (cur) {
            const Obstacle& o = cur->data;
            for (int c = o.iLeft(); c < o.iRight() && c < BW; ++c) {
                if (c < 0) continue;
                tiles[c].hasObs   = true;
                tiles[c].obsVtype = o.vtype;
                tiles[c].obsEdgeL = (c == o.iLeft());
                tiles[c].obsEdgeR = (c == o.iRight() - 1);
            }
            cur = cur->next;
        }
    }

    if (playerOnThisLane) {
        int pc = (int)p.px;
        if (pc >= 0 && pc < BW) {
            tiles[pc].isPlayer   = true;
            tiles[pc].playerDead = !p.alive;
        }
    }

    // Row 0: Top face
    for (int c = 0; c < BW; ++c) {
        int sc = c + xOffset;
        const TileInfo& ti = tiles[c];
        std::string cell;

        if (ti.isPlayer) {
            if (ti.playerDead) {
                cell = Term::sbg(52) + Term::sfg(196) + Term::sbold() + "\xe2\x96\x84";
            } else {
                int pColor = (p.jumpFrame > 0) ? 226 : 220;
                cell = Term::sbg(pColor) + Term::sfg(214) + "\xe2\x96\x84";
            }
        } else if (ti.hasObs) {
            if (ti.obsVtype == 0) {
                int carTopBg = (depth < 3) ? 250 : 244;
                int carTopFg = (depth < 3) ? 255 : 248;
                if (ti.obsEdgeL || ti.obsEdgeR)
                    cell = Term::sbg(carTopBg) + Term::sfg(carTopFg) + "\xe2\x96\x84";
                else
                    cell = Term::sbg(carTopBg - 2) + Term::sfg(carTopFg) + "\xe2\x96\x84";
            } else {
                int logBg = (depth < 3) ? 94 : 58;
                int logFg = (depth < 3) ? 136 : 100;
                if (ti.obsEdgeL || ti.obsEdgeR)
                    cell = Term::sbg(logBg) + Term::sfg(logFg + 6) + "\xe2\x96\x84";
                else {
                    int alt = (c % 2 == 0) ? logBg : (logBg - 1);
                    cell = Term::sbg(alt) + Term::sfg(logFg) + "\xe2\x96\x84";
                }
            }
        } else {
            std::string extra;
            switch (type) {
                case WATER: {
                    int wc  = (depth < 3) ? 51 : (depth < 6) ? 39 : 27;
                    int wbg = pal.top_bg;
                    if ((c + (int)(lane ? (lane->obs.head ? lane->obs.head->data.x * 0.3 : 0) : 0)) % 4 == 0)
                        extra = Term::sbg(wbg + 1) + Term::sfg(wc) + "\xe2\x96\x84";
                    else
                        extra = Term::sbg(wbg) + Term::sfg(wbg + 3) + "\xe2\x96\x84";
                    break;
                }
                case GRASS: {
                    int gbg = pal.top_bg;
                    int gfg = pal.top_fg;
                    extra = (c % 5 == 0)
                        ? Term::sbg(gbg) + Term::sfg(gfg + 2) + "\xe2\x96\x84"
                        : Term::sbg(gbg) + Term::sfg(gfg)     + "\xe2\x96\x84";
                    break;
                }
                case ROAD: {
                    int rbg = pal.top_bg;
                    int rfg = pal.top_fg;
                    extra = (c % 6 == 0)
                        ? Term::sbg(rbg + 1) + Term::sfg(rfg + 2) + "\xe2\x96\x84"
                        : Term::sbg(rbg)     + Term::sfg(rfg)     + "\xe2\x96\x84";
                    break;
                }
                case SAFE: {
                    int sbg2 = pal.top_bg;
                    int sfg2 = pal.top_fg;
                    extra = ((c + depth) % 3 == 0)
                        ? Term::sbg(sbg2 + 1) + Term::sfg(sfg2 + 2) + "\xe2\x96\x84"
                        : Term::sbg(sbg2)     + Term::sfg(sfg2)     + "\xe2\x96\x84";
                    break;
                }
            }
            cell = extra;
        }
        buf.set(screenRow, sc, cell);
    }

    // Row 1: Front face
    int fr = screenRow + 1;
    for (int c = 0; c < BW; ++c) {
        int sc = c + xOffset;
        const TileInfo& ti = tiles[c];
        std::string cell;

        if (ti.isPlayer) {
            if (ti.playerDead) {
                cell = Term::sbg(52) + Term::sfg(196) + Term::sbold() + "\xe2\x96\x93";
            } else {
                int bodyBg = (p.jumpFrame > 0) ? 214 : 208;
                cell = Term::sbg(bodyBg) + Term::sfg(220) + Term::sbold() + "\xe2\x96\x88";
            }
        } else if (ti.hasObs) {
            if (ti.obsVtype == 0) {
                int carFg, carBg;
                if (ti.obsEdgeL) {
                    carBg = (depth < 3) ? 226 : 178;
                    carFg = 255;
                    cell = Term::sbg(carBg) + Term::sfg(carFg) + "\xe2\x96\x93";
                } else if (ti.obsEdgeR) {
                    carBg = (depth < 3) ? 124 : 88;
                    carFg = 196;
                    cell = Term::sbg(carBg) + Term::sfg(carFg) + "\xe2\x96\x93";
                } else {
                    carBg = (depth < 3) ? 240 : 236;
                    carFg = (depth < 3) ? 250 : 244;
                    cell = Term::sbg(carBg) + Term::sfg(carFg) + "\xe2\x96\x88";
                }
            } else {
                int logFrontBg = (depth < 3) ? 130 : 94;
                int logFrontFg = (depth < 3) ? 172 : 130;
                if (ti.obsEdgeL || ti.obsEdgeR)
                    cell = Term::sbg(logFrontBg) + Term::sfg(logFrontFg + 6) + "\xe2\x96\x93";
                else
                    cell = Term::sbg(logFrontBg - (c%2)) + Term::sfg(logFrontFg) + "\xe2\x96\x92";
            }
        } else {
            switch (type) {
                case WATER: {
                    int wfbg = pal.front_bg;
                    int wffg = pal.front_fg;
                    cell = (c % 3 == 1)
                        ? Term::sbg(wfbg) + Term::sfg(wffg + 3) + "\xe2\x96\x92"
                        : Term::sbg(wfbg) + Term::sfg(wffg)     + "\xe2\x96\x88";
                    break;
                }
                case GRASS: {
                    int gfbg = pal.front_bg;
                    int gffg = pal.front_fg;
                    cell = (c % 4 == 0)
                        ? Term::sbg(gfbg - 1) + Term::sfg(gffg - 1) + "\xe2\x96\x92"
                        : Term::sbg(gfbg)     + Term::sfg(gffg)     + "\xe2\x96\x88";
                    break;
                }
                case ROAD: {
                    int rfbg = pal.front_bg;
                    int rffg = pal.front_fg;
                    cell = Term::sbg(rfbg) + Term::sfg(rffg) + "\xe2\x96\x88";
                    break;
                }
                case SAFE: {
                    int sfbg = pal.front_bg;
                    int sffg = pal.front_fg;
                    cell = (c % 3 == 0)
                        ? Term::sbg(sfbg + 1) + Term::sfg(sffg + 2) + "\xe2\x96\x91"
                        : Term::sbg(sfbg)     + Term::sfg(sffg)     + "\xe2\x96\x88";
                    break;
                }
            }
        }
        buf.set(fr, sc, cell);
    }

    // Row 2: Shadow strip
    int sr = screenRow + 2;
    for (int c = 0; c < BW; ++c) {
        int sc = c + xOffset;
        const TileInfo& ti = tiles[c];
        std::string cell;
        if (ti.isPlayer && p.alive)
            cell = Term::sbg(232) + Term::sfg(238) + "\xe2\x96\x91";
        else if (ti.hasObs)
            cell = Term::sbg(232) + Term::sfg(235) + "\xe2\x96\x91";
        else
            cell = Term::sbg(232) + Term::sfg(233) + " ";
        buf.set(sr, sc, cell);
    }
}

// ═══════════════════════════════════════════════════════════════════
//  HUD
// ═══════════════════════════════════════════════════════════════════
static const int HUD_ROWS = 3;

void drawHUD3D(ScreenBuf& buf, const Player& p, int hi) {
    {
        int col = 0;
        buf.set(0, col++, Term::sbg(234) + Term::sfg(240) + "\xe2\x94\x8c");
        for (int i = 0; i < SCR_W - 2; ++i)
            buf.set(0, col++, Term::sbg(234) + Term::sfg(238) + "\xe2\x94\x80");
        buf.set(0, col++, Term::sbg(234) + Term::sfg(240) + "\xe2\x94\x90");
    }

    {
        int col = 0;
        buf.set(1, col++, Term::sbg(234) + Term::sfg(240) + "\xe2\x94\x82");

        std::string title = " \xF0\x9F\x90\x94 CROSSY ROAD 3D  ";
        for (char c : title)
            buf.set(1, col++, Term::sbg(234) + Term::sfg(255) + Term::sbold() + std::string(1, c));

        // Player name in HUD
        std::string nameStr = "[" + p.name + "]";
        for (char c : nameStr)
            buf.set(1, col++, Term::sbg(234) + Term::sfg(226) + Term::sbold() + std::string(1, c));

        std::string scoreStr = "  SCORE:" + std::to_string(p.score)
                             + "  BEST:" + std::to_string(hi) + "  ";
        int used = 1 + (int)title.size() + (int)nameStr.size() + (int)scoreStr.size() + 1;
        int pad  = SCR_W - used;
        for (int i = 0; i < pad && col < SCR_W - 1; ++i)
            buf.set(1, col++, Term::sbg(234) + " ");

        for (char c : scoreStr)
            buf.set(1, col++, Term::sbg(234) + Term::sfg(248) + std::string(1, c));

        buf.set(1, SCR_W - 1, Term::sbg(234) + Term::sfg(240) + "\xe2\x94\x82");
    }

    {
        int col = 0;
        buf.set(2, col++, Term::sbg(234) + Term::sfg(240) + "\xe2\x94\x9c");
        for (int i = 0; i < SCR_W - 2; ++i)
            buf.set(2, col++, Term::sbg(234) + Term::sfg(237) + "\xe2\x94\x80");
        buf.set(2, col++, Term::sbg(234) + Term::sfg(240) + "\xe2\x94\xa4");
    }
}

void drawFooter3D(ScreenBuf& buf, int baseRow, bool dead) {
    {
        int col = 0;
        buf.set(baseRow, col++, Term::sbg(234) + Term::sfg(240) + "\xe2\x94\x94");
        for (int i = 0; i < SCR_W - 2; ++i)
            buf.set(baseRow, col++, Term::sbg(234) + Term::sfg(237) + "\xe2\x94\x80");
        buf.set(baseRow, col++, Term::sbg(234) + Term::sfg(240) + "\xe2\x94\x98");
    }

    {
        int col = 0;
        buf.set(baseRow + 1, col++, Term::sbg(232) + " ");

        std::string line;
        if (dead) {
            line = Term::sbg(88) + Term::sfg(255) + Term::sbold()
                 + "  \xe2\x95\x94\xe2\x95\x90\xe2\x95\x90 GAME OVER \xe2\x95\x90\xe2\x95\x90\xe2\x95\x97  "
                 + Term::sreset() + Term::sfg(242)
                 + "  R restart    L leaderboard    Q quit";
        } else {
            line = Term::sbg(232) + Term::sfg(240)
                 + "  W/\xe2\x86\x91 fwd  "
                 + "S/\xe2\x86\x93 back  "
                 + "A/\xe2\x86\x90 left  "
                 + "D/\xe2\x86\x92 right  "
                 + "L leaderboard  "
                 + "Q quit";
        }
        buf.set(baseRow + 1, 1, line);
        for (int c = 2; c < SCR_W; ++c)
            if (!buf.cells[baseRow + 1][c].filled)
                buf.set(baseRow + 1, c, Term::sbg(232) + " ");
    }
}

void drawDepthRail(ScreenBuf& buf, int startRow, int numLanes) {
    for (int lane = 0; lane < numLanes; ++lane) {
        int xOff = (numLanes - 1 - lane) * ISO_SKEW;
        int r    = startRow + lane * LANE_H;
        for (int row = 0; row < LANE_H - 1; ++row) {
            buf.set(r + row, xOff,
                Term::sbg(232) + Term::sfg(236) + "\xe2\x96\x8c");
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Full render
// ═══════════════════════════════════════════════════════════════════
void render3D(const World& w, const Player& p, int hi) {
    int totalRows = HUD_ROWS + PLAY_ROWS + 2;
    int totalCols = SCR_W + 2;

    ScreenBuf buf(totalRows, totalCols);

    for (int r = 0; r < totalRows; ++r)
        for (int c = 0; c < totalCols; ++c)
            buf.set(r, c, Term::sbg(232) + " ");

    drawHUD3D(buf, p, hi);

    for (int sl = 0; sl < BH; ++sl) {
        int absY    = p.absY + (PLAYER_LANE - sl);
        int xOffset = (BH - 1 - sl) * ISO_SKEW;
        int depth   = std::abs(sl - PLAYER_LANE);

        Lane* lane    = w.laneAt(absY);
        LaneType type = lane ? lane->type : SAFE;
        int screenRow = HUD_ROWS + sl * LANE_H;
        bool playerOnLane = (absY == p.absY);

        drawLane3D(buf, lane, type, screenRow, xOffset, depth, p, absY, playerOnLane);
    }

    drawDepthRail(buf, HUD_ROWS, BH);
    drawFooter3D(buf, HUD_ROWS + PLAY_ROWS, !p.alive);

    buf.flush();
}

// ═══════════════════════════════════════════════════════════════════
//  Input
// ═══════════════════════════════════════════════════════════════════
enum Key { K_NONE, K_UP, K_DOWN, K_LEFT, K_RIGHT, K_QUIT, K_RESTART, K_LEADERBOARD };

Key pollKey() {
    int k = Term::read_key();
    switch (k) {
        case 1000: case 'w': case 'W': return K_UP;
        case 1001: case 's': case 'S': return K_DOWN;
        case 1002: case 'd': case 'D': return K_RIGHT;
        case 1003: case 'a': case 'A': return K_LEFT;
        case 'q':  case 'Q':           return K_QUIT;
        case 'r':  case 'R':           return K_RESTART;
        case 'l':  case 'L':           return K_LEADERBOARD;
        default:                        return K_NONE;
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Name entry prompt
// ═══════════════════════════════════════════════════════════════════
std::string promptName() {
    Term::clear();
    std::cout << "\n\n";

    Term::fg(220); Term::bold();
    std::cout << "  ╔══════════════════════════════════════════════════════╗\n";
    std::cout << "  ║           🐔  CROSSY ROAD 3D  v4.0  🐔              ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════╝\n\n";
    Term::reset_attr();

    Term::fg(252);
    std::cout << "  Enter your name (max 18 chars, press ENTER):\n\n";
    Term::fg(226); Term::bold();
    std::cout << "  > ";
    Term::reset_attr();
    Term::show_cursor();

    std::string name = Term::readLine(18);
    if (name.empty()) name = "ANON";

    Term::hide_cursor();
    return name;
}

// ═══════════════════════════════════════════════════════════════════
//  Game Over + leaderboard integration
// ═══════════════════════════════════════════════════════════════════
void handleGameOver(Leaderboard& lb, Player& p, int& hiScore) {
    // Update hi score
    if (p.score > hiScore) hiScore = p.score;

    // Check leaderboard qualification
    bool newEntry = false;
    if (lb.qualifies(p.score) && p.score > 0) {
        LeaderEntry e;
        e.name  = p.name;
        e.score = p.score;
        e.date  = todayStr();
        lb.insert(e);
        lb.save();
        newEntry = true;
    }

    // Show leaderboard
    Term::restore();
    Term::show_cursor();
    showLeaderboard(lb, p.name, p.score, newEntry);

    if (newEntry) {
        int rank = lb.rankOf(p.score);
        Term::fg(226); Term::bold();
        std::cout << "  🎉 NEW LEADERBOARD ENTRY!  Rank #" << rank << "\n\n";
        Term::reset_attr();
    }

    Term::fg(252);
    std::cout << "  Your score: ";
    Term::fg(255); Term::bold();
    std::cout << p.score << "\n";
    Term::reset_attr();
    Term::fg(240);
    std::cout << "\n  Press R to play again, Q to quit...\n\n";
    Term::reset_attr();
}

// ═══════════════════════════════════════════════════════════════════
//  Splash screen
// ═══════════════════════════════════════════════════════════════════
void splash() {
    Term::clear();

    std::string title[] = {
        " ██████╗██████╗  ██████╗ ███████╗███████╗██╗   ██╗",
        "██╔════╝██╔══██╗██╔═══██╗██╔════╝██╔════╝╚██╗ ██╔╝",
        "██║     ██████╔╝██║   ██║███████╗███████╗ ╚████╔╝ ",
        "██║     ██╔══██╗██║   ██║╚════██║╚════██║  ╚██╔╝  ",
        "╚██████╗██║  ██║╚██████╔╝███████║███████║   ██║   ",
        " ╚═════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚══════╝   ╚═╝  "
    };
    std::string title2[] = {
        "██████╗  ██████╗  █████╗ ██████╗     ██████╗ ██████╗ ",
        "██╔══██╗██╔═══██╗██╔══██╗██╔══██╗    ╚════██╗██╔══██╗",
        "██████╔╝██║   ██║███████║██║  ██║     █████╔╝██║  ██║",
        "██╔══██╗██║   ██║██╔══██║██║  ██║     ╚═══██╗██║  ██║",
        "██║  ██║╚██████╔╝██║  ██║██████╔╝    ██████╔╝██████╔╝",
        "╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚═════╝     ╚═════╝ ╚═════╝ "
    };

    int colors[] = {196, 202, 208, 214, 220, 226};
    for (int i = 0; i < 6; ++i) {
        Term::fg(colors[i]);
        std::cout << "  " << title[i] << "\n";
    }
    std::cout << "\n";
    for (int i = 0; i < 6; ++i) {
        Term::fg(colors[5 - i]);
        std::cout << "  " << title2[i] << "\n";
    }

    Term::reset_attr();
    std::cout << "\n";

    Term::fg(245);
    std::cout << "  ┌──────────────────────────────────────────────────────┐\n";
    std::cout << "  │  C++ Terminal Edition v4.0  |  3D Isometric ASCII     │\n";
    std::cout << "  │  Linked-List Architecture   |  Persistent Leaderboard │\n";
    std::cout << "  └──────────────────────────────────────────────────────┘\n\n";

    Term::fg(240);
    std::cout << "  ┌─ CONTROLS ──────────────────────────────────────────┐\n";
    Term::fg(252);
    std::cout << "  │  W / ↑  │  Move forward                             │\n";
    std::cout << "  │  S / ↓  │  Move back                                │\n";
    std::cout << "  │  A / ←  │  Move left                                │\n";
    std::cout << "  │  D / →  │  Move right                               │\n";
    std::cout << "  │  L      │  View leaderboard anytime                 │\n";
    std::cout << "  │  R      │  Restart after death                      │\n";
    std::cout << "  │  Q      │  Quit                                     │\n";
    Term::fg(240);
    std::cout << "  └─────────────────────────────────────────────────────┘\n\n";

    Term::fg(244);
    std::cout << "  Press ENTER to continue ...\n\n";
    Term::reset_attr();
    std::cin.ignore(10000, '\n');
}

// ═══════════════════════════════════════════════════════════════════
//  Game loop
// ═══════════════════════════════════════════════════════════════════
void runGame(const std::string& playerName, Leaderboard& lb) {
    World  world;
    Player player;
    player.name = playerName;
    int    hiScore = 0;
    bool   quit    = false;
    bool   showLB  = false;

    buildInitialWorld(world);
    ensureLanes(world, player.absY);

    using Clock = std::chrono::steady_clock;
    using MS    = std::chrono::milliseconds;

    const long TICK_MS  = 90;
    const long FRAME_MS = 50;

    auto lastTick  = Clock::now();
    auto lastFrame = Clock::now();

    Term::raw();
    Term::hide_cursor();
    Term::clear();
    render3D(world, player, hiScore);

    while (!quit) {
        auto now = Clock::now();

        Key key = pollKey();
        int dx = 0, dy = 0;
        bool moved = false;

        switch (key) {
            case K_QUIT:
                quit = true;
                continue;

            case K_LEADERBOARD:
                showLB = true;
                break;

            case K_RESTART:
                if (!player.alive) {
                    // Handle game over + leaderboard before restarting
                    Term::show_cursor();
                    Term::restore();
                    handleGameOver(lb, player, hiScore);

                    // Wait for R or Q
                    bool decided = false;
                    while (!decided) {
                        int k = std::cin.get();
                        if (k == 'r' || k == 'R') decided = true;
                        else if (k == 'q' || k == 'Q') { quit = true; decided = true; }
                    }
                    if (quit) continue;

                    { World tmp; std::swap(world, tmp); }
                    s_laneGen = 0;
                    std::string savedName = player.name;
                    player    = Player();
                    player.name = savedName;
                    buildInitialWorld(world);
                    ensureLanes(world, player.absY);
                    Term::raw();
                    Term::hide_cursor();
                    Term::clear();
                }
                continue;

            case K_UP:    dy =  1; moved = true; break;
            case K_DOWN:  dy = -1; moved = true; break;
            case K_LEFT:  dx = -1; moved = true; break;
            case K_RIGHT: dx =  1; moved = true; break;
            default: break;
        }

        // Mid-game leaderboard view
        if (showLB) {
            showLB = false;
            Term::show_cursor();
            Term::restore();
            showLeaderboard(lb, player.name, player.score, false);
            Term::fg(240);
            std::cout << "  Press any key to return to game...\n\n";
            Term::reset_attr();
            std::cin.get();
            Term::raw();
            Term::hide_cursor();
            Term::clear();
            render3D(world, player, hiScore);
            continue;
        }

        if (player.alive && moved) {
            tryMove(world, player, dx, dy);
            ensureLanes(world, player.absY);
        }

        long tickMs = std::chrono::duration_cast<MS>(now - lastTick).count();
        if (tickMs >= TICK_MS) {
            lastTick = now;
            physTick(world, player);
            ensureLanes(world, player.absY);
            if (player.score > hiScore) hiScore = player.score;
        }

        long frameMs = std::chrono::duration_cast<MS>(now - lastFrame).count();
        if (frameMs >= FRAME_MS) {
            lastFrame = now;
            render3D(world, player, hiScore);
        }

        std::this_thread::sleep_for(MS(8));
    }

    // On normal quit, still submit score if non-zero
    if (!player.alive || player.score > 0) {
        if (lb.qualifies(player.score) && player.score > 0) {
            LeaderEntry e;
            e.name  = player.name;
            e.score = player.score;
            e.date  = todayStr();
            lb.insert(e);
            lb.save();
        }
        if (player.score > hiScore) hiScore = player.score;
    }

    Term::show_cursor();
    Term::restore();
    Term::clear();
    std::cout << "\n";

    Term::fg(220); Term::bold();
    std::cout << "  ╔════════════════════════════════╗\n";
    std::cout << "  ║   CROSSY ROAD 3D  — C++ v4.0  ║\n";
    std::cout << "  ╠════════════════════════════════╣\n";
    Term::reset_attr(); Term::fg(252);
    std::cout << "  ║  Player: " << player.name;
    int namePad = 20 - (int)player.name.size();
    for (int i=0; i<namePad; ++i) std::cout << ' ';
    std::cout << "║\n";
    std::cout << "  ║  Score : " << player.score;
    int pad = 21 - (int)std::to_string(player.score).size();
    for (int i=0;i<pad;i++) std::cout << ' ';
    std::cout << "║\n";
    std::cout << "  ║  Best  : " << hiScore;
    pad = 21 - (int)std::to_string(hiScore).size();
    for (int i=0;i<pad;i++) std::cout << ' ';
    std::cout << "║\n";
    Term::fg(220);
    std::cout << "  ╚════════════════════════════════╝\n\n";
    Term::reset_attr();
}

// ═══════════════════════════════════════════════════════════════════
//  Entry point
// ═══════════════════════════════════════════════════════════════════
static void cleanup(int) {
    Term::show_cursor();
    Term::restore();
    exit(0);
}

int main() {
    signal(SIGINT,  cleanup);
    signal(SIGTERM, cleanup);

    srand((unsigned)time(nullptr));

    // Load persistent leaderboard
    Leaderboard lb;
    lb.load();

    // Show splash
    splash();

    // Ask for name
    std::string name = promptName();

    // Play game
    runGame(name, lb);

    return 0;
}
