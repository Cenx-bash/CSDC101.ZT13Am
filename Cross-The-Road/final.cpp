/*
 ╔══════════════════════════════════════════════════════════════════════╗
 ║         CROSSY ROAD — C++ Terminal Edition  v4.0  🐔               ║
 ║    Pseudo-3D Isometric ASCII  |  Linked-List Architecture            ║
 ║    Leaderboard System         |  Player Name Support                 ║
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
//  SECTION 1: Terminal Helpers
//  Low-level terminal I/O: raw mode, ANSI colors, cursor control
// ═══════════════════════════════════════════════════════════════════
namespace Term {
    struct termios orig_attr;
    bool raw_active = false;

    // Enable raw mode: disable line buffering and echo
    void raw() {
        tcgetattr(STDIN_FILENO, &orig_attr);
        struct termios t = orig_attr;
        t.c_lflag &= ~(ICANON | ECHO);
        t.c_cc[VMIN]  = 0;
        t.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        raw_active = true;
    }

    // Restore original terminal settings
    void restore() {
        if (raw_active) {
            tcsetattr(STDIN_FILENO, TCSANOW, &orig_attr);
            raw_active = false;
        }
    }

    // Cursor and screen control
    void clear()       { std::cout << "\033[2J\033[H"; }
    void home()        { std::cout << "\033[H";         }
    void hide_cursor() { std::cout << "\033[?25l";      }
    void show_cursor() { std::cout << "\033[?25h";      }
    void reset_attr()  { std::cout << "\033[0m";        }
    void bold()        { std::cout << "\033[1m";        }
    void fg(int n)     { std::cout << "\033[38;5;" << n << "m"; }
    void bg(int n)     { std::cout << "\033[48;5;" << n << "m"; }
    void move(int r, int c) { std::cout << "\033[" << r << ";" << c << "H"; }

    // String-returning ANSI helpers (for building buffered output)
    std::string sfg(int n)   { return "\033[38;5;" + std::to_string(n) + "m"; }
    std::string sbg(int n)   { return "\033[48;5;" + std::to_string(n) + "m"; }
    std::string sreset()     { return "\033[0m"; }
    std::string sbold()      { return "\033[1m"; }

    // Non-blocking key read; returns special codes for arrow keys
    int read_key() {
        unsigned char buf[8] = {};
        int n = (int)read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) return 0;
        // Detect ANSI escape sequences for arrow keys
        if (n >= 3 && buf[0] == 27 && buf[1] == '[') {
            if (buf[2] == 'A') return 1000; // Up
            if (buf[2] == 'B') return 1001; // Down
            if (buf[2] == 'C') return 1002; // Right
            if (buf[2] == 'D') return 1003; // Left
        }
        return (int)buf[0];
    }
}

// ═══════════════════════════════════════════════════════════════════
//  SECTION 2: Game Constants
// ═══════════════════════════════════════════════════════════════════

static const int BW           = 40;   // Board width in tiles
static const int BH           = 12;   // Number of lanes visible on screen
static const int LANE_H       = 3;    // Terminal rows per lane (top + front + shadow)
static const int PLAY_ROWS    = BH * LANE_H;   // Total rows in play area
static const int ISO_SKEW     = 1;    // Isometric perspective offset per depth step
static const int SCR_W        = BW + BH * ISO_SKEW + 4;  // Total screen columns
static const int PLAYER_LANE  = BH - 3;  // Player's fixed screen row (from top)
static const int AHEAD_BUFFER = BH + 4;  // Lanes generated ahead of player
static const int BELOW_BUFFER = BH + 4;  // Lanes retained behind player

// Leaderboard settings
static const std::string LEADERBOARD_FILE = "leaderboard.csv";
static const int MAX_LEADERBOARD_ENTRIES  = 10;

// Lane type determines terrain behavior and appearance
enum LaneType { SAFE, GRASS, ROAD, WATER };

// ═══════════════════════════════════════════════════════════════════
//  SECTION 3: Leaderboard System
//  Stores player names + scores, sorted by score descending.
//  Persists data to a CSV file between sessions.
// ═══════════════════════════════════════════════════════════════════

struct LeaderboardEntry {
    std::string name;
    int         score;
};

// Load leaderboard from CSV file (format: "name,score" per line)
std::vector<LeaderboardEntry> loadLeaderboard() {
    std::vector<LeaderboardEntry> entries;
    std::ifstream file(LEADERBOARD_FILE);
    if (!file.is_open()) return entries; // No file yet — return empty

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        // Find the last comma to split name and score
        auto pos = line.rfind(',');
        if (pos == std::string::npos) continue;

        LeaderboardEntry e;
        e.name  = line.substr(0, pos);
        try {
            e.score = std::stoi(line.substr(pos + 1));
        } catch (...) {
            continue; // Skip malformed lines
        }
        entries.push_back(e);
    }
    return entries;
}

// Save leaderboard to CSV file
void saveLeaderboard(const std::vector<LeaderboardEntry>& entries) {
    std::ofstream file(LEADERBOARD_FILE);
    if (!file.is_open()) return;
    for (const auto& e : entries) {
        file << e.name << "," << e.score << "\n";
    }
}

// Add a new score to the leaderboard, keep top MAX_LEADERBOARD_ENTRIES
// Returns the rank achieved (1-based), or -1 if not in top entries
int submitScore(const std::string& name, int score) {
    auto entries = loadLeaderboard();

    // Add new entry
    entries.push_back({name, score});

    // Sort descending by score
    std::sort(entries.begin(), entries.end(),
        [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            return a.score > b.score;
        });

    // Find rank before trimming
    int rank = -1;
    for (int i = 0; i < (int)entries.size(); ++i) {
        if (entries[i].name == name && entries[i].score == score) {
            rank = i + 1;
            break;
        }
    }

    // Keep only the top entries
    if ((int)entries.size() > MAX_LEADERBOARD_ENTRIES)
        entries.resize(MAX_LEADERBOARD_ENTRIES);

    saveLeaderboard(entries);
    return rank;
}

// Display the leaderboard in a formatted box
void displayLeaderboard() {
    auto entries = loadLeaderboard();

    Term::fg(220); Term::bold();
    std::cout << "\n  ╔══════════════════════════════════════════╗\n";
    std::cout << "  ║           🏆 LEADERBOARD 🏆              ║\n";
    std::cout << "  ╠══════╦══════════════════════╦════════════╣\n";
    std::cout << "  ║ RANK ║ PLAYER               ║ SCORE      ║\n";
    std::cout << "  ╠══════╬══════════════════════╬════════════╣\n";
    Term::reset_attr();

    if (entries.empty()) {
        Term::fg(240);
        std::cout << "  ║      No scores yet — be the first!      ║\n";
    } else {
        // Medal colors for top 3 ranks
        int rankColors[] = {220, 248, 130}; // gold, silver, bronze

        for (int i = 0; i < (int)entries.size(); ++i) {
            int col = (i < 3) ? rankColors[i] : 245;
            Term::fg(col);

            // Rank column (6 chars including borders)
            std::string rankStr = "  " + std::to_string(i + 1);
            while ((int)rankStr.size() < 5) rankStr += " ";

            // Name column (padded to 20 chars)
            std::string nameStr = " " + entries[i].name;
            while ((int)nameStr.size() < 21) nameStr += " ";

            // Score column (padded to 10 chars)
            std::string scoreStr = " " + std::to_string(entries[i].score);
            while ((int)scoreStr.size() < 11) scoreStr += " ";

            std::cout << "  ║" << rankStr << " ║" << nameStr << " ║" << scoreStr << " ║\n";
        }
    }

    Term::fg(220); Term::bold();
    std::cout << "  ╚══════╩══════════════════════╩════════════╝\n\n";
    Term::reset_attr();
}

// ═══════════════════════════════════════════════════════════════════
//  SECTION 4: Visual Palette System
//  Depth-aware color palettes for 3D terrain rendering.
//  Colors darken with increasing distance from the player.
// ═══════════════════════════════════════════════════════════════════

struct LanePalette {
    int top_fg, top_bg;      // Top face colors
    int front_fg, front_bg;  // Front face colors
    int shadow_bg;            // Ground shadow color
    char top_ch;              // Char for top face tiles
    char front_ch;            // Char for front face tiles
};

// Build a palette for a lane type at a given depth from the player
LanePalette makePalette(LaneType t, int depthFromPlayer) {
    int d = std::min(depthFromPlayer, BH - 1);
    float fade = 1.0f - (d / (float)BH) * 0.55f; // Fade factor: 1.0 near → ~0.45 far

    // Helper: darken grayscale colors (xterm256 range 232-255)
    auto darken = [&](int base, float f) -> int {
        if (base >= 232 && base <= 255) {
            int v = (int)((base - 232) * f);
            return 232 + std::max(0, std::min(23, v));
        }
        return base;
    };

    LanePalette p;
    switch (t) {
        case SAFE: // Stone/cobblestone — warm gray
            p.top_fg    = darken(253, fade * 0.9f);
            p.top_bg    = darken(240, fade);
            p.front_fg  = darken(245, fade * 0.7f);
            p.front_bg  = darken(236, fade);
            p.shadow_bg = 232;
            p.top_ch    = ' ';
            p.front_ch  = ' ';
            break;
        case GRASS: // Green — depth-aware green shades
            {
                int gtop  = (d < 3) ? 28 : 22;
                int gfrnt = 22;
                p.top_fg    = (d < 3) ? 34 : 28;
                p.top_bg    = gtop;
                p.front_fg  = (d < 3) ? 28 : 22;
                p.front_bg  = gfrnt;
                p.shadow_bg = 232;
                p.top_ch    = ' ';
                p.front_ch  = ' ';
            }
            break;
        case ROAD: // Asphalt dark gray
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
        case WATER: // Blue water — depth-aware blue shades
            {
                int wdepth = (d < 2) ? 27 : (d < 5) ? 20 : 17;
                int wfrnt  = (d < 2) ? 20 : 17;
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
//  SECTION 5: Generic Singly-Linked List
//  Used to store obstacles within each lane.
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
//  SECTION 6: Obstacle
//  Represents a car (on ROAD lanes) or log (on WATER lanes).
//  Moves continuously at its assigned speed each physics tick.
// ═══════════════════════════════════════════════════════════════════

struct Obstacle {
    double x;      // Left edge position (fractional tile)
    int    width;  // Width in tiles
    double speed;  // Pixels/tick; negative = moving left
    int    vtype;  // Visual type: 0=car, 1=log

    // Check if tile position px is under this obstacle
    bool contains(double px) const {
        return px >= x && px < x + (double)width;
    }
    int iLeft()  const { return (int)x; }
    int iRight() const { return (int)(x + width); }
};

// ═══════════════════════════════════════════════════════════════════
//  SECTION 7: Lane
//  A single horizontal strip of terrain with its obstacle list.
//  Lanes form a doubly-linked list (the World).
// ═══════════════════════════════════════════════════════════════════

struct Lane {
    LaneType              type;  // SAFE / GRASS / ROAD / WATER
    SLinkedList<Obstacle> obs;   // Obstacles on this lane
    int                   shade; // Background shade value (unused externally)
    Lane*                 prev = nullptr;
    Lane*                 next = nullptr;

    Lane(LaneType t, int s) : type(t), shade(s) {}
    Lane(const Lane&)            = delete;
    Lane& operator=(const Lane&) = delete;
};

// ═══════════════════════════════════════════════════════════════════
//  SECTION 8: World
//  Doubly-linked list of Lanes, managed by absolute Y coordinate.
//  - head = farthest (highest absY)
//  - tail = nearest/oldest (lowest absY = baseY)
//  As the player moves forward, new lanes are appended at head
//  and old lanes are popped from tail.
// ═══════════════════════════════════════════════════════════════════

struct World {
    Lane* head  = nullptr;
    Lane* tail  = nullptr;
    int   count = 0;
    int   baseY = -1; // Absolute Y of the tail (oldest) lane

    ~World() {
        Lane* cur = head;
        while (cur) { Lane* n = cur->next; delete cur; cur = n; }
    }

    // Absolute Y of the head (newest/farthest) lane
    int headAbsY() const { return (count > 0) ? baseY + (count - 1) : -1; }

    // Push a new lane onto the top (farthest)
    void pushTop(Lane* l) {
        l->next = head; l->prev = nullptr;
        if (head) head->prev = l; else tail = l;
        head = l; ++count;
        if (count == 1) baseY = 0;
    }

    // Remove the oldest lane from the bottom
    void popBottom() {
        if (!tail) return;
        Lane* t = tail;
        tail = t->prev;
        if (tail) tail->next = nullptr; else head = nullptr;
        delete t; --count; ++baseY;
    }

    // O(n) lookup by absolute Y coordinate; walks from nearest end
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
//  SECTION 9: RNG Helpers
// ═══════════════════════════════════════════════════════════════════

static int    rng (int lo, int hi)       { return lo + rand() % (hi - lo + 1); }
static double rngf(double lo, double hi) { return lo + (hi - lo) * (rand() / (double)RAND_MAX); }

// ═══════════════════════════════════════════════════════════════════
//  SECTION 10: Lane Factory
//  Generates lanes with appropriate obstacles based on a repeating
//  16-step pattern index. ROAD lanes get cars; WATER lanes get logs.
// ═══════════════════════════════════════════════════════════════════

static int s_laneGen = 0; // Global lane generation counter

Lane* makeLane(int idx) {
    int rel = ((idx % 16) + 16) % 16; // Normalize to 0-15 range

    // Assign lane type from repeating pattern
    LaneType type;
    int shade;
    if      (rel == 0 || rel == 7 || rel == 15)              { type = SAFE;  shade = 238; }
    else if (rel == 1 || rel == 2 || rel == 8
          || rel == 13 || rel == 14)                         { type = GRASS; shade = 236; }
    else if (rel >= 3 && rel <= 6)                           { type = ROAD;  shade = 234; }
    else                                                     { type = WATER; shade = 233; }

    Lane* l = new Lane(type, shade);

    // Populate ROAD lanes with cars
    if (type == ROAD) {
        double spd    = rngf(0.20, 0.55);
        if (rand() % 2) spd = -spd; // Randomize direction
        int numCars   = rng(2, 4);
        double carW   = rng(4, 6);
        double minGap = 9.0;        // Minimum gap between cars
        double pos    = rngf(0.0, 4.0);
        for (int i = 0; i < numCars; ++i) {
            Obstacle o;
            o.x = pos; o.width = (int)carW; o.speed = spd; o.vtype = 0;
            l->obs.push_back(o);
            pos += carW + minGap + rngf(0, 6);
        }
    }
    // Populate WATER lanes with logs
    else if (type == WATER) {
        double spd    = rngf(0.12, 0.30);
        if (rand() % 2) spd = -spd;
        int numLogs   = rng(3, 5);
        double logW   = rngf(5.0, 9.0);
        double gap    = rngf(4.0, 7.0);
        double pos    = rngf(0.0, 4.0);
        for (int i = 0; i < numLogs; ++i) {
            Obstacle o;
            o.x = pos; o.width = (int)logW; o.speed = spd; o.vtype = 1;
            l->obs.push_back(o);
            pos += logW + gap;
            if (pos > BW) pos -= BW;
        }
    }
    return l;
}

// Build the initial set of lanes around the player's starting position
void buildInitialWorld(World& w) {
    s_laneGen = 0;
    Lane* l   = makeLane(s_laneGen++);
    w.head = w.tail = l;
    w.count = 1;
    w.baseY = 0;
    // Generate AHEAD_BUFFER lanes above the start
    for (int i = 1; i <= AHEAD_BUFFER; ++i) {
        Lane* nl = makeLane(s_laneGen++);
        nl->next = w.head; nl->prev = nullptr;
        if (w.head) w.head->prev = nl;
        w.head = nl;
        ++w.count;
    }
}

// Ensure lanes exist around the player; generate ahead and prune behind
void ensureLanes(World& w, int playerAbsY) {
    int needed_high = playerAbsY + AHEAD_BUFFER;
    int needed_low  = playerAbsY - BELOW_BUFFER;
    // Add new lanes ahead of the player
    while (w.headAbsY() < needed_high) {
        Lane* l = makeLane(s_laneGen++);
        l->next = w.head; l->prev = nullptr;
        if (w.head) w.head->prev = l; else w.tail = l;
        w.head = l; ++w.count;
    }
    // Remove lanes too far behind the player
    while (w.count > 0 && w.baseY < needed_low)
        w.popBottom();
}

// ═══════════════════════════════════════════════════════════════════
//  SECTION 11: Physics
//  Updates obstacle positions each tick and handles player
//  interactions with terrain (log riding, car collision, etc.)
// ═══════════════════════════════════════════════════════════════════

// Advance all obstacles in a lane by one tick, wrapping at board edges
void tickLane(Lane* l) {
    if (!l) return;
    double W = (double)BW;
    auto* cur = l->obs.head;
    while (cur) {
        Obstacle& o = cur->data;
        o.x += o.speed;
        if (o.speed > 0 && o.x >= W)              o.x -= W + o.width;
        if (o.speed < 0 && o.x + o.width <= 0.0)  o.x += W + o.width;
        cur = cur->next;
    }
}

// Find obstacle overlapping tile px in lane l, or nullptr
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
//  SECTION 12: Player
// ═══════════════════════════════════════════════════════════════════

struct Player {
    double px        = BW / 2.0; // Horizontal position (fractional tile)
    int    absY      = 0;        // Absolute Y lane coordinate (increases forward)
    bool   alive     = true;
    int    score     = 0;        // Best absY reached this session
    double logAccum  = 0.0;      // Accumulated log drift (fractional pixel)
    int    jumpFrame = 0;        // Animation: 0=idle, 1-4=jumping
    bool   movDir    = true;     // Last move direction: true=right/forward
};

// Attempt to move the player; applies collision and lane rules
void tryMove(World& w, Player& p, int dx, int dy) {
    double newPx  = p.px + dx;
    int    newAbsY = p.absY + dy;

    // Clamp horizontal movement to board bounds
    if (newPx < 0.0)      newPx = 0.0;
    if (newPx > BW - 1.0) newPx = BW - 1.0;

    if (dx != 0) p.movDir = (dx > 0);

    Lane* tgt = w.laneAt(newAbsY);
    if (!tgt) return; // No lane at destination — don't move

    if (tgt->type == ROAD) {
        Obstacle* car = findObs(tgt, newPx);
        p.px = newPx; p.absY = newAbsY; p.logAccum = 0.0;
        if (car) p.alive = false; // Stepped into a car
    } else if (tgt->type == WATER) {
        Obstacle* log = findObs(tgt, newPx);
        p.px = newPx; p.absY = newAbsY; p.logAccum = 0.0;
        if (!log) p.alive = false; // Stepped into water without a log
    } else {
        // SAFE or GRASS — always walkable
        p.px = newPx; p.absY = newAbsY; p.logAccum = 0.0;
    }

    if (p.alive) p.jumpFrame = 1; // Trigger jump animation
    if (p.absY > p.score) p.score = p.absY; // Update best score
}

// Run one physics tick: move obstacles, apply log drift, detect death
void physTick(World& w, Player& p) {
    if (!p.alive) return;

    // Tick obstacles in lanes near the player
    int hi = p.absY + PLAYER_LANE + 2;
    int lo = p.absY - (BH - PLAYER_LANE) - 2;
    for (int ay = lo; ay <= hi; ++ay) tickLane(w.laneAt(ay));

    Lane* cur = w.laneAt(p.absY);

    // Handle water: drift with log or drown
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

    // Check road collision after obstacles move
    if (cur && cur->type == ROAD) {
        if (findObs(cur, p.px)) p.alive = false;
    }

    // Advance jump animation frame
    if (p.jumpFrame > 0) p.jumpFrame = (p.jumpFrame + 1) % 5;
}

// ═══════════════════════════════════════════════════════════════════
//  SECTION 13: 3D Rendering Engine
//
//  Layout per lane (3 terminal rows):
//    Row 0 (top face):   ▄▄▄▄ — uses lower half-block ▄ to simulate top
//    Row 1 (front face): ████ — full blocks for the front wall
//    Row 2 (shadow):     ░░░░ — subtle shadow/ground strip
//
//  Isometric perspective: lane at screen depth d gets x-offset = d * ISO_SKEW
//  (nearest lane = 0 offset, farthest = BH * ISO_SKEW columns right)
// ═══════════════════════════════════════════════════════════════════

// A single character cell in the virtual screen buffer
struct ScreenCell {
    std::string seq;
    bool        filled = false;
};

// Full double-buffered screen; flushed atomically to minimize flicker
struct ScreenBuf {
    int rows, cols;
    std::vector<std::vector<ScreenCell>> cells;

    ScreenBuf(int r, int c) : rows(r), cols(c),
        cells(r, std::vector<ScreenCell>(c)) {}

    // Write a single ANSI cell; silently clip out-of-bounds writes
    void set(int r, int c, const std::string& seq, bool fill = true) {
        if (r < 0 || r >= rows || c < 0 || c >= cols) return;
        cells[r][c].seq    = seq;
        cells[r][c].filled = fill;
    }

    // Emit the entire buffer to stdout in one write
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

// Per-tile rendering info (populated before drawing each lane row)
struct TileInfo {
    bool hasObs    = false;
    int  obsVtype  = -1;   // 0=car, 1=log
    bool obsEdgeL  = false;
    bool obsEdgeR  = false;
    bool isPlayer  = false;
    bool playerDead = false;
};

// Draw a single lane's 3 rows into the screen buffer
void drawLane3D(ScreenBuf& buf,
                Lane* lane, LaneType type,
                int screenRow, int xOffset, int depth,
                const Player& p, int /*laneAbsY*/,
                bool playerOnThisLane)
{
    LanePalette pal = makePalette(type, depth);
    std::vector<TileInfo> tiles(BW);

    // ── Fill tile info from obstacle list ──────────────────────────
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

    // ── Mark player tile ───────────────────────────────────────────
    if (playerOnThisLane) {
        int pc = (int)p.px;
        if (pc >= 0 && pc < BW) {
            tiles[pc].isPlayer    = true;
            tiles[pc].playerDead  = !p.alive;
        }
    }

    // ── Unicode half-block character constants (UTF-8 encoded) ─────
    // ▄ = \xe2\x96\x84  lower half block
    // █ = \xe2\x96\x88  full block
    // ▌ = \xe2\x96\x8c  left half block
    // ░ = \xe2\x96\x91  light shade
    // ▒ = \xe2\x96\x92  medium shade
    // ▓ = \xe2\x96\x93  dark shade

    // ── Row 0: Top face (▄ with terrain-appropriate coloring) ───────
    for (int c = 0; c < BW; ++c) {
        int sc = c + xOffset;
        const TileInfo& ti = tiles[c];
        std::string cell;

        if (ti.isPlayer) {
            // Yellow/orange crown; red when dead
            if (ti.playerDead) {
                cell = Term::sbg(52) + Term::sfg(196) + Term::sbold() + "\xe2\x96\x84";
            } else {
                int pColor = (p.jumpFrame > 0) ? 226 : 220;
                cell = Term::sbg(pColor) + Term::sfg(214) + "\xe2\x96\x84";
            }
        } else if (ti.hasObs) {
            if (ti.obsVtype == 0) { // Car top: metallic
                int carTopBg = (depth < 3) ? 250 : 244;
                int carTopFg = (depth < 3) ? 255 : 248;
                if (ti.obsEdgeL || ti.obsEdgeR)
                    cell = Term::sbg(carTopBg) + Term::sfg(carTopFg) + "\xe2\x96\x84";
                else
                    cell = Term::sbg(carTopBg - 2) + Term::sfg(carTopFg) + "\xe2\x96\x84";
            } else { // Log top: brown wood
                int logBg = (depth < 3) ? 94 : 58;
                int logFg = (depth < 3) ? 136 : 100;
                if (ti.obsEdgeL || ti.obsEdgeR)
                    cell = Term::sbg(logBg) + Term::sfg(logFg + 6) + "\xe2\x96\x84";
                else
                    cell = Term::sbg((c % 2 == 0) ? logBg : logBg - 1) + Term::sfg(logFg) + "\xe2\x96\x84";
            }
        } else {
            // Terrain texture with type-specific detail
            switch (type) {
                case WATER: {
                    int wc  = (depth < 3) ? 51 : (depth < 6) ? 39 : 27;
                    int wbg = pal.top_bg;
                    if ((c + (int)(lane ? (lane->obs.head ? lane->obs.head->data.x * 0.3 : 0) : 0)) % 4 == 0)
                        cell = Term::sbg(wbg + 1) + Term::sfg(wc) + "\xe2\x96\x84";
                    else
                        cell = Term::sbg(wbg) + Term::sfg(wbg + 3) + "\xe2\x96\x84";
                    break;
                }
                case GRASS: {
                    int gbg = pal.top_bg, gfg = pal.top_fg;
                    if (c % 5 == 0)
                        cell = Term::sbg(gbg) + Term::sfg(gfg + 2) + "\xe2\x96\x84";
                    else
                        cell = Term::sbg(gbg) + Term::sfg(gfg) + "\xe2\x96\x84";
                    break;
                }
                case ROAD: {
                    int rbg = pal.top_bg, rfg = pal.top_fg;
                    if (c % 6 == 0)
                        cell = Term::sbg(rbg + 1) + Term::sfg(rfg + 2) + "\xe2\x96\x84";
                    else
                        cell = Term::sbg(rbg) + Term::sfg(rfg) + "\xe2\x96\x84";
                    break;
                }
                case SAFE: {
                    int sbg2 = pal.top_bg, sfg2 = pal.top_fg;
                    if ((c + depth) % 3 == 0)
                        cell = Term::sbg(sbg2 + 1) + Term::sfg(sfg2 + 2) + "\xe2\x96\x84";
                    else
                        cell = Term::sbg(sbg2) + Term::sfg(sfg2) + "\xe2\x96\x84";
                    break;
                }
            }
        }
        buf.set(screenRow, sc, cell);
    }

    // ── Row 1: Front face (solid color blocks) ─────────────────────
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
            if (ti.obsVtype == 0) { // Car front face with headlight/taillight
                int carFg, carBg;
                if (ti.obsEdgeL) {
                    carBg = (depth < 3) ? 226 : 178; // Headlight yellow
                    carFg = 255;
                    cell = Term::sbg(carBg) + Term::sfg(carFg) + "\xe2\x96\x93";
                } else if (ti.obsEdgeR) {
                    carBg = (depth < 3) ? 124 : 88;  // Taillight red
                    carFg = 196;
                    cell = Term::sbg(carBg) + Term::sfg(carFg) + "\xe2\x96\x93";
                } else {
                    carBg = (depth < 3) ? 240 : 236;
                    carFg = (depth < 3) ? 250 : 244;
                    cell = Term::sbg(carBg) + Term::sfg(carFg) + "\xe2\x96\x88";
                }
            } else { // Log front: bark texture
                int logFrontBg = (depth < 3) ? 130 : 94;
                int logFrontFg = (depth < 3) ? 172 : 130;
                if (ti.obsEdgeL || ti.obsEdgeR)
                    cell = Term::sbg(logFrontBg) + Term::sfg(logFrontFg + 6) + "\xe2\x96\x93";
                else
                    cell = Term::sbg(logFrontBg - (c % 2)) + Term::sfg(logFrontFg) + "\xe2\x96\x92";
            }
        } else {
            // Terrain front face with type-specific texture
            switch (type) {
                case WATER: {
                    int wfbg = pal.front_bg, wffg = pal.front_fg;
                    cell = (c % 3 == 1)
                        ? Term::sbg(wfbg) + Term::sfg(wffg + 3) + "\xe2\x96\x92"
                        : Term::sbg(wfbg) + Term::sfg(wffg) + "\xe2\x96\x88";
                    break;
                }
                case GRASS: {
                    int gfbg = pal.front_bg, gffg = pal.front_fg;
                    cell = (c % 4 == 0)
                        ? Term::sbg(gfbg - 1) + Term::sfg(gffg - 1) + "\xe2\x96\x92"
                        : Term::sbg(gfbg) + Term::sfg(gffg) + "\xe2\x96\x88";
                    break;
                }
                case ROAD: {
                    cell = Term::sbg(pal.front_bg) + Term::sfg(pal.front_fg) + "\xe2\x96\x88";
                    break;
                }
                case SAFE: {
                    int sfbg = pal.front_bg, sffg = pal.front_fg;
                    cell = (c % 3 == 0)
                        ? Term::sbg(sfbg + 1) + Term::sfg(sffg + 2) + "\xe2\x96\x91"
                        : Term::sbg(sfbg) + Term::sfg(sffg) + "\xe2\x96\x88";
                    break;
                }
            }
        }
        buf.set(fr, sc, cell);
    }

    // ── Row 2: Shadow / ground strip ──────────────────────────────
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
//  SECTION 14: HUD and Footer Rendering
// ═══════════════════════════════════════════════════════════════════

