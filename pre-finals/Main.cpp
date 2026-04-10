/*
 ╔══════════════════════════════════════════════════════════════════════════╗
 ║          CROSSY ROAD — C++ Terminal Edition  v3.0  "3D Isometric"       ║
 ║   Linked-List Architecture | Isometric ASCII Renderer | ANSI 256-color  ║
 ╚══════════════════════════════════════════════════════════════════════════╝

  3D isometric projection via ASCII art:
    Each world tile is drawn as a 3D block with top/left/right faces.
    The camera sits at a fixed isometric angle.

  Isometric mapping (world → screen):
    sx = (x - z) * TILE_W/2  +  ORIGIN_X
    sy = (x + z) * TILE_H/2  +  cam_offset_y

  Each tile occupies:
    TILE_W = 6  columns  (width of top face projected)
    TILE_H = 3  rows     (height of top face projected)
    BLOCK_H= 2  rows     (visible side height)

  World coords:
    x  = horizontal lane column  (0 .. COLS-1)
    z  = lane depth (absZ)       (0 = spawn, higher = forward)
    y  = block height            (not rendered independently; encoded as side depth)

  Data structures:
    SLinkedList<Obstacle>  — per-lane obstacle list (singly-linked)
    World (DLL of Lane)    — doubly-linked lane list, pushTop/popBottom
*/

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <chrono>
#include <thread>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <csignal>
#include <algorithm>

// ═══════════════════════════════════════════════════════════════════
//  Terminal helpers
// ═══════════════════════════════════════════════════════════════════
namespace Term {
    struct termios orig;
    bool raw_on = false;

    void raw() {
        tcgetattr(STDIN_FILENO, &orig);
        struct termios t = orig;
        t.c_lflag &= ~(ICANON|ECHO);
        t.c_cc[VMIN] = 0; t.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        raw_on = true;
    }
    void restore() {
        if (raw_on) { tcsetattr(STDIN_FILENO, TCSANOW, &orig); raw_on=false; }
    }
    void clear()       { std::cout << "\033[2J\033[H"; }
    void home()        { std::cout << "\033[H"; }
    void hide_cursor() { std::cout << "\033[?25l"; }
    void show_cursor() { std::cout << "\033[?25h"; }
    void reset()       { std::cout << "\033[0m"; }
    void fg256(int n)  { std::cout << "\033[38;5;" << n << "m"; }
    void bg256(int n)  { std::cout << "\033[48;5;" << n << "m"; }
    void bold()        { std::cout << "\033[1m"; }

    int term_cols() {
        struct winsize w; ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        return w.ws_col > 0 ? w.ws_col : 120;
    }
    int term_rows() {
        struct winsize w; ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        return w.ws_row > 0 ? w.ws_row : 40;
    }

    int read_key() {
        unsigned char buf[8] = {};
        int n = (int)read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) return 0;
        if (n >= 3 && buf[0]==27 && buf[1]=='[') {
            if (buf[2]=='A') return 1000;
            if (buf[2]=='B') return 1001;
            if (buf[2]=='C') return 1002;
            if (buf[2]=='D') return 1003;
        }
        return (int)buf[0];
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Screen framebuffer
// ═══════════════════════════════════════════════════════════════════
struct Cell {
    char    ch  = ' ';
    int     fg  = -1;   // -1 = default
    int     bg  = -1;
    bool    bold= false;
    int     priority = 0; // higher wins when painting
};

struct Screen {
    int W, H;
    std::vector<Cell> buf;

    Screen(int w, int h) : W(w), H(h), buf(w*h) {}

    void clear() {
        for (auto& c : buf) { c = Cell(); }
    }

    Cell& at(int x, int y) {
        static Cell dummy;
        if (x<0||x>=W||y<0||y>=H) return dummy;
        return buf[y*W+x];
    }

    void put(int x, int y, char ch, int fg, int bg, bool bld=false, int pri=0) {
        if (x<0||x>=W||y<0||y>=H) return;
        Cell& c = buf[y*W+x];
        if (pri < c.priority) return;
        c.ch=ch; c.fg=fg; c.bg=bg; c.bold=bld; c.priority=pri;
    }

    void putStr(int x, int y, const std::string& s, int fg, int bg, bool bld=false, int pri=0) {
        for (int i=0; i<(int)s.size(); ++i) put(x+i,y,s[i],fg,bg,bld,pri);
    }

