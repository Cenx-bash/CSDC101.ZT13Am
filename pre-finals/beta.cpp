 /*
 ╔══════════════════════════════════════════════════════════════════╗
 ║             CROSSY ROAD — C++ Terminal Edition  v2.0            ║
 ║        Linked-List Architecture | Monotone ASCII UI              ║
 ╚══════════════════════════════════════════════════════════════════╝

  Data structures
  ───────────────
  SLinkedList<T>  generic singly-linked list (obstacles per lane)
  World           doubly-linked list of Lane nodes (infinite scroll)

  Coordinate model
  ────────────────
  absY  — player's absolute lane index.  0 = spawn.  Higher = forward.
  The world's doubly-linked list stores lanes:
    head = topmost on screen (highest absY, most-advanced lane)
    tail = bottommost on screen (lowest absY, spawn side)
  baseY = absY of the tail lane.
  headAbsY = baseY - (count-1).
*/

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <thread>
#include <termios.h>
#include <unistd.h>
#include <csignal>

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
    void reset()       { std::cout << "\033[0m";        }
    void bold()        { std::cout << "\033[1m";        }
    void fg(int n)     { std::cout << "\033[38;5;" << n << "m"; }
    void bg(int n)     { std::cout << "\033[48;5;" << n << "m"; }

    // Read a full escape sequence in one shot (non-blocking)
    int read_key() {
        unsigned char buf[8] = {};
        int n = (int)read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) return 0;
        // Arrow keys: ESC [ A/B/C/D
        if (n >= 3 && buf[0] == 27 && buf[1] == '[') {
            if (buf[2] == 'A') return 1000; // UP
            if (buf[2] == 'B') return 1001; // DOWN
            if (buf[2] == 'C') return 1002; // RIGHT
            if (buf[2] == 'D') return 1003; // LEFT
        }
        return (int)buf[0];
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Constants
// ═══════════════════════════════════════════════════════════════════
static const int BW           = 56;  // play-area columns
static const int BH           = 20;  // visible rows
static const int PLAYER_SROW  = BH - 4; // player fixed screen row
static const int AHEAD_BUFFER = BH + 4;
static const int BELOW_BUFFER = BH + 4;

enum LaneType { SAFE, GRASS, ROAD, WATER };

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
//  Obstacle
// ═══════════════════════════════════════════════════════════════════
struct Obstacle {
    double x;       // left-edge column (fractional)
    int    width;
    double speed;   // columns per tick, signed
    char   glyph;

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
    int                   shade;  // 232-255
    Lane*                 prev = nullptr;
    Lane*                 next = nullptr;

    Lane(LaneType t, int s) : type(t), shade(s) {}
    Lane(const Lane&)            = delete;
    Lane& operator=(const Lane&) = delete;
};

// ═══════════════════════════════════════════════════════════════════
//  World — doubly-linked lane list
//   head = highest absY (top of screen, most advanced)
//   tail = lowest  absY (bottom of screen, spawn side)
//   baseY = absY of tail
// ═══════════════════════════════════════════════════════════════════
struct World {
    Lane* head  = nullptr;
    Lane* tail  = nullptr;
    int   count = 0;
    int   baseY = -1; // absY of tail; -1 = empty

    ~World() {
        Lane* cur = head;
        while (cur) { Lane* n = cur->next; delete cur; cur = n; }
    }

    int headAbsY() const { return (count > 0) ? baseY + (count - 1) : -1; }

    // Push a new lane that is more advanced (higher absY) — goes to head
    void pushTop(Lane* l) {
        l->next = head; l->prev = nullptr;
        if (head) head->prev = l; else tail = l;
        head = l; ++count;
        if (count == 1) baseY = 0;
        // headAbsY increases by 1; we don't change baseY
    }

    // Pop the least-advanced lane (tail) — bottom of screen
    void popBottom() {
        if (!tail) return;
        Lane* t = tail;
        tail = t->prev;
        if (tail) tail->next = nullptr; else head = nullptr;
        delete t; --count; ++baseY; // baseY rises (tail was lowest)
    }