static const int HUD_ROWS = 3;

// Draw score / title bar at top of buffer
void drawHUD3D(ScreenBuf& buf, const Player& p, int hi, const std::string& playerName) {
    // Top border
    { int col = 0;
      buf.set(0, col++, Term::sbg(234) + Term::sfg(240) + "\xe2\x94\x8c");
      for (int i = 0; i < SCR_W - 2; ++i)
          buf.set(0, col++, Term::sbg(234) + Term::sfg(238) + "\xe2\x94\x80");
      buf.set(0, col, Term::sbg(234) + Term::sfg(240) + "\xe2\x94\x90");
    }

    // Title + player name + scores
    { int col = 0;
      buf.set(1, col++, Term::sbg(234) + Term::sfg(240) + "\xe2\x94\x82");
      std::string title = " \xF0\x9F\x90\x94 CROSSY ROAD 3D  C++  [" + playerName + "]";
      for (char c : title)
          buf.set(1, col++, Term::sbg(234) + Term::sfg(255) + Term::sbold() + std::string(1, c));
      std::string scoreStr = "  SCORE:" + std::to_string(p.score)
                           + "  BEST:" + std::to_string(hi) + "  ";
      int used = 1 + (int)title.size() + (int)scoreStr.size() + 1;
      int pad  = SCR_W - used;
      for (int i = 0; i < pad && col < SCR_W - 1; ++i)
          buf.set(1, col++, Term::sbg(234) + " ");
      for (char c : scoreStr)
          buf.set(1, col++, Term::sbg(234) + Term::sfg(248) + std::string(1, c));
      buf.set(1, SCR_W - 1, Term::sbg(234) + Term::sfg(240) + "\xe2\x94\x82");
    }

    // Separator
    { int col = 0;
      buf.set(2, col++, Term::sbg(234) + Term::sfg(240) + "\xe2\x94\x9c");
      for (int i = 0; i < SCR_W - 2; ++i)
          buf.set(2, col++, Term::sbg(234) + Term::sfg(237) + "\xe2\x94\x80");
      buf.set(2, col, Term::sbg(234) + Term::sfg(240) + "\xe2\x94\xa4");
    }
}