    void flush() {
        Term::home();
        int lastFg=-2, lastBg=-2; bool lastBold=false;
        std::string out;
        out.reserve(W*H*8);

        for (int y=0; y<H; ++y) {
            for (int x=0; x<W; ++x) {
                const Cell& c = buf[y*W+x];

                if (c.bold != lastBold) {
                    if (c.bold) out += "\033[1m";
                    else        out += "\033[22m";
                    lastBold = c.bold; lastFg=-2; lastBg=-2;
                }
                if (c.fg != lastFg) {
                    if (c.fg<0) out += "\033[39m";
                    else { out += "\033[38;5;"; out += std::to_string(c.fg); out += "m"; }
                    lastFg=c.fg;
                }
                if (c.bg != lastBg) {
                    if (c.bg<0) out += "\033[49m";
                    else { out += "\033[48;5;"; out += std::to_string(c.bg); out += "m"; }
                    lastBg=c.bg;
                }
                out += c.ch;
            }
            if (y < H-1) out += '\n';
        }
        out += "\033[0m";
        std::cout << out;
        std::cout.flush();
    }
};

// ═══════════════════════════════════════════════════════════════════
//  Isometric constants
// ═══════════════════════════════════════════════════════════════════
static const int COLS      = 11;    // world columns (x: 0..COLS-1)
static const int TILE_W    = 6;     // screen cols per tile (top face)
static const int TILE_H    = 3;     // screen rows per tile (top face)
static const int SIDE_H    = 2;     // block side height (rows)
static const int VIS_LANES = 14;    // visible lanes (z depth)

// Convert world (col, lane_rel) to screen top-left of tile TOP FACE
// lane_rel: 0=closest lane (bottom of screen), increases going away
// col: 0=left, COLS-1=right
static int isoX(int col, int lane_rel, int originX) {
    return originX + (col - lane_rel) * (TILE_W / 2);
}
static int isoY(int col, int lane_rel, int originY) {
    return originY - (col + lane_rel) * (TILE_H / 2);
}

// ═══════════════════════════════════════════════════════════════════
//  Generic singly-linked list
// ═══════════════════════════════════════════════════════════════════
template<typename T>
struct SLNode { T data; SLNode* next=nullptr; explicit SLNode(const T& d):data(d){} };

template<typename T>
struct SLinkedList {
    SLNode<T>* head=nullptr;
    SLNode<T>* tail=nullptr;
    int size=0;
    SLinkedList()=default;
    SLinkedList(const SLinkedList&)=delete;
    SLinkedList& operator=(const SLinkedList&)=delete;
    ~SLinkedList(){ clear(); }
    void push_back(const T& d){
        auto* n=new SLNode<T>(d);
        if(!tail){head=tail=n;}else{tail->next=n;tail=n;}
        ++size;
    }
    void clear(){
        while(head){auto*t=head;head=head->next;delete t;}
        tail=nullptr;size=0;
    }
};

// ═══════════════════════════════════════════════════════════════════
//  Lane types & obstacle
// ═══════════════════════════════════════════════════════════════════
enum LaneType { SAFE, GRASS, ROAD, WATER };

struct Obstacle {
    double x;      // left edge (col units, fractional)
    int    width;  // cols wide
    double speed;  // cols/tick, signed
    char   glyph;

    bool contains(double px) const { return px>=x && px<x+width; }
};

// ═══════════════════════════════════════════════════════════════════
//  Lane (node in world DLL)
// ═══════════════════════════════════════════════════════════════════
struct Lane {
    LaneType              type;
    SLinkedList<Obstacle> obs;
    Lane* prev=nullptr;
    Lane* next=nullptr;
    Lane(LaneType t): type(t) {}
    Lane(const Lane&)=delete;
    Lane& operator=(const Lane&)=delete;
};

// ═══════════════════════════════════════════════════════════════════
//  World — doubly-linked lane list
//   head = highest absZ (farthest from spawn / top of view)
//   tail = lowest  absZ (spawn side / bottom of view)
//   baseZ = absZ of tail
// ═══════════════════════════════════════════════════════════════════
struct World {
    Lane* head=nullptr; Lane* tail=nullptr;
    int count=0; int baseZ=-1;

    ~World(){ Lane* c=head; while(c){Lane*n=c->next;delete c;c=n;} }

    int headAbsZ() const { return count>0 ? baseZ+(count-1) : -1; }

    void pushTop(Lane* l){
        l->next=head; l->prev=nullptr;
        if(head) head->prev=l; else tail=l;
        head=l; ++count;
        if(count==1) baseZ=0;
    }
    void popBottom(){
        if(!tail) return;
        Lane* t=tail; tail=t->prev;
        if(tail) tail->next=nullptr; else head=nullptr;
        delete t; --count; ++baseZ;
    }