    // Get lane at absolute Y.  O(distance from nearest end).
    Lane* laneAt(int absY) const {
        if (count == 0) return nullptr;
        int tailAbsY = baseY;
        int headAY   = tailAbsY + (count - 1);
        if (absY < tailAbsY || absY > headAY) return nullptr;
        int fromTail = absY - tailAbsY; // 0 = tail
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
static int    rng (int lo, int hi) { return lo + rand() % (hi - lo + 1); }
static double rngf(double lo, double hi) {
    return lo + (hi-lo)*(rand()/(double)RAND_MAX);
}

// ═══════════════════════════════════════════════════════════════════
//  Lane factory
// ═══════════════════════════════════════════════════════════════════
static int s_laneGen = 0; // monotonically increasing lane counter

Lane* makeLane(int idx) {
    // Pattern repeating every 16 rows:
    //  0      SAFE   (spawn pad)
    //  1-2    GRASS
    //  3-6    ROAD
    //  7      SAFE   (breather)
    //  8      GRASS
    //  9-12   WATER
    //  13-14  GRASS
    //  15     SAFE
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
        double carW = rng(3, 5);
        double minGap = 8.0;
        double pos = rngf(0.0, 4.0);

        for (int i = 0; i < numCars; ++i) {
            Obstacle o;
            o.x     = pos;
            o.width = (int)carW;
            o.speed = spd;
            o.glyph = '=';
            l->obs.push_back(o);
            pos += carW + minGap + rngf(0, 6);
            // Wrap seed if past board (they'll wrap at runtime)
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
            o.glyph = '#';
            l->obs.push_back(o);
            pos += logW + gap;
            if (pos > BW) pos -= BW; // wrap placement seed
        }
    }

    return l;
}

// ─── Seed initial world around spawn ──────────────────────────────
void buildInitialWorld(World& w) {
    s_laneGen = 0;
    // Push lanes from absY=0 (tail/spawn) upward to AHEAD_BUFFER
    // First push is absY=0 (spawn, SAFE), it becomes both head and tail (baseY=0).
    // Each subsequent pushTop adds a lane with absY = previous headAbsY + 1.

    // Push the bottom/spawn lane first (becomes tail, baseY=0)
    {
        Lane* l = makeLane(s_laneGen++);
        w.head = w.tail = l;
        w.count = 1;
        w.baseY = 0;
    }
    // Push lanes above (higher absY) to head
    for (int i = 1; i <= AHEAD_BUFFER; ++i) {
        Lane* l = makeLane(s_laneGen++);
        // pushTop: goes to head, headAbsY = baseY + count
        l->next = w.head; l->prev = nullptr;
        if (w.head) w.head->prev = l;
        w.head = l;
        ++w.count;
    }
    // Also push lanes below spawn (negative absY — they won't be visited
    // by the player but prevent null lookups when camera is at spawn)
    // We just track baseY correctly.  Spawn is absY=0, tail is absY=0,
    // no need to push further down.
}

// ─── Ensure enough lanes around player ────────────────────────────
void ensureLanes(World& w, int playerAbsY) {
    int needed_high = playerAbsY + AHEAD_BUFFER;
    int needed_low  = playerAbsY - BELOW_BUFFER;

    // Grow upward (toward head)
    while (w.headAbsY() < needed_high) {
        Lane* l = makeLane(s_laneGen++);
        l->next = w.head; l->prev = nullptr;
        if (w.head) w.head->prev = l; else w.tail = l;
        w.head = l; ++w.count;
    }

    // Cull bottom lanes that are too far below player
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
    double px      = BW / 2.0;
    int    absY    = 0;
    bool   alive   = true;
    int    score   = 0;
    double logAccum= 0.0;  // sub-pixel accumulator for log drift
};

// ═══════════════════════════════════════════════════════════════════
//  Movement
// ═══════════════════════════════════════════════════════════════════
void tryMove(World& w, Player& p, int dx, int dy) {
    double newPx = p.px + dx;
    int newAbsY  = p.absY + dy;

    if (newPx < 0.0)       newPx = 0.0;
    if (newPx > BW - 1.0)  newPx = BW - 1.0;

    Lane* tgt = w.laneAt(newAbsY);
    if (!tgt) return;

    if (tgt->type == ROAD) {
        Obstacle* car = findObs(tgt, newPx);
        p.px   = newPx;
        p.absY = newAbsY;
        p.logAccum = 0.0;
        if (car) p.alive = false;
    } else if (tgt->type == WATER) {
        Obstacle* log = findObs(tgt, newPx);
        p.px   = newPx;
        p.absY = newAbsY;
        p.logAccum = 0.0;
        if (!log) p.alive = false;
    } else {
        p.px   = newPx;
        p.absY = newAbsY;
        p.logAccum = 0.0;
    }

    if (p.absY > p.score) p.score = p.absY;
}

void physTick(World& w, Player& p) {
    if (!p.alive) return;

    // Advance all visible lanes
    int hi = p.absY + PLAYER_SROW + 2;
    int lo = p.absY - (BH - PLAYER_SROW) - 2;
    for (int ay = lo; ay <= hi; ++ay) tickLane(w.laneAt(ay));

    Lane* cur = w.laneAt(p.absY);

    // Log drift
    if (cur && cur->type == WATER) {
        Obstacle* log = findObs(cur, p.px);
        if (!log) { p.alive = false; return; }
        p.logAccum += log->speed;
        int drift   = (int)p.logAccum;
        p.logAccum -= drift;
        p.px       += drift;
        // Clamp and re-check
        if (p.px < 0.0 || p.px >= BW) { p.alive = false; return; }
        if (!findObs(cur, p.px))      { p.alive = false; return; }
    } else {
        p.logAccum = 0.0;
    }

    // Car driven into player
    if (cur && cur->type == ROAD) {
        if (findObs(cur, p.px)) p.alive = false;
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Rendering
// ═══════════════════════════════════════════════════════════════════
void drawHUD(const Player& p, int hi) {
    // Border top
    Term::fg(240); Term::bg(232);
    std::cout << "\xE2\x94\x8C";
    for (int i = 0; i < BW; ++i) std::cout << "\xE2\x94\x80";
    std::cout << "\xE2\x94\x90";
    Term::reset(); std::cout << '\n';

    // Title / score line
    Term::bg(232); Term::fg(240);
    std::cout << "\xE2\x94\x82";
    Term::fg(255); Term::bold();
    std::string L = " CROSSY ROAD  C++";
    std::cout << L;
    Term::reset();
    std::string R = "SCORE:" + std::to_string(p.score)
                  + "  BEST:" + std::to_string(hi) + " ";
    int sp = BW - (int)L.size() - (int)R.size();
    for (int i = 0; i < sp; ++i) std::cout << ' ';
    Term::bg(232); Term::fg(248);
    std::cout << R;
    Term::bg(232); Term::fg(240);
    std::cout << "\xE2\x94\x82";
    Term::reset(); std::cout << '\n';

    // Separator
    Term::fg(238); Term::bg(232);
    std::cout << "\xE2\x94\x9C";
    for (int i = 0; i < BW; ++i) std::cout << "\xE2\x94\x80";
    std::cout << "\xE2\x94\xA4";
    Term::reset(); std::cout << '\n';
}

void drawFooter(bool dead) {
    Term::fg(238); Term::bg(232);
    std::cout << "\xE2\x94\x94";
    for (int i = 0; i < BW; ++i) std::cout << "\xE2\x94\x80";
    std::cout << "\xE2\x94\x98";
    Term::reset(); std::cout << '\n';

    if (dead) {
        Term::bold(); Term::fg(255);
        std::cout << "  \xE2\x95\x94\xE2\x95\x90\xE2\x95\x90 GAME OVER \xE2\x95\x90\xE2\x95\x90\xE2\x95\x97";
        Term::reset(); Term::fg(242);
        std::cout << "   R restart    Q quit                 \n";
    } else {
        Term::fg(238);
        std::cout << "  W/\xe2\x86\x91 fwd  S/\xe2\x86\x93 back  A/\xe2\x86\x90 left  D/\xe2\x86\x92 right  Q quit\n";
    }
    Term::reset();
}

void drawRow(const World& w, const Player& p, int srow) {
    // absY of this screen row
    // srow 0 = top of screen = highest absY = player.absY + PLAYER_SROW
    int absY = p.absY + (PLAYER_SROW - srow);
    Lane* lane = w.laneAt(absY);

    char bgCh  = ' ';
    int  shade = 235;
    if (lane) {
        shade = lane->shade;
        switch (lane->type) {
            case ROAD:  bgCh = '.'; break;
            case WATER: bgCh = '~'; break;
            case GRASS: bgCh = ' '; break;
            case SAFE:  bgCh = '_'; break;
        }
    }

    // Cell buffer
    struct Cell { char ch; int fg; bool b; };
    std::vector<Cell> cells(BW, {bgCh, shade + 1, false});

    // Shade background chars by type
    if (lane) {
        int tfg;
        switch (lane->type) {
            case ROAD:  tfg = 236; break;
            case WATER: tfg = 234; break;
            case GRASS: tfg = 237; break;
            case SAFE:  tfg = 238; break;
            default:    tfg = 236;
        }
        for (auto& c : cells) c.fg = tfg;
    }

    // Obstacles
    if (lane) {
        auto* cur = lane->obs.head;
        while (cur) {
            const Obstacle& o = cur->data;
            int ofg = (lane->type == ROAD) ? 250 : 244;
            for (int c = o.iLeft(); c < o.iRight(); ++c)
                if (c >= 0 && c < BW) cells[c] = {o.glyph, ofg, false};
            cur = cur->next;
        }
    }

    // Player
    if (srow == PLAYER_SROW) {
        int pc = (int)p.px;
        if (pc < 0)  pc = 0;
        if (pc >= BW) pc = BW-1;
        if (!p.alive)
            cells[pc] = {'X', 240, false};
        else
            cells[pc] = {'@', 255, true};
    }

    // Emit
    Term::bg(232); Term::fg(238);
    std::cout << "\xE2\x94\x82";

    int lastFg = -1; bool lastB = false;
    for (int c = 0; c < BW; ++c) {
        const Cell& cl = cells[c];
        Term::bg(shade);
        if (cl.b != lastB) {
            if (cl.b) Term::bold();
            else      { Term::reset(); Term::bg(shade); }
            lastFg = -1; lastB = cl.b;
        }
        if (cl.fg != lastFg) { Term::fg(cl.fg); lastFg = cl.fg; }
        std::cout << cl.ch;
    }

    Term::reset();
    Term::bg(232); Term::fg(238);
    std::cout << "\xE2\x94\x82";
    Term::reset(); std::cout << '\n';
}

void render(const World& w, const Player& p, int hi) {
    Term::home();
    drawHUD(p, hi);
    for (int r = 0; r < BH; ++r) drawRow(w, p, r);
    drawFooter(!p.alive);
    std::cout.flush();
}

// ═══════════════════════════════════════════════════════════════════
//  Input
// ═══════════════════════════════════════════════════════════════════
enum Key { K_NONE, K_UP, K_DOWN, K_LEFT, K_RIGHT, K_QUIT, K_RESTART };

Key pollKey() {
    int k = Term::read_key();
    switch (k) {
        case 1000: case 'w': case 'W': return K_UP;
        case 1001: case 's': case 'S': return K_DOWN;
        case 1002: case 'd': case 'D': return K_RIGHT;
        case 1003: case 'a': case 'A': return K_LEFT;
        case 'q':  case 'Q':           return K_QUIT;
        case 'r':  case 'R':           return K_RESTART;
        default:                        return K_NONE;
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Splash
// ═══════════════════════════════════════════════════════════════════
void splash() {
    Term::clear();
    Term::fg(255); Term::bold();
    std::cout <<
"\n"
"  \xE2\x95\x94\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x97\n"
"  \xE2\x95\x91                                                            \xE2\x95\x91\n"
"  \xE2\x95\x91   CROSSY ROAD  -  C++ Terminal Edition  v2.0               \xE2\x95\x91\n"
"  \xE2\x95\x91   Linked-List Architecture  |  Monotone ASCII UI            \xE2\x95\x91\n"
"  \xE2\x95\x91                                                            \xE2\x95\x91\n"
"  \xE2\x95\xa0\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\xa3\n";
    Term::reset();
    std::cout <<
"  \xE2\x95\x91                                                            \xE2\x95\x91\n"
"  \xE2\x95\x91  CONTROLS                                                  \xE2\x95\x91\n"
"  \xE2\x95\x91    W / Up    Move forward (advance lane)                   \xE2\x95\x91\n"
"  \xE2\x95\x91    S / Down  Move back                                     \xE2\x95\x91\n"
"  \xE2\x95\x91    A / Left  Move left                                     \xE2\x95\x91\n"
"  \xE2\x95\x91    D / Right Move right                                    \xE2\x95\x91\n"
"  \xE2\x95\x91    R         Restart after death                           \xE2\x95\x91\n"
"  \xE2\x95\x91    Q         Quit                                          \xE2\x95\x91\n"
"  \xE2\x95\x91                                                            \xE2\x95\x91\n"
"  \xE2\x95\x91  LEGEND                                                    \xE2\x95\x91\n"
"  \xE2\x95\x91    @    You (the chicken)                                  \xE2\x95\x91\n"
"  \xE2\x95\x91    ===  Car  - dodge on road lanes (...)                   \xE2\x95\x91\n"
"  \xE2\x95\x91    ###  Log  - ride across water lanes (~~~)               \xE2\x95\x91\n"
"  \xE2\x95\x91    ___  Safe zone - always safe                            \xE2\x95\x91\n"
"  \xE2\x95\x91                                                            \xE2\x95\x91\n"
"  \xE2\x95\x91  DATA STRUCTURES                                           \xE2\x95\x91\n"
"  \xE2\x95\x91    SLinkedList<Obstacle>  singly-linked per-lane obs list  \xE2\x95\x91\n"
"  \xE2\x95\x91    World                  doubly-linked lane list           \xE2\x95\x91\n"
"  \xE2\x95\x91    pushTop / popBottom    O(1) infinite world scroll        \xE2\x95\x91\n"
"  \xE2\x95\x91    laneAt(absY)           O(n/2) mid-point walk            \xE2\x95\x91\n"
"  \xE2\x95\x91                                                            \xE2\x95\x91\n"
"  \xE2\x95\x9A\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x9D\n\n";

    Term::fg(244);
    std::cout << "  Press ENTER to start ...\n\n";
    Term::reset();
    std::cin.ignore(10000, '\n');
}

// ═══════════════════════════════════════════════════════════════════
//  Game loop
// ═══════════════════════════════════════════════════════════════════
void runGame() {
    srand((unsigned)time(nullptr));

    World  world;
    Player player;
    int    hiScore = 0;
    bool   quit    = false;

    buildInitialWorld(world);
    ensureLanes(world, player.absY);

    using Clock = std::chrono::steady_clock;
    using MS    = std::chrono::milliseconds;

    const long TICK_MS  = 90;  // physics  (~11 Hz)
    const long FRAME_MS = 33;  // render   (~30 fps)

    auto lastTick  = Clock::now();
    auto lastFrame = Clock::now();

    Term::raw();
    Term::hide_cursor();
    Term::clear();
    render(world, player, hiScore);

    while (!quit) {
        auto now = Clock::now();

        // ── Input ─────────────────────────────────────────────────
        Key key = pollKey();
        int dx = 0, dy = 0;
        bool moved = false;

        switch (key) {
            case K_QUIT: quit = true; continue;
            case K_RESTART:
                if (!player.alive) {
                    { World tmp; std::swap(world, tmp); }
                    s_laneGen = 0;
                    player    = Player();
                    buildInitialWorld(world);
                    ensureLanes(world, player.absY);
                    Term::clear();
                }
                continue;
            case K_UP:    dy =  1; moved = true; break;
            case K_DOWN:  dy = -1; moved = true; break;
            case K_LEFT:  dx = -1; moved = true; break;
            case K_RIGHT: dx =  1; moved = true; break;
            default: break;
        }

        if (player.alive && moved) {
            tryMove(world, player, dx, dy);
            ensureLanes(world, player.absY);
        }

        // ── Physics tick ──────────────────────────────────────────
        long tickMs = std::chrono::duration_cast<MS>(now - lastTick).count();
        if (tickMs >= TICK_MS) {
            lastTick = now;
            physTick(world, player);
            ensureLanes(world, player.absY);
            if (player.score > hiScore) hiScore = player.score;
        }

        // ── Render ────────────────────────────────────────────────
        long frameMs = std::chrono::duration_cast<MS>(now - lastFrame).count();
        if (frameMs >= FRAME_MS) {
            lastFrame = now;
            render(world, player, hiScore);
        }

        std::this_thread::sleep_for(MS(8));
    }

    Term::show_cursor();
    Term::restore();
    Term::clear();
    std::cout << "\n  CROSSY ROAD — C++ v2.0\n"
              << "  Score : " << player.score << "\n"
              << "  Best  : " << hiScore      << "\n\n";
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
    splash();
    runGame();
    return 0;
}