// Draw controls / game-over line at bottom of buffer
void drawFooter3D(ScreenBuf& buf, int baseRow, bool dead) {
    // Bottom border
    { int col = 0;
      buf.set(baseRow, col++, Term::sbg(234) + Term::sfg(240) + "\xe2\x94\x94");
      for (int i = 0; i < SCR_W - 2; ++i)
          buf.set(baseRow, col++, Term::sbg(234) + Term::sfg(237) + "\xe2\x94\x80");
      buf.set(baseRow, col, Term::sbg(234) + Term::sfg(240) + "\xe2\x94\x98");
    }

    // Controls or game-over message
    { int col = 0;
      buf.set(baseRow + 1, col++, Term::sbg(232) + " ");
      std::string line;
      if (dead) {
          line = Term::sbg(88) + Term::sfg(255) + Term::sbold()
               + "  \xe2\x95\x94\xe2\x95\x90\xe2\x95\x90 GAME OVER \xe2\x95\x90\xe2\x95\x90\xe2\x95\x97  "
               + Term::sreset() + Term::sfg(242)
               + "  R restart    L leaderboard    Q quit";
      } else {
          line = Term::sbg(232) + Term::sfg(240)
               + "  W/\xe2\x86\x91 fwd  S/\xe2\x86\x93 back  "
               + "A/\xe2\x86\x90 left  D/\xe2\x86\x92 right  "
               + "L leaderboard  Q quit";
      }
      buf.set(baseRow + 1, 1, line);
      for (int c = 2; c < SCR_W; ++c)
          if (!buf.cells[baseRow + 1][c].filled)
              buf.set(baseRow + 1, c, Term::sbg(232) + " ");
    }
}