    Lane* laneAt(int absZ) const {
        if(count==0) return nullptr;
        int tailZ=baseZ, headZ=tailZ+(count-1);
        if(absZ<tailZ||absZ>headZ) return nullptr;
        int ft=absZ-tailZ, fh=headZ-absZ;
        if(ft<=fh){ Lane* c=tail; for(int i=0;i<ft;++i) c=c->prev; return c; }
        else      { Lane* c=head; for(int i=0;i<fh;++i) c=c->next; return c; }
    }
};

// ═══════════════════════════════════════════════════════════════════
//  RNG helpers
// ═══════════════════════════════════════════════════════════════════
static int    rng (int lo,int hi){ return lo+rand()%(hi-lo+1); }
static double rngf(double lo,double hi){ return lo+(hi-lo)*(rand()/(double)RAND_MAX); }

// ═══════════════════════════════════════════════════════════════════
//  Lane factory
// ═══════════════════════════════════════════════════════════════════
static int s_gen=0;

LaneType laneTypeOf(int idx){
    int r=((idx%16)+16)%16;
    if(r==0||r==7||r==15) return SAFE;
    if(r==1||r==2||r==8||r==13||r==14) return GRASS;
    if(r>=3&&r<=6) return ROAD;
    return WATER;
}

Lane* makeLane(int idx){
    LaneType t=laneTypeOf(idx);
    Lane* l=new Lane(t);
    if(t==ROAD){
        double spd=rngf(0.18,0.50); if(rand()%2) spd=-spd;
        int num=rng(2,4); double pos=rngf(0,2);
        for(int i=0;i<num;++i){
            Obstacle o; o.x=pos; o.width=rng(2,3); o.speed=spd; o.glyph='=';
            l->obs.push_back(o);
            pos += o.width + rngf(3,6);
        }
    } else if(t==WATER){
        double spd=rngf(0.10,0.28); if(rand()%2) spd=-spd;
        int num=rng(3,5); double pos=rngf(0,2);
        for(int i=0;i<num;++i){
            Obstacle o; o.x=pos; o.width=rng(3,5); o.speed=spd; o.glyph='#';
            l->obs.push_back(o);
            pos += o.width + rngf(2,5);
            if(pos>COLS) pos-=COLS;
        }
    }
    return l;
}

void buildWorld(World& w){
    s_gen=0;
    Lane* l=makeLane(s_gen++); w.head=w.tail=l; w.count=1; w.baseZ=0;
    for(int i=1;i<=VIS_LANES+4;++i){
        Lane* nl=makeLane(s_gen++);
        nl->next=w.head; nl->prev=nullptr;
        if(w.head) w.head->prev=nl; w.head=nl; ++w.count;
    }
}

void ensureLanes(World& w, int pz){
    while(w.headAbsZ() < pz+VIS_LANES+4){
        Lane* nl=makeLane(s_gen++);
        nl->next=w.head; nl->prev=nullptr;
        if(w.head) w.head->prev=nl; else w.tail=nl;
        w.head=nl; ++w.count;
    }
    while(w.count>0 && w.baseZ < pz-4) w.popBottom();
}

// ═══════════════════════════════════════════════════════════════════
//  Physics
// ═══════════════════════════════════════════════════════════════════
void tickLane(Lane* l){
    if(!l) return;
    double W=(double)COLS;
    for(auto* n=l->obs.head;n;n=n->next){
        Obstacle& o=n->data;
        o.x+=o.speed;
        if(o.speed>0 && o.x>=W)              o.x-=W+o.width;
        if(o.speed<0 && o.x+o.width<=0.0)    o.x+=W+o.width;
    }
}

Obstacle* findObs(Lane* l, double px){
    if(!l) return nullptr;
    for(auto* n=l->obs.head;n;n=n->next)
        if(n->data.contains(px)) return &n->data;
    return nullptr;
}

// ═══════════════════════════════════════════════════════════════════
//  Player
// ═══════════════════════════════════════════════════════════════════
struct Player {
    double px   = COLS/2.0;
    int    absZ = 0;
    bool   alive= true;
    int    score= 0;
    double logAcc=0.0;
    // hop animation
    float  hopT  = 1.0f;  // 0..1, 1=grounded
    int    hopDX = 0;
    int    hopDZ = 0;
};

// ═══════════════════════════════════════════════════════════════════
//  Movement
// ═══════════════════════════════════════════════════════════════════
void tryMove(World& w, Player& p, int dx, int dz){
    double nx=p.px+dx;
    int    nz=p.absZ+dz;
    if(nx<0.0) nx=0.0;
    if(nx>COLS-1.0) nx=COLS-1.0;
    Lane* tgt=w.laneAt(nz);
    if(!tgt) return;

    if(tgt->type==ROAD){
        Obstacle* car=findObs(tgt,nx);
        p.px=nx; p.absZ=nz; p.logAcc=0.0;
        if(car) p.alive=false;
    } else if(tgt->type==WATER){
        Obstacle* log=findObs(tgt,nx);
        p.px=nx; p.absZ=nz; p.logAcc=0.0;
        if(!log) p.alive=false;
    } else {
        p.px=nx; p.absZ=nz; p.logAcc=0.0;
    }
    if(p.absZ>p.score) p.score=p.absZ;
    // start hop animation
    p.hopT=0.0f; p.hopDX=dx; p.hopDZ=dz;
}

void physTick(World& w, Player& p){
    if(!p.alive) return;
    int hi=p.absZ+VIS_LANES+2, lo=p.absZ-3;
    for(int z=lo;z<=hi;++z) tickLane(w.laneAt(z));

    Lane* cur=w.laneAt(p.absZ);
    if(cur && cur->type==WATER){
        Obstacle* log=findObs(cur,p.px);
        if(!log){ p.alive=false; return; }
        p.logAcc+=log->speed;
        int drift=(int)p.logAcc; p.logAcc-=drift;
        p.px+=drift;
        if(p.px<0.0||p.px>=COLS){ p.alive=false; return; }
        if(!findObs(cur,p.px))  { p.alive=false; return; }
    } else {
        p.logAcc=0.0;
    }
    if(cur && cur->type==ROAD){
        if(findObs(cur,p.px)) p.alive=false;
    }
}

// ═══════════════════════════════════════════════════════════════════
//  3D Isometric ASCII Renderer
// ═══════════════════════════════════════════════════════════════════

// Color palette per lane type
struct LaneColors {
    int top_fg, top_bg;      // top face
    int left_fg, left_bg;    // left face (shadow)
    int right_fg, right_bg;  // right face (darker shadow)
};

LaneColors laneColors(LaneType t){
    switch(t){
        case SAFE:  return {238,29,  240,22,  237,22};
        case GRASS: return {240,28,  242,22,  237,22};
        case ROAD:  return {244,240, 241,236, 238,235};
        case WATER: return {39, 27,  33, 17,  25, 17};
    }
    return {240,28,242,22,237,22};
}

// Draw one isometric tile block at screen (sx,sy) = top-left of top face
// tw=TILE_W, th=TILE_H, sh=SIDE_H
void drawIsoTile(Screen& scr, int sx, int sy,
                 int top_fg, int top_bg,
                 int left_fg, int left_bg,
                 int right_fg, int right_bg,
                 const std::string& top_fill,   // character for top face
                 int priority=0)
{
    // Top face: diamond shape   ╱‾‾‾‾╲
    //                          │      │
    //                           ╲____╱
    // We approximate with lines of chars:
    // Row 0 (apex):    "   __   "  -> 2 chars at center
    // Row 1 (mid top): " /    \ "
    // Row 2 (base of top face) = edge shared with sides

    // Simple isometric top: slanted parallelogram
    // Top face rows (TILE_H=3):
    //  y+0: " /‾‾‾‾\ "   (but we do ASCII)
    //  y+1: "/      \" 
    //  y+2: "‾‾‾‾‾‾‾"   (shared edge)

    // Using box-drawing chars for crispness:
    // Top face (parallelogram, 6-wide, 3-tall):
    //   row0:  "  /--\ "    (shifted by lane_rel for iso)
    //   row1:  " /    \"
    //   row2:  "/______"  <- this is the shared edge with left/right sides

    // We render each cell individually:
    // Top face cell map for 6-wide, 3-tall:
    // We'll use a 2-row top + 1 shared row approach.

    // Top parallelogram (TILE_W=6, TILE_H=3):
    // The top face goes: starting at sx, sy
    //   Characters per row to make a parallelogram:

    // Row sy+0:  "  /==\ "  (positions sx..sx+5)
    // Row sy+1:  " /====\ " but only 6 wide -> " /=====
    // Actually let's do a clean flat-top iso:

    // ISO top face: left-right columns staggered
    // We use a 2:1 isometric style (2 chars wide, 1 char tall per step)

    // For TILE_W=6, TILE_H=3 block:
    // row 0:  cols [0,1]=space  [2,3]="__"  [4,5]=space    -> "  __  "
    // row 1:  col 0='/'  [1..4]=fill  col 5='\'             -> "/fill\"
    // row 2:  col 0='\' [1..4]=fill   col 5='/'  <- bottom of top face
    //         (or we attach sides below row 1)

    // Actually standard ASCII iso uses this top:
    // +------+
    // We'll do:
    //   row0:  "  .--. "  -> (tw=6)  positions 0-5
    //   row1:  " /    \ "  <- but that's 8 chars.
    // Let's just do it for tw=6:
    //   row0:  " .--.  " not great
    // Best approach for tw=6, th=2 top + sh=2 sides:

    // TOP FACE (th rows):
    //   th=2:
    //     row0: " /TTTT\"   width 6: pos0='/',pos1-4=fill,pos5='\'
    //     row1: "/TTTTTT\"  width 8... too wide
    // Use tw=6 strictly:
    //     row0: "/====\" (6 chars: '/' + 4*fill + '\')
    //     row1: bottom edge shared with sides

    // SIDES (sh=2 rows below top):
    //   Left face  (goes down-left):  '|' + fill chars
    //   Right face (goes down-right): fill chars + '|'

    // Let's use this layout (tw=6, top_rows=2, side_rows=2):
    // sy-1:  "  /--\  "   <- notch top (optional)
    // sy  :  " /TTTT\ "   <- top face row 0
    // sy+1:  "/TTTTTT\"   <- but only 6 wide...

    // Final decision: tw=6 tile, 2-row top face, 2-row sides:
    // sy:    /TTTT\    (6 chars starting at sx)
    // sy+1:  LLLLRR    left-face chars on left half, right-face on right
    // sy+2:  LLLLRR    second row of sides

    // This is a bit squat but very readable in terminal. Let's go:

    const int tw=TILE_W, th=1, sh=SIDE_H;
    // top face = th rows (1 row, tw wide, using '/' and '\' borders)
    // Actually let's do th=2:
    //   row 0: " /TT\ " (tw=6)  -> ' ' / T T \ space  -> 6 chars
    //   row 1: "/TTTT\"          -> / T T T T \         -> 6 chars

    // Top face: 2 rows
    // Row sy+0:  pos sx+0=' '  sx+1='/'  sx+2..sx+3=top_fill  sx+4='\'  sx+5=' '
    // Row sy+1:  pos sx+0='/'  sx+1..sx+4=top_fill  sx+5='\'
    {
        // Row 0 of top face
        scr.put(sx+0, sy,   ' ',  top_fg, top_bg, false, priority);
        scr.put(sx+1, sy,   '/', top_fg, top_bg, false, priority);
        for(int i=2;i<=3;++i) scr.put(sx+i, sy, top_fill.empty()?'_':top_fill[0], top_fg, top_bg, false, priority);
        scr.put(sx+4, sy,  '\\', top_fg, top_bg, false, priority);
        scr.put(sx+5, sy,   ' ',  top_fg, top_bg, false, priority);
    }
    {
        // Row 1 of top face
        scr.put(sx+0, sy+1, '/', top_fg, top_bg, false, priority);
        for(int i=1;i<=4;++i) scr.put(sx+i, sy+1, top_fill.empty()?'_':top_fill[0], top_fg, top_bg, false, priority);
        scr.put(sx+5, sy+1, '\\', top_fg, top_bg, false, priority);
    }
    // Side faces: sh=2 rows
    for(int r=0;r<sh;++r){
        int ry = sy+2+r;
        // Left face: left half (cols 0..2)
        scr.put(sx+0, ry, '|', left_fg, left_bg, false, priority);
        scr.put(sx+1, ry, ' ', left_fg, left_bg, false, priority);
        scr.put(sx+2, ry, ' ', left_fg, left_bg, false, priority);
        // Right face: right half (cols 3..5)
        scr.put(sx+3, ry, ' ', right_fg, right_bg, false, priority);
        scr.put(sx+4, ry, ' ', right_fg, right_bg, false, priority);
        scr.put(sx+5, ry, '|', right_fg, right_bg, false, priority);
    }
    // Bottom edge
    {
        int ry=sy+2+sh;
        scr.put(sx+0, ry, '\\', left_fg,  left_bg,  false, priority);
        scr.put(sx+1, ry, '_',  left_fg,  left_bg,  false, priority);
        scr.put(sx+2, ry, '_',  left_fg,  left_bg,  false, priority);
        scr.put(sx+3, ry, '_',  right_fg, right_bg, false, priority);
        scr.put(sx+4, ry, '_',  right_fg, right_bg, false, priority);
        scr.put(sx+5, ry, '/',  right_fg, right_bg, false, priority);
    }
}

// Draw an obstacle (car or log) on top of a lane tile
// obsX = obstacle col (world), laneRel = lane's relative Z, originX/Y = iso origin
void drawObstacle(Screen& scr, double obsX, int obsWidth, char glyph, LaneType lt,
                  int laneRel, int originX, int originY, int priority)
{
    bool isCar = (glyph=='=');
    int fg = isCar ? 196 : 94;   // red car, brown log
    int bg = isCar ? 160 : 58;
    int sfg= isCar ? 124 : 52;
    int sbg= isCar ? 88  : 22;

    for(int c=0;c<obsWidth;++c){
        int col=(int)obsX+c;
        if(col<0||col>=COLS) continue;
        int sx=isoX(col,laneRel,originX);
        int sy=isoY(col,laneRel,originY);

        // top face override
        scr.put(sx+0, sy,   ' ',  fg, bg, false, priority);
        scr.put(sx+1, sy,   '/', fg, bg, false, priority);
        scr.put(sx+2, sy, isCar?'=':(c==0?'(':glyph), fg, bg, false, priority);
        scr.put(sx+3, sy, isCar?'=':(c==obsWidth-1?')':glyph), fg, bg, false, priority);
        scr.put(sx+4, sy,  '\\', fg, bg, false, priority);
        scr.put(sx+5, sy,   ' ',  fg, bg, false, priority);

        scr.put(sx+0, sy+1, '/', fg, bg, false, priority);
        char mid= isCar ? (c==0?'>':(c==obsWidth-1?'>':'-')) : '#';
        for(int i=1;i<=4;++i) scr.put(sx+i, sy+1, mid, fg, bg, false, priority);
        scr.put(sx+5, sy+1, '\\', fg, bg, false, priority);

        // sides (shorter for obstacles)
        scr.put(sx+0, sy+2, '|', sfg, sbg, false, priority);
        scr.put(sx+1, sy+2, ' ', sfg, sbg, false, priority);
        scr.put(sx+2, sy+2, ' ', sfg, sbg, false, priority);
        scr.put(sx+3, sy+2, ' ', sfg, sbg, false, priority);
        scr.put(sx+4, sy+2, ' ', sfg, sbg, false, priority);
        scr.put(sx+5, sy+2, '|', sfg, sbg, false, priority);

        scr.put(sx+0, sy+3, '\\', sfg, sbg, false, priority);
        scr.put(sx+1, sy+3, '_',  sfg, sbg, false, priority);
        scr.put(sx+2, sy+3, '_',  sfg, sbg, false, priority);
        scr.put(sx+3, sy+3, '_',  sfg, sbg, false, priority);
        scr.put(sx+4, sy+3, '_',  sfg, sbg, false, priority);
        scr.put(sx+5, sy+3, '/',  sfg, sbg, false, priority);
    }
}

// Draw player chicken (@) as a little 3D block with a face
void drawPlayer(Screen& scr, int col, int laneRel, int originX, int originY, bool alive, float hopT){
    int sx=isoX(col,laneRel,originX);
    int sy=isoY(col,laneRel,originY);

    // Hop: small vertical bounce
    int hop=0;
    if(hopT<1.0f){
        float t=hopT*2.0f-1.0f; // -1..1
        hop = (int)(3.0f*(1.0f-t*t)); // parabola, max 3
    }
    sy -= hop;

    int pri = 200;
    if(!alive){
        // Death X
        scr.put(sx+1, sy,   'X', 196, -1, true, pri);
        scr.put(sx+2, sy,   'X', 196, -1, true, pri);
        scr.put(sx+3, sy,   'X', 196, -1, true, pri);
        scr.put(sx+4, sy,   'X', 196, -1, true, pri);
        scr.put(sx+2, sy+1, 'X', 196, -1, true, pri);
        scr.put(sx+3, sy+1, 'X', 196, -1, true, pri);
        return;
    }

    // Chicken body (golden yellow)
    int pfg=226, pbg=220, psfg=214, psbg=172;
    // Top face
    scr.put(sx+0, sy,   ' ',  pfg, pbg, true, pri);
    scr.put(sx+1, sy,   '/', pfg, pbg, true, pri);
    scr.put(sx+2, sy,  '^',  pfg, pbg, true, pri);
    scr.put(sx+3, sy,  'v',  pfg, pbg, true, pri);
    scr.put(sx+4, sy,  '\\', pfg, pbg, true, pri);
    scr.put(sx+5, sy,   ' ',  pfg, pbg, true, pri);

    scr.put(sx+0, sy+1, '/', pfg, pbg, true, pri);
    scr.put(sx+1, sy+1, 'o', 232, pbg, true, pri); // eye
    scr.put(sx+2, sy+1, '@', pfg, pbg, true, pri);
    scr.put(sx+3, sy+1, '>', 208, pbg, true, pri); // beak
    scr.put(sx+4, sy+1, ')', pfg, pbg, true, pri);
    scr.put(sx+5, sy+1,'\\', pfg, pbg, true, pri);

    // sides
    scr.put(sx+0, sy+2, '|', psfg, psbg, true, pri);
    scr.put(sx+1, sy+2, '_', psfg, psbg, true, pri);
    scr.put(sx+2, sy+2, '_', psfg, psbg, true, pri);
    scr.put(sx+3, sy+2, '_', psfg, psbg, true, pri);
    scr.put(sx+4, sy+2, '_', psfg, psbg, true, pri);
    scr.put(sx+5, sy+2, '|', psfg, psbg, true, pri);

    scr.put(sx+0, sy+3, '\\', psfg, psbg, true, pri);
    scr.put(sx+1, sy+3, 'w',  psfg, psbg, true, pri); // foot
    scr.put(sx+2, sy+3, ' ',  psfg, psbg, true, pri);
    scr.put(sx+3, sy+3, ' ',  psfg, psbg, true, pri);
    scr.put(sx+4, sy+3, 'w',  psfg, psbg, true, pri); // foot
    scr.put(sx+5, sy+3, '/',  psfg, psbg, true, pri);
}

// ── HUD overlay drawn after world ──────────────────────────────────
void drawHUD(Screen& scr, int score, int best, bool dead, int W, int H){
    // Top bar background
    for(int x=0;x<W;++x){
        scr.put(x,0,' ',250,232,false,500);
        scr.put(x,1,' ',250,232,false,500);
    }
    // Title
    std::string title=" CROSSY ROAD 3D ";
    scr.putStr(1,0,title,255,232,true,500);
    // Score
    std::string sc="SCR:"+std::to_string(score)+"  BEST:"+std::to_string(best)+" ";
    scr.putStr(W-(int)sc.size()-1,0,sc,250,232,false,500);

    // Controls hint (bottom)
    for(int x=0;x<W;++x) scr.put(x,H-1,' ',240,232,false,500);
    if(dead){
        std::string msg="  *** GAME OVER ***   R=restart  Q=quit  ";
        scr.putStr(1,H-1,msg,196,232,true,500);
    } else {
        std::string msg=" W/^ fwd  S/v back  A/< left  D/> right  Q quit ";
        scr.putStr(1,H-1,msg,240,232,false,500);
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Main render function
// ═══════════════════════════════════════════════════════════════════
void render(Screen& scr, const World& w, const Player& p, int hi){
    scr.clear();

    int SW=scr.W, SH=scr.H;

    // Isometric origin: center-bottom of visible area
    // Player's lane should appear at laneRel=3 (a few rows from bottom)
    // originX: screen col where col=playerCol, laneRel=0 maps
    // We want player tile centered horizontally.
    // Player col = (int)p.px
    // At laneRel=PR, col=PC: sx = originX + (PC-PR)*TW/2
    //                         sy = originY - (PC+PR)*TH/2
    // For player (PC,PR=playerRel), tile top should be at screen row ~SH-10
    const int PLAYER_REL = 3; // lanes below player visible
    const int PC = (int)p.px;
    const int PR = PLAYER_REL;

    // Player's tile top on screen: SH-10 (leaving room for sides and HUD)
    int player_screen_y = SH - 12;
    int player_screen_x = SW / 2 - PC * (TILE_W/2) + PR * (TILE_W/2); // center

    // originX, originY from:
    // sx = originX + (PC - PR)*3  =>  originX = player_sx - (PC-PR)*3
    // sy = originY - (PC + PR)*2  =>  originY = player_sy + (PC+PR)*2  (TILE_H/2=1 since int)
    const int TW2 = TILE_W/2; // 3
    const int TH2 = TILE_H/2; // 1  (we use 1 for vertical iso step)

    int originX = SW/2 - (PC - PR) * TW2;
    int originY = player_screen_y + (PC + PR) * TH2;

    // Each row drawn back-to-front (higher laneRel = further back = drawn first)
    // laneRel=0 -> closest to camera (player side)
    // We draw VIS_LANES lanes total
    // Lane at laneRel=r corresponds to absZ = p.absZ - PLAYER_REL + r

    // Draw lanes back to front (large laneRel first)
    for(int r=VIS_LANES; r>=0; --r){
        int absZ = p.absZ - PLAYER_REL + r;
        Lane* lane = w.laneAt(absZ);
        if(!lane) continue;

        LaneColors lc = laneColors(lane->type);

        // Choose top fill char
        char tf = ' ';
        if(lane->type==SAFE)  tf='_';
        if(lane->type==ROAD)  tf='.';
        if(lane->type==GRASS) tf=',';
        if(lane->type==WATER) tf='~';

        std::string ts(1,tf);

        // Draw all COLS tiles in this lane (col 0..COLS-1)
        // Draw right-to-left for correct painter's order in isometric view
        for(int col=COLS-1; col>=0; --col){
            int sx = originX + (col - r) * TW2;
            int sy = originY - (col + r) * TH2;
            int pri = (VIS_LANES - r) * 10 + (COLS - col); // back-to-front, right-to-left

            // Water: animated ripple color
            int top_bg = lc.top_bg;
            if(lane->type==WATER){
                // slight color variation for ripple feel
                top_bg = ((col+absZ)%2==0) ? 27 : 26;
            }

            drawIsoTile(scr, sx, sy,
                        lc.top_fg, top_bg,
                        lc.left_fg, lc.left_bg,
                        lc.right_fg, lc.right_bg,
                        ts, pri);
        }

        // Draw obstacles on this lane
        for(auto* n=lane->obs.head; n; n=n->next){
            const Obstacle& o=n->data;
            int pri2=(VIS_LANES-r)*10 + 5 + 20;
            drawObstacle(scr, o.x, o.width, o.glyph, lane->type,
                         r, originX, originY, pri2);
        }

        // Draw player on their lane
        if(absZ == p.absZ){
            int pcol = (int)p.px;
            int ppri = (VIS_LANES-r)*10 + 5 + 50;
            drawPlayer(scr, pcol, r, originX, originY, p.alive, p.hopT);
        }
    }

    // HUD
    drawHUD(scr, p.score, hi, !p.alive, SW, SH);
}

// ═══════════════════════════════════════════════════════════════════
//  Input
// ═══════════════════════════════════════════════════════════════════
enum Key { K_NONE, K_UP, K_DOWN, K_LEFT, K_RIGHT, K_QUIT, K_RESTART };

Key pollKey(){
    int k=Term::read_key();
    switch(k){
        case 1000: case 'w': case 'W': return K_UP;
        case 1001: case 's': case 'S': return K_DOWN;
        case 1002: case 'd': case 'D': return K_RIGHT;
        case 1003: case 'a': case 'A': return K_LEFT;
        case 'q': case 'Q':           return K_QUIT;
        case 'r': case 'R':           return K_RESTART;
        default:                       return K_NONE;
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Splash screen
// ═══════════════════════════════════════════════════════════════════
void splash(){
    Term::clear();
    Term::bold(); Term::fg256(226);
    std::cout <<
"\n"
"  ╔═══════════════════════════════════════════════════════════════╗\n"
"  ║                                                               ║\n"
"  ║   ██████╗██████╗  ██████╗ ███████╗███████╗██╗   ██╗          ║\n"
"  ║  ██╔════╝██╔══██╗██╔═══██╗██╔════╝██╔════╝╚██╗ ██╔╝          ║\n"
"  ║  ██║     ██████╔╝██║   ██║███████╗███████╗ ╚████╔╝           ║\n"
"  ║  ██║     ██╔══██╗██║   ██║╚════██║╚════██║  ╚██╔╝            ║\n"
"  ║  ╚██████╗██║  ██║╚██████╔╝███████║███████║   ██║             ║\n"
"  ║   ╚═════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚══════╝   ╚═╝             ║\n"
"  ║                     R  O  A  D                                ║\n";
    Term::reset(); Term::fg256(214);
    std::cout <<
"  ║             C++ Terminal Edition  v3.0  \"3D ISO\"              ║\n"
"  ╠═══════════════════════════════════════════════════════════════╣\n";
    Term::reset();
    std::cout <<
"  ║                                                               ║\n"
"  ║  CONTROLS                                                     ║\n"
"  ║    W / ▲   Move forward (away from camera)                    ║\n"
"  ║    S / ▼   Move back                                          ║\n"
"  ║    A / ◄   Move left                                          ║\n"
"  ║    D / ►   Move right                                         ║\n"
"  ║    R       Restart after death                                ║\n"
"  ║    Q       Quit                                               ║\n"
"  ║                                                               ║\n"
"  ║  WORLD                                                        ║\n"
"  ║   /^v\\  You — the golden chicken                              ║\n"
"  ║   /=>\\  Car — dodge on road lanes  ( . . . )                  ║\n"
"  ║   /(#)  Log — ride across water   ( ~ ~ ~ )                   ║\n"
"  ║   /___  Safe zone                 ( _ _ _ )                   ║\n"
"  ║                                                               ║\n"
"  ║  DATA STRUCTURES                                              ║\n"
"  ║    SLinkedList<Obstacle>  per-lane singly-linked obs list      ║\n"
"  ║    World (DLL of Lane)    pushTop/popBottom infinite scroll    ║\n"
"  ║    Screen framebuffer     full isometric painter's algorithm   ║\n"
"  ║    Isometric projection   (col,laneRel) → (screenX,screenY)   ║\n"
"  ║                                                               ║\n"
"  ╚═══════════════════════════════════════════════════════════════╝\n\n";
    Term::fg256(244);
    std::cout << "  Terminal must be ≥120×40. Press ENTER to start...\n\n";
    Term::reset();
    std::cin.ignore(100000,'\n');
}

// ═══════════════════════════════════════════════════════════════════
//  Game loop
// ═══════════════════════════════════════════════════════════════════
void runGame(){
    srand((unsigned)time(nullptr));

    int SW = std::max(Term::term_cols(), 120);
    int SH = std::max(Term::term_rows()-1, 40);

    Screen scr(SW, SH);
    World  world;
    Player player;
    int    hi=0;
    bool   quit=false;

    buildWorld(world);
    ensureLanes(world, player.absZ);

    using Clock=std::chrono::steady_clock;
    using MS=std::chrono::milliseconds;
    const long TICK_MS=100, FRAME_MS=33;
    auto lastTick=Clock::now(), lastFrame=Clock::now();
    const float HOP_SPEED=0.15f;

    Term::raw();
    Term::hide_cursor();
    Term::clear();

    render(scr, world, player, hi);
    scr.flush();

    while(!quit){
        auto now=Clock::now();

        // Input
        Key key=pollKey();
        int dx=0, dz=0; bool moved=false;
        switch(key){
            case K_QUIT: quit=true; continue;
            case K_RESTART:
                if(!player.alive){
                    { World tmp; std::swap(world,tmp); }
                    s_gen=0; player=Player();
                    buildWorld(world); ensureLanes(world,player.absZ);
                    Term::clear();
                }
                continue;
            case K_UP:    dz= 1; moved=true; break;
            case K_DOWN:  dz=-1; moved=true; break;
            case K_LEFT:  dx=-1; moved=true; break;
            case K_RIGHT: dx= 1; moved=true; break;
            default: break;
        }
        if(player.alive && moved){
            tryMove(world,player,dx,dz);
            ensureLanes(world,player.absZ);
        }

        // Hop animation
        if(player.hopT < 1.0f){
            player.hopT += HOP_SPEED;
            if(player.hopT>1.0f) player.hopT=1.0f;
        }

        // Physics tick
        if(std::chrono::duration_cast<MS>(now-lastTick).count()>=TICK_MS){
            lastTick=now;
            physTick(world,player);
            ensureLanes(world,player.absZ);
            if(player.score>hi) hi=player.score;
        }

        // Render
        if(std::chrono::duration_cast<MS>(now-lastFrame).count()>=FRAME_MS){
            lastFrame=now;
            render(scr,world,player,hi);
            scr.flush();
        }

        std::this_thread::sleep_for(MS(8));
    }

    Term::show_cursor();
    Term::restore();
    Term::clear();
    std::cout << "\n  CROSSY ROAD 3D — C++ v3.0\n"
              << "  Score : " << player.score << "\n"
              << "  Best  : " << hi           << "\n\n";
}

// ═══════════════════════════════════════════════════════════════════
//  Entry
// ═══════════════════════════════════════════════════════════════════
static void cleanup(int){ Term::show_cursor(); Term::restore(); exit(0); }

int main(){
    signal(SIGINT,cleanup); signal(SIGTERM,cleanup);
    splash();
    runGame();
    return 0;
}