// Draw the isometric left-edge depth rail
void drawDepthRail(ScreenBuf& buf, int startRow, int numLanes) {
    for (int lane = 0; lane < numLanes; ++lane) {
        int xOff = (numLanes - 1 - lane) * ISO_SKEW;
        int r    = startRow + lane * LANE_H;
        for (int row = 0; row < LANE_H - 1; ++row)
            buf.set(r + row, xOff, Term::sbg(232) + Term::sfg(236) + "\xe2\x96\x8c");
    }
}

// ═══════════════════════════════════════════════════════════════════
//  SECTION 15: Full Frame Render
// ═══════════════════════════════════════════════════════════════════

void render3D(const World& w, const Player& p, int hi, const std::string& playerName) {
    int totalRows = HUD_ROWS + PLAY_ROWS + 2;
    int totalCols = SCR_W + 2;
    ScreenBuf buf(totalRows, totalCols);

    // Fill background to dark gray
    for (int r = 0; r < totalRows; ++r)
        for (int c = 0; c < totalCols; ++c)
            buf.set(r, c, Term::sbg(232) + " ");

    drawHUD3D(buf, p, hi, playerName);

    // Render each visible lane
    for (int sl = 0; sl < BH; ++sl) {
        int absY    = p.absY + (PLAYER_LANE - sl); // Absolute Y of this screen slot
        int xOffset = (BH - 1 - sl) * ISO_SKEW;   // Isometric horizontal shift
        int depth   = std::abs(sl - PLAYER_LANE);  // Distance from player's lane
        Lane*    lane    = w.laneAt(absY);
        LaneType type    = lane ? lane->type : SAFE;
        int      screenRow = HUD_ROWS + sl * LANE_H;
        bool     playerOnLane = (absY == p.absY);
        drawLane3D(buf, lane, type, screenRow, xOffset, depth, p, absY, playerOnLane);
    }

    drawDepthRail(buf, HUD_ROWS, BH);
    drawFooter3D(buf, HUD_ROWS + PLAY_ROWS, !p.alive);
    buf.flush();
}

// ═══════════════════════════════════════════════════════════════════
//  SECTION 16: Input Handling
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
//  SECTION 17: Splash Screen
// ═══════════════════════════════════════════════════════════════════

// Returns the player name entered at the prompt
std::string splash() {
    Term::clear();

    // ASCII art title
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

    // Subtitle
    Term::fg(245);
    std::cout << "  ┌──────────────────────────────────────────────────────┐\n";
    std::cout << "  │  C++ Terminal Edition v1.5  │  3D Isometric ASCII    │\n";
    std::cout << "  │  Linked-List Architecture   │  256-Color Renderer    │\n";
    std::cout << "  │  Persistent Leaderboard     │  Player Name Support   │\n";
    std::cout << "  └──────────────────────────────────────────────────────┘\n\n";

    // Controls
    Term::fg(240);
    std::cout << "  ┌─ CONTROLS ──────────────────────────────────────────┐\n";Term::fg(252);
    std::cout << "  │  W / ↑   Move forward (into the 3D depth)           │\n";
    std::cout << "  │  S / ↓   Move back                                  │\n";
    std::cout << "  │  A / ←   Move left                                  │\n";
    std::cout << "  │  D / →   Move right                                 │\n";
    std::cout << "  │  L       View leaderboard (in-game or after death)  │\n";
    std::cout << "  │  R       Restart after death                        │\n";
    std::cout << "  │  Q       Quit                                       │\n";Term::fg(240);
    std::cout << "  └─────────────────────────────────────────────────────┘\n\n";

    // Show existing leaderboard on splash
    displayLeaderboard();

    // Player name prompt
    Term::fg(220); Term::bold();
    std::cout << "  Enter your name: ";
    Term::reset_attr();
    std::string name;
    std::getline(std::cin, name);

    // Default name if empty
    if (name.empty()) name = "Anonymous";
    // Trim to 20 chars to fit leaderboard column
    if ((int)name.size() > 20) name = name.substr(0, 20);

    std::cout << "\n";
    Term::fg(244);
    std::cout << "  Welcome, " << name << "! Press ENTER to start ...\n\n";
    Term::reset_attr();
    std::cin.ignore(10000, '\n');

    return name;
}

// ═══════════════════════════════════════════════════════════════════
//  SECTION 18: Game Loop
// ═══════════════════════════════════════════════════════════════════

void runGame(const std::string& playerName) {
    srand((unsigned)time(nullptr));

    World  world;
    Player player;
    int    hiScore = 0;
    bool   quit    = false;

    buildInitialWorld(world);
    ensureLanes(world, player.absY);

    using Clock = std::chrono::steady_clock;
    using MS    = std::chrono::milliseconds;

    const long TICK_MS  = 90;  // Physics update interval
    const long FRAME_MS = 50;  // Render interval (~20 fps)

    auto lastTick  = Clock::now();
    auto lastFrame = Clock::now();

    Term::raw();
    Term::hide_cursor();
    Term::clear();
    render3D(world, player, hiScore, playerName);

    while (!quit) {
        auto now = Clock::now();

        // ── Input processing ──────────────────────────────────────
        Key key = pollKey();
        int dx = 0, dy = 0;
        bool moved = false;

        switch (key) {
            case K_QUIT:
                quit = true;
                continue;

            case K_RESTART:
                if (!player.alive) {
                    // Reset world and player state
                    { World tmp; std::swap(world, tmp); }
                    s_laneGen = 0;
                    player    = Player();
                    buildInitialWorld(world);
                    ensureLanes(world, player.absY);
                    Term::clear();
                }
                continue;

            case K_LEADERBOARD:
                // Pause and show leaderboard overlay
                Term::restore();
                Term::show_cursor();
                Term::clear();
                displayLeaderboard();
                Term::fg(244);
                std::cout << "  Press ENTER to return to game...\n";
                Term::reset_attr();
                {
                    // Temporarily restore canonical input to read Enter
                    struct termios t;
                    tcgetattr(STDIN_FILENO, &t);
                    t.c_lflag |= ICANON | ECHO;
                    tcsetattr(STDIN_FILENO, TCSANOW, &t);
                    std::string dummy;
                    std::getline(std::cin, dummy);
                }
                Term::raw();
                Term::hide_cursor();
                Term::clear();
                render3D(world, player, hiScore, playerName);
                continue;

            case K_UP:    dy =  1; moved = true; break;
            case K_DOWN:  dy = -1; moved = true; break;
            case K_LEFT:  dx = -1; moved = true; break;
            case K_RIGHT: dx =  1; moved = true; break;
            default: break;
        }

        // Move player if alive and a direction key was pressed
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

        // ── Render frame ──────────────────────────────────────────
        long frameMs = std::chrono::duration_cast<MS>(now - lastFrame).count();
        if (frameMs >= FRAME_MS) {
            lastFrame = now;
            render3D(world, player, hiScore, playerName);
        }

        std::this_thread::sleep_for(MS(8)); // Yield CPU between frames
    }

    // ── Session end: submit score and show summary ─────────────────
    Term::show_cursor();
    Term::restore();
    Term::clear();

    // Submit final score to leaderboard
    int rank = submitScore(playerName, player.score);

    std::cout << "\n";
    Term::fg(220); Term::bold();
    std::cout << "  ╔════════════════════════════════╗\n";
    std::cout << "  ║           Your Record          ║\n";
    std::cout << "  ╠════════════════════════════════╣\n";
    Term::reset_attr(); Term::fg(252);
    std::cout << "  ║   Player: " << playerName;
    { int pad = 21 - (int)playerName.size(); for (int i=0;i<pad;i++) std::cout<<' '; }
    std::cout << "║\n";
    std::cout << "  ║   Score : " << player.score;
    { int pad = 21 - (int)std::to_string(player.score).size(); for (int i=0;i<pad;i++) std::cout<<' '; }
    std::cout << "║\n";
    std::cout << "  ║   Best  : " << hiScore;
    { int pad = 21 - (int)std::to_string(hiScore).size(); for (int i=0;i<pad;i++) std::cout<<' '; }
    std::cout << "║\n";

    // Show rank if it made the leaderboard
    if (rank >= 1 && rank <= MAX_LEADERBOARD_ENTRIES) {
        std::string rankLine = "  ║   Rank  : #" + std::to_string(rank) + " on leaderboard";
        std::cout << rankLine;
        int pad = 35 - (int)rankLine.size() + 2;
        for (int i=0;i<pad;i++) std::cout<<' ';
        std::cout << "║\n";
    }

    Term::fg(220);
    std::cout << "  ╚════════════════════════════════╝\n\n";
    Term::reset_attr();

    // Show updated leaderboard after session
    displayLeaderboard();
}

// ═══════════════════════════════════════════════════════════════════
//  SECTION 19: Entry Point
// ═══════════════════════════════════════════════════════════════════

// Signal handler: cleanly restore terminal on Ctrl+C
static void cleanup(int) {
    Term::show_cursor();
    Term::restore();
    exit(0);
}

int main() {
    signal(SIGINT,  cleanup);
    signal(SIGTERM, cleanup);

    // Show splash, get player name, then run the game
    std::string name = splash();
    runGame(name);

    return 0;
}
