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
#include <signal.h>
#include <cstdint>

// ═══════════════════════════════════════════════════════════════
//  ANSI palette
// ═══════════════════════════════════════════════════════════════
namespace A
{
    inline constexpr const char *R = "\033[0m";
    inline constexpr const char *DIM = "\033[2m";
    inline constexpr const char *BOLD = "\033[1m";
    inline constexpr const char *ITAL = "\033[3m";
    inline constexpr const char *UL = "\033[4m";
    inline constexpr const char *BLINK = "\033[5m";
    inline constexpr const char *REV = "\033[7m";
    inline constexpr const char *W = "\033[97m";
    inline constexpr const char *BW = "\033[1;97m";
    inline constexpr const char *LG = "\033[37m";
    inline constexpr const char *MG = "\033[90m";
    inline constexpr const char *DG = "\033[2;37m";
    inline constexpr const char *CYN = "\033[96m";
    inline constexpr const char *BCYN = "\033[1;96m";
    inline constexpr const char *YEL = "\033[93m";
    inline constexpr const char *BYEL = "\033[1;93m";
    inline constexpr const char *GRN = "\033[92m";
    inline constexpr const char *BGRN = "\033[1;92m";
    inline constexpr const char *MAG = "\033[95m";
    inline constexpr const char *BMAG = "\033[1;95m";
    inline constexpr const char *RED = "\033[91m";
    inline constexpr const char *BRED = "\033[1;91m";
    inline constexpr const char *INV = "\033[30;47m";
    inline constexpr const char *HC = "\033[?25l";
    inline constexpr const char *SC = "\033[?25h";

    inline std::string go(int r, int c)
    {
        return "\033[" + std::to_string(r) + ";" + std::to_string(c) + "H";
    }
    inline std::string fg(int r, int g, int b)
    {
        return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
    }
    inline std::string clrScr() { return "\033[2J\033[H"; }
    inline std::string clrEOL() { return "\033[K"; }
}

// ═══════════════════════════════════════════════════════════════
//  Global terminal size
// ═══════════════════════════════════════════════════════════════
static int gCols = 80, gRows = 24;
static void updateTermSize()
{
    struct winsize w{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
    {
        gCols = (w.ws_col > 20) ? w.ws_col : 80;
        gRows = (w.ws_row > 8) ? w.ws_row : 24;
    }
}
static volatile sig_atomic_t gResized = 0;
static void sigwinchHandler(int) { gResized = 1; }

// ═══════════════════════════════════════════════════════════════
//  RawMode RAII
// ═══════════════════════════════════════════════════════════════
class RawMode
{
public:
    RawMode()
    {
        tcgetattr(STDIN_FILENO, &saved_);
        struct termios t = saved_;
        t.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        t.c_oflag &= ~OPOST;
        t.c_cflag |= CS8;
        t.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        t.c_cc[VMIN] = 0;
        t.c_cc[VTIME] = 1;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
        std::cout << A::HC << std::flush;
        active_ = true;
    }
    ~RawMode() { restore(); }
    void restore()
    {
        if (!active_)
            return;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_);
        std::cout << A::SC << A::R << std::flush;
        active_ = false;
    }
    const struct termios &saved() const { return saved_; }

private:
    struct termios saved_{};
    bool active_ = false;
};
static RawMode *gRaw = nullptr;

// ═══════════════════════════════════════════════════════════════
//  Input helpers
// ═══════════════════════════════════════════════════════════════
static int readKey()
{
    unsigned char c = 0;
    int n = read(STDIN_FILENO, &c, 1);
    return (n == 1) ? (int)c : 0;
}

static std::string readLine(const std::string &def = "")
{
    if (!gRaw)
    {
        std::string s;
        std::getline(std::cin, s);
        // trim
        while (!s.empty() && std::isspace((unsigned char)s.front()))
            s.erase(s.begin());
        while (!s.empty() && std::isspace((unsigned char)s.back()))
            s.pop_back();
        return s.empty() ? def : s;
    }
    // Temporarily restore canonical mode
    struct termios t = gRaw->saved();
    t.c_lflag |= (ECHO | ICANON);
    t.c_oflag |= OPOST;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
    std::cout << A::SC << std::flush;

    std::string s;
    std::getline(std::cin, s);

    // Re-apply raw mode
    struct termios r = gRaw->saved();
    r.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    r.c_oflag &= ~OPOST;
    r.c_cflag |= CS8;
    r.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    r.c_cc[VMIN] = 0;
    r.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &r);
    std::cout << A::HC << std::flush;

    while (!s.empty() && std::isspace((unsigned char)s.front()))
        s.erase(s.begin());
    while (!s.empty() && std::isspace((unsigned char)s.back()))
        s.pop_back();
    return s.empty() ? def : s;
}

// ═══════════════════════════════════════════════════════════════
//  String / display utilities
// ═══════════════════════════════════════════════════════════════
static std::string lo(const std::string &s)
{
    std::string r = s;
    for (char &c : r)
        c = (char)std::tolower((unsigned char)c);
    return r;
}

static std::string trimS(const std::string &s)
{
    const char *ws = " \t\r\n";
    size_t a = s.find_first_not_of(ws);
    if (a == std::string::npos)
        return "";
    size_t b = s.find_last_not_of(ws);
    return s.substr(a, b - a + 1);
}

// Visible length — strips ANSI escapes, counts UTF-8 leading bytes
static int visLen(const std::string &s)
{
    int len = 0;
    bool inEsc = false;
    for (size_t i = 0; i < s.size(); ++i)
    {
        unsigned char c = (unsigned char)s[i];
        if (c == '\033')
        {
            inEsc = true;
            continue;
        }
        if (inEsc)
        {
            // CSI ends on alphabetic; OSC ends on BEL or ST
            if (std::isalpha(c) || c == '\a')
                inEsc = false;
            continue;
        }
        // Count only leading bytes of UTF-8 multibyte (not continuation bytes)
        if ((c & 0xC0) != 0x80)
            ++len;
    }
    return len;
}

// Pad right to visual width w
static std::string padR(const std::string &s, int w)
{
    if (w <= 0)
        return "";
    int vl = visLen(s);
    if (vl >= w)
    {
        if (vl == w)
            return s;
        // Truncate
        std::string out;
        int vis = 0;
        bool inEsc = false;
        for (size_t i = 0; i < s.size() && vis < w - 3;)
        {
            unsigned char c = (unsigned char)s[i];
            if (c == '\033')
            {
                inEsc = true;
                out += s[i++];
                continue;
            }
            if (inEsc)
            {
                out += s[i];
                if (std::isalpha(c))
                    inEsc = false;
                ++i;
                continue;
            }
            if ((c & 0xC0) != 0x80)
                ++vis;
            out += s[i++];
        }
        out += A::R;
        out += "...";
        return out;
    }
    return s + std::string((size_t)(w - vl), ' ');
}

static std::string padL(const std::string &s, int w)
{
    if (w <= 0)
        return "";
    int vl = visLen(s);
    if (vl >= w)
        return s;
    return std::string((size_t)(w - vl), ' ') + s;
}

static std::string rep(const std::string &s, int n)
{
    if (n <= 0 || s.empty())
        return "";
    std::string r;
    r.reserve(s.size() * (size_t)n);
    for (int i = 0; i < n; i++)
        r += s;
    return r;
}

static std::string fmtTime(int sec)
{
    if (sec < 0)
        sec = 0;
    char b[16];
    std::snprintf(b, sizeof(b), "%02d:%02d", sec / 60, sec % 60);
    return b;
}
static std::string fmtTimeLong(int sec)
{
    if (sec < 0)
        sec = 0;
    char b[32];
    std::snprintf(b, sizeof(b), "%dh %02dm %02ds", sec / 3600, (sec % 3600) / 60, sec % 60);
    return b;
}
static int parseSecs(const std::string &dur)
{
    int m = 0, s = 0;
    std::sscanf(dur.c_str(), "%d:%d", &m, &s);
    return m * 60 + s;
}
static std::string starStr(int r)
{
    r = std::max(0, std::min(5, r));
    std::string s;
    for (int i = 1; i <= 5; i++)
        s += (i <= r ? "★" : "☆");
    return s;
}

// ═══════════════════════════════════════════════════════════════
//  Box drawing
// ═══════════════════════════════════════════════════════════════
static std::string boxTop(int w) { return "╔" + rep("═", w - 2) + "╗"; }
static std::string boxBot(int w) { return "╚" + rep("═", w - 2) + "╝"; }
static std::string boxDiv(int w) { return "╠" + rep("═", w - 2) + "╣"; }

// Row: ║ <content padded to w-4> ║
static std::string boxRow(const std::string &content, int w)
{
    int avail = w - 4;
    if (avail < 0)
        avail = 0;
    int vl = visLen(content);
    std::string inner;
    if (vl <= avail)
    {
        inner = content + std::string((size_t)(avail - vl), ' ');
    }
    else
    {
        int vis = 0;
        bool inEsc = false;
        for (size_t i = 0; i < content.size() && vis < avail;)
        {
            unsigned char c = (unsigned char)content[i];
            if (c == '\033')
            {
                inEsc = true;
                inner += content[i++];
                continue;
            }
            if (inEsc)
            {
                inner += content[i];
                if (std::isalpha(c))
                    inEsc = false;
                ++i;
                continue;
            }
            if ((c & 0xC0) != 0x80)
                ++vis;
            inner += content[i++];
        }
        inner += A::R;
    }
    return std::string("║ ") + inner + " ║";
}

// ═══════════════════════════════════════════════════════════════
//  Visual primitives
// ═══════════════════════════════════════════════════════════════

static std::string spectrumBar(int tick, int width, bool playing)
{
    if (width <= 0)
        return "";
    if (!playing)
    {
        std::string s;
        for (int i = 0; i < width; i++)
            s += "▁";
        return s;
    }
    const char *blk[] = {" ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    std::string bar;
    bar.reserve((size_t)width * 4);
    for (int i = 0; i < width; i++)
    {
        float x = (float)i / (float)(width > 1 ? width - 1 : 1);
        float t = (float)tick * 0.08f;
        float v = 0.4f * std::sin(x * 6.28f * 2.1f + t * 1.3f) + 0.3f * std::sin(x * 6.28f * 5.3f + t * 2.1f + 1.0f) + 0.2f * std::sin(x * 6.28f * 11.7f + t * 3.7f + 2.0f) + 0.1f * std::sin(x * 6.28f * 23.0f + t * 5.1f + 0.5f);
        float env = 1.0f - std::pow(2.0f * x - 1.0f, 4.0f);
        v = (v * env + 1.0f) * 0.5f;
        int idx = (int)(v * 8.4f);
        if (idx > 8)
            idx = 8;
        if (idx < 0)
            idx = 0;
        bar += blk[idx];
    }
    return bar;
}

static std::string pbar(float fill, int width)
{
    if (width < 2)
        return "";
    fill = std::max(0.0f, std::min(1.0f, fill));
    const char *sub[] = {" ", "▏", "▎", "▍", "▌", "▋", "▊", "▉", "█"};
    float total = fill * (float)width;
    std::string bar;
    bar.reserve((size_t)width * 4);
    for (int i = 0; i < width; i++)
    {
        float diff = total - (float)i;
        if (diff >= 1.0f)
            bar += "█";
        else if (diff > 0.0f)
        {
            int idx = (int)(diff * 8.0f);
            if (idx > 8)
                idx = 8;
            bar += sub[idx];
        }
        else
            bar += "░";
    }
    return bar;
}

static std::string vbar(int vol, int w)
{
    if (w <= 0)
        return "";
    int f = (int)((float)vol / 100.0f * (float)w);
    std::string b;
    for (int i = 0; i < w; i++)
        b += (i < f ? "█" : "░");
    return b;
}

// ═══════════════════════════════════════════════════════════════
//  Lyrics / Karaoke engine
// ═══════════════════════════════════════════════════════════════
struct LyricLine
{
    int timeMs = 0;
    int timeSec = 0;
    std::string text;
};

static int parseLRCTime(const std::string &tok)
{
    int m = 0, s = 0, cs = 0;
    std::sscanf(tok.c_str(), "%d:%d.%d", &m, &s, &cs);
    return (m * 60 + s) * 1000 + cs * 10;
}

static std::vector<LyricLine> parseLRC(const std::string &lrc)
{
    std::vector<LyricLine> lines;
    std::istringstream ss(lrc);
    std::string line;
    while (std::getline(ss, line))
    {
        if (line.empty() || line[0] != '[')
            continue;
        size_t pos = 0;
        std::vector<int> times;
        while (pos < line.size() && line[pos] == '[')
        {
            size_t close = line.find(']', pos);
            if (close == std::string::npos)
                break;
            std::string tag = line.substr(pos + 1, close - pos - 1);
            if (!tag.empty() && std::isdigit((unsigned char)tag[0]))
                times.push_back(parseLRCTime(tag));
            pos = close + 1;
        }
        std::string text = trimS(line.substr(pos));
        for (int t : times)
        {
            LyricLine ll;
            ll.timeMs = t;
            ll.timeSec = t / 1000;
            ll.text = text;
            lines.push_back(ll);
        }
    }
    std::sort(lines.begin(), lines.end(), [](const LyricLine &a, const LyricLine &b)
              { return a.timeMs < b.timeMs; });
    return lines;
}

static int activeLyric(const std::vector<LyricLine> &lines, int posMs)
{
    if (lines.empty())
        return -1;
    int best = 0;
    for (int i = 0; i < (int)lines.size(); i++)
    {
        if (lines[i].timeMs <= posMs)
            best = i;
        else
            break;
    }
    return best;
}

// ═══════════════════════════════════════════════════════════════
//  Built-in lyrics
// ═══════════════════════════════════════════════════════════════
static std::map<std::string, std::string> gBuiltinLyrics;

static void initBuiltinLyrics()
{
    gBuiltinLyrics["Comfortably Numb"] =
        "[00:00.00]Hello?\n"
        "[00:04.00]Is there anybody in there?\n"
        "[00:09.00]Just nod if you can hear me\n"
        "[00:14.00]Is there anyone at home?\n"
        "[00:20.00]Come on now\n"
        "[00:23.00]I hear you're feeling down\n"
        "[00:28.00]Well I can ease your pain\n"
        "[00:33.00]Get you on your feet again\n"
        "[00:39.00]Relax\n"
        "[00:43.00]I'll need some information first\n"
        "[00:49.00]Just the basic facts\n"
        "[00:53.00]Can you show me where it hurts?\n"
        "[01:00.00]There is no pain, you are receding\n"
        "[01:06.00]A distant ship, smoke on the horizon\n"
        "[01:13.00]You are only coming through in waves\n"
        "[01:19.00]Your lips move but I can't hear what you're saying\n"
        "[01:27.00]When I was a child I had a fever\n"
        "[01:33.00]My hands felt just like two balloons\n"
        "[01:39.00]Now I've got that feeling once again\n"
        "[01:45.00]I can't explain, you would not understand\n"
        "[01:51.00]This is not how I am\n"
        "[01:57.00]I have become comfortably numb\n"
        "[02:10.00]Okay\n"
        "[02:14.00]Just a little pin prick\n"
        "[02:18.00]There'll be no more — Aaah!\n"
        "[02:28.00]Can you stand up?\n"
        "[02:31.00]I do believe it's working, good\n"
        "[02:36.00]That'll keep you going through the show\n"
        "[02:41.00]Come on, it's time to go\n"
        "[02:48.00]There is no pain, you are receding\n"
        "[02:54.00]A distant ship, smoke on the horizon\n"
        "[03:01.00]You are only coming through in waves\n"
        "[03:07.00]Your lips move but I can't hear what you're saying\n"
        "[03:15.00]When I was a child\n"
        "[03:19.00]I caught a fleeting glimpse\n"
        "[03:23.00]Out of the corner of my eye\n"
        "[03:29.00]I turned to look but it was gone\n"
        "[03:35.00]I cannot put my finger on it now\n"
        "[03:41.00]The child is grown, the dream is gone\n"
        "[03:49.00]I have become comfortably numb\n";

    gBuiltinLyrics["Hurt"] =
        "[00:00.00]I hurt myself today\n"
        "[00:05.00]To see if I still feel\n"
        "[00:10.00]I focus on the pain\n"
        "[00:15.00]The only thing that's real\n"
        "[00:20.00]The needle tears a hole\n"
        "[00:25.00]The old familiar sting\n"
        "[00:30.00]Try to kill it all away\n"
        "[00:35.00]But I remember everything\n"
        "[00:42.00]What have I become?\n"
        "[00:47.00]My sweetest friend\n"
        "[00:52.00]Everyone I know\n"
        "[00:57.00]Goes away in the end\n"
        "[01:03.00]And you could have it all\n"
        "[01:09.00]My empire of dirt\n"
        "[01:15.00]I will let you down\n"
        "[01:21.00]I will make you hurt\n"
        "[01:30.00]I wear this crown of thorns\n"
        "[01:35.00]Upon my liar's chair\n"
        "[01:41.00]Full of broken thoughts\n"
        "[01:46.00]I cannot repair\n"
        "[01:52.00]Beneath the stains of time\n"
        "[01:57.00]The feelings disappear\n"
        "[02:03.00]You are someone else\n"
        "[02:08.00]I am still right here\n"
        "[02:15.00]What have I become?\n"
        "[02:20.00]My sweetest friend\n"
        "[02:25.00]Everyone I know\n"
        "[02:30.00]Goes away in the end\n"
        "[02:36.00]And you could have it all\n"
        "[02:42.00]My empire of dirt\n"
        "[02:48.00]I will let you down\n"
        "[02:54.00]I will make you hurt\n"
        "[03:05.00]If I could start again\n"
        "[03:11.00]A million miles away\n"
        "[03:17.00]I would keep myself\n"
        "[03:23.00]I would find a way\n";

    gBuiltinLyrics["Pyramid Song"] =
        "[00:00.00]♪ Intro ♪\n"
        "[00:12.00]I jumped in the river\n"
        "[00:17.00]And what did I see?\n"
        "[00:22.00]Black-eyed angels swam with me\n"
        "[00:28.00]A moon full of stars\n"
        "[00:33.00]And astral cars\n"
        "[00:38.00]And all the figures I used to see\n"
        "[00:44.00]All my lovers were there\n"
        "[00:49.00]With me there\n"
        "[00:54.00]All my past and futures\n"
        "[01:00.00]And we all went to heaven\n"
        "[01:06.00]In a little row boat\n"
        "[01:12.00]There was nothing to fear\n"
        "[01:18.00]And nothing to doubt\n"
        "[01:28.00]I jumped into the river\n"
        "[01:33.00]Black-eyed angels swam with me\n"
        "[01:40.00]A moon full of stars and astral cars\n"
        "[01:48.00]All the figures I used to see\n"
        "[01:55.00]All my lovers were there with me\n"
        "[02:02.00]All my past and futures\n"
        "[02:09.00]And we all went to heaven in a little row boat\n"
        "[02:18.00]There was nothing to fear\n"
        "[02:25.00]And nothing to doubt\n"
        "[02:35.00]♪ Piano solo ♪\n"
        "[02:50.00]There was nothing to fear\n"
        "[02:57.00]And nothing to doubt\n"
        "[03:05.00]Nothing to fear\n"
        "[03:12.00]Nothing to doubt\n";

    gBuiltinLyrics["Echoes"] =
        "[00:00.00]♪ Opening ♪\n"
        "[01:30.00]Overhead the albatross\n"
        "[01:38.00]Hangs motionless upon the air\n"
        "[01:46.00]And deep beneath the rolling waves\n"
        "[01:54.00]In labyrinths of coral caves\n"
        "[02:02.00]The echo of a distant tide\n"
        "[02:10.00]Comes willowing across the sand\n"
        "[02:18.00]And everything is green and submarine\n"
        "[02:30.00]And no one showed us to the land\n"
        "[02:38.00]And no one knows the where's or why's\n"
        "[02:46.00]But something stirs and something tries\n"
        "[02:54.00]And starts to climb toward the light\n"
        "[03:05.00]Strangers passing in the street\n"
        "[03:13.00]By chance two separate glances meet\n"
        "[03:21.00]And I am you and what I see is me\n"
        "[03:30.00]And do I take you by the hand\n"
        "[03:38.00]And lead you through the land\n"
        "[03:46.00]And help me understand the best I can?\n"
        "[04:05.00]♪ Instrumental ♪\n"
        "[08:00.00]No one calls us to move on\n"
        "[08:10.00]And no one forces down our eyes\n"
        "[08:20.00]No one speaks and no one tries\n"
        "[08:30.00]No one flies around the sun\n"
        "[22:30.00]And following our will and wind\n"
        "[22:40.00]We may just go where no one's been\n"
        "[22:50.00]We'll ride the spiral to the end\n"
        "[23:00.00]And may just go where no one's been\n";

    gBuiltinLyrics["Black"] =
        "[00:00.00]Hey, oh\n"
        "[00:08.00]Sheets of empty canvas\n"
        "[00:12.00]Untouched sheets upon the bed\n"
        "[00:17.00]The night is so divided\n"
        "[00:26.00]And me for what's ahead\n"
        "[00:31.00]Paint the black rose\n"
        "[00:36.00]And ask the dawn\n"
        "[00:41.00]To hold me every breath\n"
        "[00:47.00]I know I'm just the dreamer\n"
        "[01:02.00]That I ever dreamed she'd be\n"
        "[01:10.00]And all I taught her was everything I know\n"
        "[01:26.00]And now my bitter hands\n"
        "[01:31.00]Chafe beneath the clouds\n"
        "[01:36.00]Of what was everything\n"
        "[01:41.00]Oh the pictures have all been washed in black\n"
        "[01:50.00]Tattooed everything\n"
        "[01:55.00]All I gave to you\n"
        "[02:00.00]All I have is black\n"
        "[02:08.00]I know someday you'll have a beautiful life\n"
        "[02:15.00]I know you'll be a star\n"
        "[02:21.00]In somebody else's sky\n"
        "[02:27.00]But why, why, why\n"
        "[02:33.00]Can't it be mine?\n";
}

static std::string getLyrics(const std::string &title)
{
    auto it = gBuiltinLyrics.find(title);
    return (it != gBuiltinLyrics.end()) ? it->second : "";
}

// ═══════════════════════════════════════════════════════════════
//  Song model
// ═══════════════════════════════════════════════════════════════
struct Song
{
    std::string title, artist, genre, duration, tags, addedDate;
    int year = 0, playCount = 0, rating = 0;
    int toSecs() const { return parseSecs(duration); }
    bool valid() const { return !title.empty() && !artist.empty(); }
};

// ═══════════════════════════════════════════════════════════════
//  Circular doubly-linked list node
// ═══════════════════════════════════════════════════════════════
struct Node
{
    Song song;
    Node *prev = nullptr;
    Node *next = nullptr;
    explicit Node(const Song &s) : song(s) {}
};

enum class RepeatMode
{
    NONE,
    ONE,
    ALL
};
static const char *rLabel(RepeatMode m)
{
    switch (m)
    {
    case RepeatMode::NONE:
        return "OFF";
    case RepeatMode::ONE:
        return " 1 ";
    default:
        return "ALL";
    }
}

struct Bookmark
{
    std::string songTitle, songArtist, label;
    int positionSec = 0;
};

struct HistEntry
{
    std::string title, artist, playedAt;
    int listenSec = 0;
};

// ═══════════════════════════════════════════════════════════════
//  MusicPlayer
// ═══════════════════════════════════════════════════════════════
class MusicPlayer
{
public:
    enum class SortKey
    {
        TITLE,
        ARTIST,
        GENRE,
        YEAR,
        PLAYS,
        RATING,
        DURATION
    };

    MusicPlayer()
    {
        srand((unsigned)time(nullptr));
        updateDate();
    }
    ~MusicPlayer() { freeList(); }

    // ── I/O ──────────────────────────────────────────────────
    void loadCSV(const std::string &file)
    {
        csvFile_ = file;
        std::ifstream f(file);
        if (!f.is_open())
            return;
        std::string line;
        int dupes = 0;
        while (std::getline(f, line))
        {
            if (line.empty())
                continue;
            Song s = parseLine(line);
            if (!s.valid())
                continue;
            if (!insertBack(s, true))
                ++dupes;
        }
        dupesOnLoad_ = dupes;
    }
    void saveCSV() const
    {
        if (!csvFile_.empty())
            saveCSV(csvFile_);
    }
    void saveCSV(const std::string &file) const
    {
        if (file.empty())
            return;
        std::ofstream f(file, std::ios::trunc);
        if (!f.is_open())
            return;
        if (!head_)
            return;
        Node *n = head_;
        do
        {
            f << ce(n->song.title) << ',' << ce(n->song.artist) << ','
              << ce(n->song.genre) << ',' << n->song.year << ','
              << ce(n->song.duration) << ',' << n->song.playCount << ','
              << n->song.rating << ',' << ce(n->song.tags) << ','
              << ce(n->song.addedDate) << '\n';
            n = n->next;
        } while (n != head_);
    }
    void saveBookmarks(const std::string &file) const
    {
        std::ofstream f(file, std::ios::trunc);
        if (!f.is_open())
            return;
        for (const auto &b : bookmarks_)
            f << ce(b.songTitle) << ',' << ce(b.songArtist) << ','
              << b.positionSec << ',' << ce(b.label) << '\n';
    }
    void loadBookmarks(const std::string &file)
    {
        std::ifstream f(file);
        if (!f.is_open())
            return;
        std::string line;
        while (std::getline(f, line))
        {
            if (line.empty())
                continue;
            auto flds = splitCSV(line);
            if (flds.size() < 4)
                continue;
            Bookmark b;
            b.songTitle = flds[0];
            b.songArtist = flds[1];
            try
            {
                b.positionSec = std::stoi(flds[2]);
            }
            catch (...)
            {
            }
            b.label = flds[3];
            bookmarks_.push_back(b);
        }
    }

    // ── Playlist mutation ─────────────────────────────────────
    bool insertBack(const Song &s, bool silent = false)
    {
        if (isDupe(s))
        {
            if (!silent)
                lastMsg_ = "[!] Duplicate: " + s.title + " - " + s.artist;
            return false;
        }
        Node *nd = new Node(s);
        if (!head_)
        {
            head_ = cur_ = nd;
            nd->next = nd->prev = nd;
        }
        else
        {
            Node *tail = head_->prev;
            tail->next = nd;
            nd->prev = tail;
            nd->next = head_;
            head_->prev = nd;
        }
        ++size_;
        return true;
    }

    bool deleteCurrent()
    {
        if (!cur_)
            return false;
        Node *v = cur_;
        if (size_ == 1)
        {
            delete v;
            head_ = cur_ = nullptr;
            size_ = 0;
            return true;
        }
        cur_ = v->next;
        if (v == head_)
            head_ = v->next;
        v->prev->next = v->next;
        v->next->prev = v->prev;
        delete v;
        --size_;
        elapsedMs_ = 0;
        return true;
    }

    // ── Transport ─────────────────────────────────────────────
    void next(bool /*autoplay*/ = false)
    {
        if (!cur_)
            return;
        pushHistory();
        cur_ = cur_->next;
        elapsedMs_ = 0;
        playing_ = true;
    }
    void prev()
    {
        if (!cur_)
            return;
        if (elapsedMs_ > 3000)
        {
            elapsedMs_ = 0;
            return;
        }
        cur_ = cur_->prev;
        elapsedMs_ = 0;
        playing_ = true;
    }
    void jumpRandom()
    {
        if (!head_ || size_ < 2)
            return;
        pushHistory();
        int skip = rand() % (size_ - 1) + 1;
        Node *n = cur_ ? cur_ : head_;
        for (int i = 0; i < skip; i++)
            n = n->next;
        cur_ = n;
        elapsedMs_ = 0;
        playing_ = true;
    }
    void seekSec(int delta)
    {
        if (!cur_)
            return;
        elapsedMs_ += delta * 1000;
        int maxMs = cur_->song.toSecs() * 1000;
        if (maxMs <= 0)
            maxMs = 30000;
        elapsedMs_ = std::max(0, std::min(elapsedMs_, maxMs - 500));
    }
    void warpTo(float pct)
    {
        if (!cur_)
            return;
        int maxMs = cur_->song.toSecs() * 1000;
        if (maxMs <= 0)
            maxMs = 30000;
        pct = std::max(0.0f, std::min(1.0f, pct));
        elapsedMs_ = (int)(pct * (float)maxMs);
    }

    // ── Tick (called every 80 ms) ─────────────────────────────
    void advance(int ms = 80)
    {
        if (!playing_ || !cur_)
            return;
        elapsedMs_ += ms;
        sessionListenMs_ += ms;
        totalListenMs_ += ms;
        ++tick_;
        int total = cur_->song.toSecs() * 1000;
        if (total <= 0)
            total = 30000;
        if (elapsedMs_ >= total)
        {
            cur_->song.playCount++;
            pushHistory();
            switch (repeat_)
            {
            case RepeatMode::ONE:
                elapsedMs_ = 0;
                break;
            case RepeatMode::ALL:
                cur_ = cur_->next;
                elapsedMs_ = 0;
                break;
            case RepeatMode::NONE:
                if (cur_->next == head_)
                {
                    // End of list — stop
                    elapsedMs_ = total;
                    playing_ = false;
                }
                else
                {
                    cur_ = cur_->next;
                    elapsedMs_ = 0;
                }
                break;
            }
        }
    }

    // ── Playlist ops ──────────────────────────────────────────
    void shuffle()
    {
        if (size_ < 2)
            return;
        std::vector<Song> arr;
        arr.reserve(size_);
        Node *n = head_;
        do
        {
            arr.push_back(n->song);
            n = n->next;
        } while (n != head_);
        for (int i = (int)arr.size() - 1; i > 0; i--)
        {
            int j = rand() % (i + 1);
            std::swap(arr[i], arr[j]);
        }
        std::string ct = cur_ ? cur_->song.title : "";
        std::string ca = cur_ ? cur_->song.artist : "";
        freeList();
        for (const auto &s : arr)
            insertBack(s, true);
        if (!ct.empty())
        {
            Node *nc = findNode(ct, ca);
            if (nc)
                cur_ = nc;
        }
        lastMsg_ = "Shuffled " + std::to_string(size_) + " songs.";
    }

    void sortBy(SortKey key, bool descending = false)
    {
        if (size_ < 2)
            return;
        std::vector<Song> arr;
        arr.reserve(size_);
        Node *n = head_;
        do
        {
            arr.push_back(n->song);
            n = n->next;
        } while (n != head_);
        auto cmp = [&](const Song &a, const Song &b) -> bool
        {
            bool r = false;
            switch (key)
            {
            case SortKey::TITLE:
                r = lo(a.title) < lo(b.title);
                break;
            case SortKey::ARTIST:
                r = lo(a.artist) < lo(b.artist);
                break;
            case SortKey::GENRE:
                r = lo(a.genre) < lo(b.genre);
                break;
            case SortKey::YEAR:
                r = a.year < b.year;
                break;
            case SortKey::PLAYS:
                r = a.playCount > b.playCount;
                break;
            case SortKey::RATING:
                r = a.rating > b.rating;
                break;
            case SortKey::DURATION:
                r = a.toSecs() < b.toSecs();
                break;
            }
            return descending ? !r : r;
        };
        std::stable_sort(arr.begin(), arr.end(), cmp);
        std::string ct = cur_ ? cur_->song.title : "";
        std::string ca = cur_ ? cur_->song.artist : "";
        freeList();
        for (const auto &s : arr)
            insertBack(s, true);
        if (!ct.empty())
        {
            Node *nc = findNode(ct, ca);
            if (nc)
                cur_ = nc;
        }
    }

    void editField(const std::string &field, const std::string &val)
    {
        if (!cur_)
            return;
        if (field == "title")
            cur_->song.title = val;
        else if (field == "artist")
            cur_->song.artist = val;
        else if (field == "genre")
            cur_->song.genre = val;
        else if (field == "duration")
            cur_->song.duration = val;
        else if (field == "tags")
            cur_->song.tags = val;
        else if (field == "year")
        {
            try
            {
                cur_->song.year = std::stoi(val);
            }
            catch (...)
            {
            }
        }
        else if (field == "rating")
        {
            try
            {
                int r = std::stoi(val);
                cur_->song.rating = std::max(0, std::min(5, r));
            }
            catch (...)
            {
            }
        }
    }

    // ── Search / filter ───────────────────────────────────────
    std::vector<Node *> search(const std::string &q) const
    {
        std::vector<Node *> res;
        if (!head_)
            return res;
        std::string ql = lo(q);
        Node *n = head_;
        do
        {
            if (lo(n->song.title).find(ql) != std::string::npos ||
                lo(n->song.artist).find(ql) != std::string::npos ||
                lo(n->song.genre).find(ql) != std::string::npos ||
                lo(n->song.tags).find(ql) != std::string::npos)
                res.push_back(n);
            n = n->next;
        } while (n != head_);
        return res;
    }

    std::vector<Node *> byGenre(const std::string &genre) const
    {
        std::vector<Node *> res;
        if (!head_)
            return res;
        std::string gl = lo(genre);
        Node *n = head_;
        do
        {
            if (lo(n->song.genre) == gl)
                res.push_back(n);
            n = n->next;
        } while (n != head_);
        return res;
    }

    std::vector<std::string> genres() const
    {
        std::set<std::string> s;
        if (!head_)
            return {};
        Node *n = head_;
        do
        {
            if (!n->song.genre.empty())
                s.insert(n->song.genre);
            n = n->next;
        } while (n != head_);
        return {s.begin(), s.end()};
    }

    // ── Bookmarks ─────────────────────────────────────────────
    void addBookmark(const std::string &label)
    {
        if (!cur_)
            return;
        Bookmark b;
        b.songTitle = cur_->song.title;
        b.songArtist = cur_->song.artist;
        b.positionSec = elapsedMs_ / 1000;
        b.label = label;
        bookmarks_.push_back(b);
        lastMsg_ = "Bookmark: " + label + " @ " + fmtTime(b.positionSec);
    }
    void jumpToBookmark(int idx)
    {
        if (idx < 0 || (size_t)idx >= bookmarks_.size())
            return;
        const Bookmark &b = bookmarks_[(size_t)idx];
        Node *n = findNode(b.songTitle, b.songArtist);
        if (n)
        {
            cur_ = n;
            elapsedMs_ = b.positionSec * 1000;
            playing_ = true;
        }
    }
    const std::vector<Bookmark> &bookmarks() const { return bookmarks_; }
    void removeBookmark(int idx)
    {
        if (idx >= 0 && (size_t)idx < bookmarks_.size())
            bookmarks_.erase(bookmarks_.begin() + idx);
    }

    // ── Rating ────────────────────────────────────────────────
    void rateCurrent(int stars)
    {
        if (!cur_)
            return;
        cur_->song.rating = std::max(0, std::min(5, stars));
        lastMsg_ = "Rated: " + starStr(cur_->song.rating);
    }

    // ── Stats helpers ─────────────────────────────────────────
    std::string topArtist() const
    {
        std::map<std::string, int> m;
        if (!head_)
            return "-";
        Node *n = head_;
        do
        {
            m[n->song.artist] += n->song.playCount;
            n = n->next;
        } while (n != head_);
        auto best = std::max_element(m.begin(), m.end(),
                                     [](const auto &a, const auto &b)
                                     { return a.second < b.second; });
        return (best != m.end()) ? best->first : "-";
    }
    std::string topGenre() const
    {
        std::map<std::string, int> m;
        if (!head_)
            return "-";
        Node *n = head_;
        do
        {
            m[n->song.genre] += n->song.playCount;
            n = n->next;
        } while (n != head_);
        auto best = std::max_element(m.begin(), m.end(),
                                     [](const auto &a, const auto &b)
                                     { return a.second < b.second; });
        return (best != m.end()) ? best->first : "-";
    }
    int totalPlayCount() const
    {
        int t = 0;
        if (!head_)
            return 0;
        Node *n = head_;
        do
        {
            t += n->song.playCount;
            n = n->next;
        } while (n != head_);
        return t;
    }
    int totalDurationSec() const
    {
        int t = 0;
        if (!head_)
            return 0;
        Node *n = head_;
        do
        {
            t += n->song.toSecs();
            n = n->next;
        } while (n != head_);
        return t;
    }
    const Song *mostPlayed() const
    {
        if (!head_)
            return nullptr;
        Node *best = head_;
        Node *n = head_->next;
        while (n != head_)
        {
            if (n->song.playCount > best->song.playCount)
                best = n;
            n = n->next;
        }
        return &best->song;
    }
    const Song *highestRated() const
    {
        if (!head_)
            return nullptr;
        Node *best = head_;
        Node *n = head_->next;
        while (n != head_)
        {
            if (n->song.rating > best->song.rating)
                best = n;
            n = n->next;
        }
        return best->song.rating > 0 ? &best->song : nullptr;
    }

    // ── Wrapped stats ─────────────────────────────────────────
    void calculateStats()
    {
        topArtists_.clear();
        topSongs_.clear();
        topGenres_.clear();
        std::map<std::string, int> ap, sp, gp;
        int tp = 0, tr = 0, rs = 0;
        if (head_)
        {
            Node *n = head_;
            do
            {
                ap[n->song.artist] += n->song.playCount;
                tp += n->song.playCount;
                sp[n->song.title + " - " + n->song.artist] += n->song.playCount;
                gp[n->song.genre] += n->song.playCount;
                if (n->song.rating > 0)
                {
                    tr += n->song.rating;
                    rs++;
                }
                n = n->next;
            } while (n != head_);
        }
        auto toVec = [](std::map<std::string, int> &m)
        {
            std::vector<std::pair<std::string, int>> v(m.begin(), m.end());
            std::sort(v.begin(), v.end(), [](const auto &a, const auto &b)
                      { return a.second > b.second; });
            if (v.size() > 5)
                v.resize(5);
            return v;
        };
        topArtists_ = toVec(ap);
        topSongs_ = toVec(sp);
        topGenres_ = toVec(gp);
        totalMinutes_ = (int)(totalListenMs_ / 60000);
        avgPlaysPerSong_ = size_ > 0 ? (double)tp / size_ : 0.0;
        avgRating_ = rs > 0 ? (double)tr / rs : 0.0;

        size_t ua = ap.size(), ug = gp.size();
        if (size_ == 0)
            listeningVibe_ = "Silent Mode  —  add some songs to start!";
        else if (ua > 0 && ug > 0 && (double)ua / size_ > 0.7 && (double)ug / size_ > 0.5)
            listeningVibe_ = "The Explorer  —  always discovering new artists!";
        else if (avgPlaysPerSong_ > 5.0)
            listeningVibe_ = "The Devotee  —  you have your favourites!";
        else if (avgRating_ > 4.0)
            listeningVibe_ = "The Connoisseur  —  only the finest tracks!";
        else if (totalMinutes_ > 10000)
            listeningVibe_ = "The Audiophile  —  music is life!";
        else if (ug <= 2)
            listeningVibe_ = "The Specialist  —  you know what you like!";
        else
        {
            const char *v[] = {
                "The Night Owl  —  best listening after dark!",
                "The Road Tripper  —  every journey needs a soundtrack!",
                "The Festival Goer  —  you love variety!",
                "The Rocker  —  keeping the spirit alive!",
                "The Meditator  —  music for focus and calm!"};
            listeningVibe_ = v[rand() % 5];
        }
    }

    const std::vector<std::pair<std::string, int>> &getTopArtists() const { return topArtists_; }
    const std::vector<std::pair<std::string, int>> &getTopSongs() const { return topSongs_; }
    const std::vector<std::pair<std::string, int>> &getTopGenres() const { return topGenres_; }
    int getTotalMinutes() const { return totalMinutes_; }
    double getAvgPlaysPerSong() const { return avgPlaysPerSong_; }
    double getAvgRating() const { return avgRating_; }
    const std::string &getListeningVibe() const { return listeningVibe_; }

    // ── Accessors ─────────────────────────────────────────────
    const Song *currentSong() const { return cur_ ? &cur_->song : nullptr; }
    Node *currentNode() { return cur_; }
    bool isPlaying() const { return playing_; }
    void togglePlay() { playing_ = !playing_; }
    void setPlaying(bool v) { playing_ = v; }
    int elapsedSec() const { return elapsedMs_ / 1000; }
    int elapsedMs() const { return elapsedMs_; }
    float progress() const
    {
        if (!cur_)
            return 0.f;
        int t = cur_->song.toSecs();
        if (t <= 0)
            return 0.f;
        return std::min(1.0f, (float)elapsedMs_ / ((float)t * 1000.f));
    }
    int volume() const { return vol_; }
    void volUp(int step = 5) { vol_ = std::min(100, vol_ + step); }
    void volDown(int step = 5) { vol_ = std::max(0, vol_ - step); }
    RepeatMode repeat() const { return repeat_; }
    void cycleRepeat()
    {
        repeat_ = (repeat_ == RepeatMode::NONE) ? RepeatMode::ONE : (repeat_ == RepeatMode::ONE) ? RepeatMode::ALL
                                                                                                 : RepeatMode::NONE;
    }
    int size() const { return size_; }
    bool empty() const { return !head_; }
    Node *headNode() { return head_; }
    const Node *headNode() const { return head_; }
    int dupesOnLoad() const { return dupesOnLoad_; }
    const std::deque<HistEntry> &fullHistory() const { return histQ_; }
    std::string lastMsg() const { return lastMsg_; }
    void clearMsg() { lastMsg_.clear(); }
    int tick() const { return tick_; }
    int sessionListenSec() const { return (int)(sessionListenMs_ / 1000); }
    int totalListenSec() const { return (int)(totalListenMs_ / 1000); }
    std::string today() const { return today_; }

    int currentIndex() const
    {
        if (!head_ || !cur_)
            return -1;
        Node *n = head_;
        int i = 0;
        do
        {
            if (n == cur_)
                return i;
            n = n->next;
            ++i;
        } while (n != head_);
        return -1;
    }

private:
    Node *head_ = nullptr;
    Node *cur_ = nullptr;
    int size_ = 0;
    bool playing_ = false;
    int elapsedMs_ = 0;
    int vol_ = 80;
    RepeatMode repeat_ = RepeatMode::NONE;
    std::string csvFile_, lastMsg_, today_;
    int dupesOnLoad_ = 0;
    int tick_ = 0;
    int64_t sessionListenMs_ = 0;
    int64_t totalListenMs_ = 0;
    std::deque<HistEntry> histQ_;
    std::vector<Bookmark> bookmarks_;
    std::vector<std::pair<std::string, int>> topArtists_, topSongs_, topGenres_;
    int totalMinutes_ = 0;
    double avgPlaysPerSong_ = 0.0;
    double avgRating_ = 0.0;
    std::string listeningVibe_;

    void updateDate()
    {
        time_t now = time(nullptr);
        struct tm *t = localtime(&now);
        char buf[16];
        strftime(buf, sizeof(buf), "%Y-%m-%d", t);
        today_ = buf;
    }
    void pushHistory()
    {
        if (!cur_)
            return;
        time_t now = time(nullptr);
        struct tm *tm_ = localtime(&now);
        char buf[16];
        strftime(buf, sizeof(buf), "%H:%M:%S", tm_);
        HistEntry e;
        e.title = cur_->song.title;
        e.artist = cur_->song.artist;
        e.playedAt = buf;
        e.listenSec = elapsedMs_ / 1000;
        if (histQ_.size() >= 50)
            histQ_.pop_front();
        histQ_.push_back(e);
    }
    bool isDupe(const Song &s) const
    {
        if (!head_)
            return false;
        Node *n = head_;
        do
        {
            if (lo(n->song.title) == lo(s.title) && lo(n->song.artist) == lo(s.artist))
                return true;
            n = n->next;
        } while (n != head_);
        return false;
    }
    Node *findNode(const std::string &title, const std::string &artist) const
    {
        if (!head_)
            return nullptr;
        Node *n = head_;
        do
        {
            if (lo(n->song.title) == lo(title) && lo(n->song.artist) == lo(artist))
                return n;
            n = n->next;
        } while (n != head_);
        return nullptr;
    }
    void freeList()
    {
        if (!head_)
            return;
        Node *n = head_;
        Node *nxt;
        do
        {
            nxt = n->next;
            delete n;
            n = nxt;
        } while (n != head_);
        head_ = cur_ = nullptr;
        size_ = 0;
    }
    static Song parseLine(const std::string &line)
    {
        auto flds = splitCSV(line);
        Song s;
        if (flds.size() < 5)
            return s;
        s.title = flds[0];
        s.artist = flds[1];
        s.genre = flds[2];
        try
        {
            s.year = std::stoi(flds[3]);
        }
        catch (...)
        {
        }
        s.duration = flds[4];
        if (flds.size() > 5)
        {
            try
            {
                s.playCount = std::stoi(flds[5]);
            }
            catch (...)
            {
            }
        }
        if (flds.size() > 6)
        {
            try
            {
                s.rating = std::stoi(flds[6]);
            }
            catch (...)
            {
            }
        }
        if (flds.size() > 7)
            s.tags = flds[7];
        if (flds.size() > 8)
            s.addedDate = flds[8];
        return s;
    }
    static std::vector<std::string> splitCSV(const std::string &line)
    {
        std::vector<std::string> f;
        std::string field;
        bool inQ = false;
        for (char c : line)
        {
            if (c == '"')
            {
                inQ = !inQ;
                continue;
            }
            if (c == ',' && !inQ)
            {
                f.push_back(trimS(field));
                field.clear();
                continue;
            }
            field += c;
        }
        f.push_back(trimS(field));
        return f;
    }
    static std::string ce(const std::string &v)
    {
        if (v.find(',') == std::string::npos && v.find('"') == std::string::npos)
            return v;
        std::string r = "\"";
        for (char c : v)
        {
            if (c == '"')
                r += '"';
            r += c;
        }
        return r + '"';
    }
};

// ═══════════════════════════════════════════════════════════════
//  UI
// ═══════════════════════════════════════════════════════════════
class UI
{
public:
    enum class Mode
    {
        MAIN,
        KARAOKE,
        WARP
    };

    explicit UI(MusicPlayer &mp) : mp_(mp) {}

    void draw()
    {
        if (gResized)
        {
            gResized = 0;
            updateTermSize();
        }
        buf_.clear();
        buf_ += A::go(1, 1);
        if (mode_ == Mode::KARAOKE)
            renderKaraoke();
        else if (mode_ == Mode::WARP)
        {
            renderMain();
            renderWarpOverlay();
        }
        else
            renderMain();
        std::cout << buf_ << std::flush;
    }

    void setStatus(const std::string &m, const char *colour = A::W)
    {
        statusMsg_ = std::string(colour) + m + A::R;
        clock_gettime(CLOCK_MONOTONIC, &statusSetAt_);
    }

    Mode mode() const { return mode_; }
    void setMode(Mode m) { mode_ = m; }
    std::string &warpInput() { return warpInput_; }

    void commitWarp()
    {
        try
        {
            float pct = std::stof(warpInput_) / 100.0f;
            mp_.warpTo(pct);
            setStatus("Warped to " + warpInput_ + "%", A::GRN);
        }
        catch (...)
        {
            setStatus("[!] Invalid position.", A::RED);
        }
        warpInput_.clear();
        mode_ = Mode::MAIN;
    }

    // ── Modals ────────────────────────────────────────────────
    void modalAdd()
    {
        rawClear();
        modalHeader("ADD SONG");
        Song s;
        s.addedDate = mp_.today();
        s.title = fp("  Title              ");
        if (s.title.empty())
        {
            setStatus("[!] Title required.", A::RED);
            return;
        }
        s.artist = fp("  Artist             ");
        if (s.artist.empty())
        {
            setStatus("[!] Artist required.", A::RED);
            return;
        }
        s.genre = fp("  Genre              ");
        std::string yr = fp("  Year               ");
        try
        {
            s.year = std::stoi(yr);
        }
        catch (...)
        {
        }
        s.duration = fp("  Duration (MM:SS)   ");
        if (s.duration.empty())
            s.duration = "00:00";
        s.tags = fp("  Tags (comma-sep)   ");
        std::string rat = fp("  Rating (0-5)       ");
        try
        {
            s.rating = std::max(0, std::min(5, std::stoi(rat)));
        }
        catch (...)
        {
        }
        if (mp_.insertBack(s))
        {
            setStatus("[+] Added: " + s.title + " - " + s.artist, A::GRN);
            mp_.saveCSV();
        }
        else
        {
            setStatus(mp_.lastMsg(), A::RED);
            mp_.clearMsg();
        }
    }

    void modalEdit()
    {
        const Song *s = mp_.currentSong();
        if (!s)
        {
            setStatus("[!] No song.", A::RED);
            return;
        }
        rawClear();
        modalHeader("EDIT: " + s->title);
        auto ef = [&](const std::string &lbl, const std::string &cur, const std::string &key)
        {
            std::cout << A::MG << "  " << lbl << A::LG << cur << A::R << "\n";
            std::cout << A::MG << "  New (Enter=keep): " << A::R;
            std::string v = readLine(cur);
            if (v != cur)
                mp_.editField(key, v);
        };
        ef("Title    : ", s->title, "title");
        ef("Artist   : ", s->artist, "artist");
        ef("Genre    : ", s->genre, "genre");
        ef("Year     : ", std::to_string(s->year), "year");
        ef("Duration : ", s->duration, "duration");
        ef("Tags     : ", s->tags, "tags");
        ef("Rating   : ", std::to_string(s->rating), "rating");
        setStatus("Song updated.", A::GRN);
        mp_.saveCSV();
    }

    void modalDelete()
    {
        const Song *s = mp_.currentSong();
        if (!s)
        {
            setStatus("[!] No song.", A::RED);
            return;
        }
        rawClear();
        modalHeader("DELETE SONG");
        std::cout << A::W << "\n  " << s->title << " - " << s->artist << "\n"
                  << A::MG << "\n  Confirm? [y/N]: " << A::R << std::flush;
        std::string yn = readLine();
        if (!yn.empty() && (yn[0] == 'y' || yn[0] == 'Y'))
        {
            std::string t = s->title;
            mp_.deleteCurrent();
            setStatus("[-] Deleted: " + t, A::YEL);
            mp_.saveCSV();
        }
        else
            setStatus("Cancelled.");
    }

    void modalSearch()
    {
        rawClear();
        modalHeader("SEARCH");
        std::string q = fp("  Query             ");
        if (q.empty())
            return;
        auto results = mp_.search(q);
        std::cout << "\n";
        if (results.empty())
        {
            std::cout << A::MG << "  No results for \"" << q << "\"\n"
                      << A::R;
        }
        else
        {
            hline();
            std::cout << A::MG << "  "
                      << padR("TITLE", 30) << padR("ARTIST", 22) << "DUR    ★\n"
                      << A::R;
            hline();
            for (Node *n : results)
            {
                bool ic = (n == mp_.currentNode());
                std::string row = "  " + padR(n->song.title, 30) + padR(n->song.artist, 22) + padR(n->song.duration, 7) + std::to_string(n->song.rating);
                if (ic)
                    std::cout << A::INV << row << A::R << "\n";
                else
                    std::cout << A::LG << row << A::R << "\n";
            }
            hline();
            std::cout << A::CYN << "  " << results.size() << " result(s)\n"
                      << A::R;
        }
        waitPrompt();
    }

    void modalPlaylist()
    {
        int offset = std::max(0, mp_.currentIndex() - 5);
        while (true)
        {
            rawClear();
            int visible = gRows - 12;
            if (visible < 3)
                visible = 3;
            int total = mp_.size(), curIdx = mp_.currentIndex();
            if (offset > curIdx)
                offset = std::max(0, curIdx);
            if (offset + visible <= curIdx)
                offset = curIdx - visible + 1;
            offset = std::max(0, std::min(offset, std::max(0, total - visible)));

            modalHeader("PLAYLIST  [j/k or ↑↓=scroll  q=back]");
            std::cout << A::MG << "  " << padR("#", 5) << padR("TITLE", 28)
                      << padR("ARTIST", 20) << padR("DUR", 7) << "★  PLAYS\n"
                      << A::R;
            hline();
            if (mp_.empty())
            {
                std::cout << A::MG << "  (empty)\n"
                          << A::R;
            }
            else
            {
                Node *n = mp_.headNode();
                int idx = 0;
                do
                {
                    if (idx >= offset && idx < offset + visible)
                    {
                        bool ic = (idx == curIdx);
                        std::string row = "  " + padR(std::to_string(idx + 1) + ".", 5) + padR(n->song.title, 28) + padR(n->song.artist, 20) + padR(n->song.duration, 7) + std::to_string(n->song.rating) + "  " + std::to_string(n->song.playCount);
                        if (ic)
                            std::cout << A::INV << row << A::R << "\n";
                        else
                            std::cout << A::LG << row << A::R << "\n";
                    }
                    n = n->next;
                    ++idx;
                } while (n != mp_.headNode());
            }
            hline();
            std::cout << A::MG << "  " << total << " songs  "
                      << (offset + 1) << "-" << std::min(offset + visible, total) << "\n"
                      << A::R;

            int k = 0;
            while (k == 0)
            {
                usleep(20000);
                k = readKey();
            }
            if (k == 'q' || k == 'Q')
                break;
            if (k == 'k')
            {
                if (offset > 0)
                    --offset;
            }
            if (k == 'j')
            {
                if (offset + visible < total)
                    ++offset;
            }
            if (k == 27)
            {
                int k2 = readKey();
                if (k2 == '[')
                {
                    int k3 = readKey();
                    if (k3 == 'A' && offset > 0)
                        --offset;
                    if (k3 == 'B' && offset + visible < total)
                        ++offset;
                }
            }
        }
    }

    void modalHistory()
    {
        rawClear();
        modalHeader("PLAYBACK HISTORY");
        const auto &h = mp_.fullHistory();
        if (h.empty())
        {
            std::cout << A::MG << "  No history yet.\n"
                      << A::R;
        }
        else
        {
            hline();
            std::cout << A::MG << "  " << padR("TIME", 10) << padR("TITLE", 28)
                      << padR("ARTIST", 20) << "LISTENED\n"
                      << A::R;
            hline();
            int shown = 0;
            for (auto it = h.rbegin(); it != h.rend() && shown < 30; ++it, ++shown)
            {
                std::cout << A::LG << "  " << padR(it->playedAt, 10)
                          << padR(it->title, 28) << padR(it->artist, 20)
                          << fmtTime(it->listenSec) << A::R << "\n";
            }
            hline();
        }
        waitPrompt();
    }

    void modalStats()
    {
        rawClear();
        modalHeader("STATISTICS DASHBOARD");
        std::cout << "\n";
        auto row = [&](const std::string &k, const std::string &v)
        {
            std::cout << A::MG << "  " << padR(k, 24) << A::W << v << A::R << "\n";
        };
        std::cout << A::BCYN << "  ── Library ──\n"
                  << A::R;
        row("Total songs", std::to_string(mp_.size()));
        row("Total duration", fmtTimeLong(mp_.totalDurationSec()));
        row("Total play count", std::to_string(mp_.totalPlayCount()));
        row("Top artist", mp_.topArtist());
        row("Top genre", mp_.topGenre());
        const Song *best = mp_.mostPlayed();
        if (best)
            row("Most played", best->title + " - " + best->artist + " (" + std::to_string(best->playCount) + ")");
        const Song *rated = mp_.highestRated();
        if (rated)
            row("Highest rated", rated->title + " - " + starStr(rated->rating));

        std::cout << "\n"
                  << A::BCYN << "  ── Session ──\n"
                  << A::R;
        row("Session listen time", fmtTimeLong(mp_.sessionListenSec()));
        row("Bookmarks saved", std::to_string(mp_.bookmarks().size()));

        std::cout << "\n"
                  << A::BCYN << "  ── Genre Breakdown ──\n"
                  << A::R;
        std::map<std::string, int> gmap;
        if (!mp_.empty())
        {
            Node *n = mp_.headNode();
            do
            {
                if (!n->song.genre.empty())
                    gmap[n->song.genre]++;
                n = n->next;
            } while (n != mp_.headNode());
        }
        int maxG = 1;
        for (auto &kv : gmap)
            maxG = std::max(maxG, kv.second);
        int barW = std::min(30, gCols - 30);
        if (barW < 4)
            barW = 4;
        for (auto &kv : gmap)
        {
            int filled = (int)((float)kv.second / (float)maxG * (float)barW);
            std::cout << A::MG << "  " << padR(kv.first, 16)
                      << A::CYN << rep("█", filled)
                      << A::MG << rep("░", barW - filled)
                      << " " << kv.second << A::R << "\n";
        }
        waitPrompt();
    }

    void modalInfo()
    {
        const Song *s = mp_.currentSong();
        rawClear();
        modalHeader("SONG DETAILS");
        if (!s)
        {
            std::cout << A::MG << "  No song loaded.\n"
                      << A::R;
        }
        else
        {
            std::cout << "\n";
            auto row = [&](const std::string &k, const std::string &v)
            {
                std::cout << A::MG << "  " << padR(k, 16) << A::W << v << A::R << "\n";
            };
            row("Title", s->title);
            row("Artist", s->artist);
            row("Genre", s->genre);
            row("Year", s->year > 0 ? std::to_string(s->year) : "-");
            row("Duration", s->duration + "  (" + std::to_string(s->toSecs()) + "s)");
            row("Rating", starStr(s->rating) + "  (" + std::to_string(s->rating) + "/5)");
            row("Play Count", std::to_string(s->playCount));
            row("Tags", s->tags.empty() ? "-" : s->tags);
            row("Added", s->addedDate.empty() ? "-" : s->addedDate);
            row("Track", std::to_string(mp_.currentIndex() + 1) + " / " + std::to_string(mp_.size()));
            bool hasLyrics = !getLyrics(s->title).empty();
            row("Lyrics", hasLyrics ? "Available  —  [c] for Karaoke mode" : "None");
        }
        waitPrompt();
    }

    void modalSort()
    {
        rawClear();
        modalHeader("SORT PLAYLIST");
        std::cout << A::MG
                  << "\n  [1] Title (A-Z)\n  [2] Artist (A-Z)\n  [3] Genre\n"
                  << "  [4] Year (oldest first)\n  [5] Most Played\n  [6] Highest Rated\n"
                  << "  [7] Duration (shortest first)\n\n"
                  << "  Choice: " << A::R << std::flush;
        std::string ch = readLine();
        const char *labels[] = {"title", "artist", "genre", "year", "plays", "rating", "duration"};
        MusicPlayer::SortKey keys[] = {
            MusicPlayer::SortKey::TITLE, MusicPlayer::SortKey::ARTIST,
            MusicPlayer::SortKey::GENRE, MusicPlayer::SortKey::YEAR,
            MusicPlayer::SortKey::PLAYS, MusicPlayer::SortKey::RATING,
            MusicPlayer::SortKey::DURATION};
        int idx = -1;
        try
        {
            idx = std::stoi(ch) - 1;
        }
        catch (...)
        {
        }
        if (idx >= 0 && idx < 7)
        {
            mp_.sortBy(keys[idx]);
            setStatus(std::string("Sorted by ") + labels[idx] + ".", A::GRN);
            mp_.saveCSV();
        }
        else
            setStatus("Cancelled.");
    }

    void modalGenreFilter()
    {
        rawClear();
        modalHeader("GENRE FILTER");
        auto genres = mp_.genres();
        if (genres.empty())
        {
            std::cout << A::MG << "  No genres found.\n"
                      << A::R;
            waitPrompt();
            return;
        }
        std::cout << "\n";
        for (int i = 0; i < (int)genres.size(); i++)
            std::cout << A::MG << "  [" << (i + 1) << "]  " << A::LG << genres[i] << A::R << "\n";
        std::cout << "\n"
                  << A::MG << "  Pick genre (Enter=cancel): " << A::R << std::flush;
        std::string ch = readLine();
        int idx = -1;
        try
        {
            idx = std::stoi(ch) - 1;
        }
        catch (...)
        {
        }
        if (idx >= 0 && idx < (int)genres.size())
        {
            auto results = mp_.byGenre(genres[idx]);
            rawClear();
            modalHeader("GENRE: " + genres[idx]);
            hline();
            for (Node *n : results)
            {
                bool ic = (n == mp_.currentNode());
                std::string row = "  " + padR(n->song.title, 30) + padR(n->song.artist, 22) + n->song.duration;
                if (ic)
                    std::cout << A::INV << row << A::R << "\n";
                else
                    std::cout << A::LG << row << A::R << "\n";
            }
            hline();
            std::cout << A::MG << "  " << results.size() << " songs in genre.\n"
                      << A::R;
            waitPrompt();
        }
    }

    void modalBookmarks()
    {
        rawClear();
        modalHeader("BOOKMARKS  [a=add  d#=del  j#=jump  q=back]");
        const auto &bms = mp_.bookmarks();
        if (bms.empty())
        {
            std::cout << A::MG << "  No bookmarks yet.  Press [b] while playing.\n"
                      << A::R;
        }
        else
        {
            hline();
            for (int i = 0; i < (int)bms.size(); i++)
            {
                const auto &b = bms[i];
                std::cout << A::MG << "  [" << padL(std::to_string(i + 1), 2) << "]  "
                          << A::W << padR(b.label, 20)
                          << A::LG << padR(b.songTitle, 25)
                          << A::MG << "@ " << fmtTime(b.positionSec) << A::R << "\n";
            }
            hline();
        }
        std::cout << A::MG << "  Command: " << A::R << std::flush;
        std::string cmd = readLine();
        if (cmd.empty())
            return;
        if (cmd == "a" || cmd == "A")
        {
            std::cout << A::MG << "  Label: " << A::R << std::flush;
            std::string lbl = readLine("mark");
            mp_.addBookmark(lbl.empty() ? "mark" : lbl);
            mp_.saveCSV();
        }
        else if (!cmd.empty() && cmd[0] == 'd')
        {
            int i = -1;
            try
            {
                i = std::stoi(cmd.substr(1)) - 1;
            }
            catch (...)
            {
            }
            if (i >= 0 && i < (int)bms.size())
            {
                mp_.removeBookmark(i);
                setStatus("Bookmark removed.", A::YEL);
            }
        }
        else if (!cmd.empty() && cmd[0] == 'j')
        {
            int i = -1;
            try
            {
                i = std::stoi(cmd.substr(1)) - 1;
            }
            catch (...)
            {
            }
            if (i >= 0 && i < (int)bms.size())
            {
                mp_.jumpToBookmark(i);
                setStatus("Jumped to: " + bms[i].label, A::CYN);
            }
        }
    }

    void modalRate()
    {
        rawClear();
        modalHeader("RATE CURRENT SONG");
        const Song *s = mp_.currentSong();
        if (!s)
        {
            std::cout << A::MG << "  No song.\n"
                      << A::R;
            waitPrompt();
            return;
        }
        std::cout << "\n  " << A::W << s->title << A::MG << "  —  " << A::LG << s->artist << A::R << "\n";
        std::cout << "\n  Current: " << A::BYEL << starStr(s->rating) << A::R << "\n\n";
        std::cout << A::MG << "  New rating [0-5]: " << A::R << std::flush;
        std::string r = readLine();
        try
        {
            mp_.rateCurrent(std::stoi(r));
            mp_.saveCSV();
            // Re-fetch after rating update
            const Song *updated = mp_.currentSong();
            setStatus("Rated: " + starStr(updated ? updated->rating : 0), A::BYEL);
        }
        catch (...)
        {
            setStatus("[!] Invalid rating (0-5).", A::RED);
        }
    }

    void modalHelp()
    {
        rawClear();
        modalHeader("ANCEL  —  KEYBOARD SHORTCUTS");
        struct KV
        {
            const char *k;
            const char *d;
        };
        KV h[] = {
            {"SPACE", "Play / Pause"},
            {"← →", "Prev / Next song"},
            {"↑ ↓", "Volume up / down"},
            {"n / p", "Next / Prev (vi-style)"},
            {"+ / -", "Volume +5 / -5"},
            {"r", "Cycle repeat  OFF → 1 → ALL"},
            {"s", "Shuffle (Fisher-Yates)"},
            {"z", "Random jump"},
            {"f / B", "Seek +10s / -10s"},
            {"w", "WARP overlay (seek% + stats)"},
            {"c", "KARAOKE / Lyrics mode"},
            {"b", "Bookmark current position"},
            {"k", "Bookmark manager"},
            {"m", "Rate current song (0-5 stars)"},
            {"a", "Add song"},
            {"d", "Delete current song"},
            {"e", "Edit current song metadata"},
            {"i", "Song details panel"},
            {"/", "Search (title / artist / tag)"},
            {"l", "Playlist browser"},
            {"h", "Playback history"},
            {"t", "Statistics dashboard"},
            {"S", "Sort menu"},
            {"g", "Genre filter"},
            {"?", "This help screen"},
            {"q", "Save & quit"},
            {nullptr, nullptr}};
        std::cout << "\n";
        for (int i = 0; h[i].k; i++)
            std::cout << A::BCYN << "  " << padR(h[i].k, 14)
                      << A::MG << h[i].d << A::R << "\n";
        waitPrompt();
    }

private:
    MusicPlayer &mp_;
    std::string statusMsg_;
    struct timespec statusSetAt_ = {0, 0};
    static constexpr double STATUS_TTL = 4.0;
    Mode mode_ = Mode::MAIN;
    std::string warpInput_;
    std::string buf_; // double-buffer

    // ── Status ────────────────────────────────────────────────
    bool statusExpired()
    {
        if (statusMsg_.empty())
            return true;
        struct timespec now{};
        clock_gettime(CLOCK_MONOTONIC, &now);
        double e = (now.tv_sec - statusSetAt_.tv_sec) + (now.tv_nsec - statusSetAt_.tv_nsec) * 1e-9;
        return e >= STATUS_TTL;
    }

    // ── Buffered output ───────────────────────────────────────
    void out(const std::string &s) { buf_ += s; }
    void outln(const std::string &s = "")
    {
        buf_ += s;
        buf_ += "\n";
    }

    // Non-buffered modal helpers
    void hline(char ch = '-')
    {
        std::cout << A::MG << std::string((size_t)gCols, ch) << A::R << "\n";
    }
    // Buffered divider
    void bDiv(char ch = '─')
    {
        buf_ += A::MG;
        buf_ += std::string((size_t)gCols, ch);
        buf_ += A::R;
        buf_ += "\n";
    }

    // ── Karaoke mode ──────────────────────────────────────────
    void renderKaraoke()
    {
        const Song *s = mp_.currentSong();
        buf_ += "\033[2J\033[H";

        // Header box top
        buf_ += A::MG;
        buf_ += "╔";
        buf_ += rep("═", gCols - 2);
        buf_ += "╗\n";

        // App name + mode tag
        {
            std::string line = std::string(A::BCYN) + " ♪ ANCEL" + A::MG + "  ♫ KARAOKE MODE ♫";
            buf_ += A::MG;
            buf_ += "║ ";
            buf_ += line;
            int used = visLen(line);
            int rem = gCols - 4 - used;
            if (rem > 0)
                buf_ += std::string((size_t)rem, ' ');
            buf_ += A::MG;
            buf_ += " ║\n";
        }

        // Title / Artist
        {
            std::string titleStr = s
                                       ? std::string(A::BW) + s->title + A::MG + "  —  " + A::CYN + s->artist
                                       : std::string(A::MG) + "No song loaded";
            buf_ += A::MG;
            buf_ += "║ ";
            buf_ += titleStr;
            int tl = visLen(titleStr);
            int rem = gCols - 4 - tl;
            if (rem > 0)
                buf_ += std::string((size_t)rem, ' ');
            buf_ += A::MG;
            buf_ += " ║\n";
        }

        // Progress line
        {
            int el = mp_.elapsedSec();
            int tot = s ? s->toSecs() : 0;
            std::string elStr = fmtTime(el);
            std::string totStr = fmtTime(tot);
            int barW = gCols - 4 - (int)elStr.size() - (int)totStr.size() - 4;
            if (barW < 4)
                barW = 4;
            std::string bar = pbar(mp_.progress(), barW);

            buf_ += A::MG;
            buf_ += "║ ";
            buf_ += A::DG;
            buf_ += elStr;
            buf_ += "  ";
            buf_ += A::CYN;
            buf_ += bar;
            buf_ += A::R;
            buf_ += A::MG;
            buf_ += "  ";
            buf_ += totStr;
            int used = (int)elStr.size() + 2 + barW + 2 + (int)totStr.size();
            int rem = gCols - 4 - used;
            if (rem > 0)
                buf_ += std::string((size_t)rem, ' ');
            buf_ += A::MG;
            buf_ += " ║\n";
        }

        buf_ += A::MG;
        buf_ += "╠";
        buf_ += rep("═", gCols - 2);
        buf_ += "╣\n";

        // ── Lyrics area ───────────────────────────────────────
        int lyricsRows = gRows - 10;
        if (lyricsRows < 3)
            lyricsRows = 3;

        std::vector<LyricLine> lines;
        if (s)
        {
            std::string lrc = getLyrics(s->title);
            if (!lrc.empty())
                lines = parseLRC(lrc);
        }
        int activeIdx = activeLyric(lines, mp_.elapsedMs());

        if (lines.empty())
        {
            for (int row = 0; row < lyricsRows; row++)
            {
                buf_ += A::MG;
                buf_ += "║ ";
                if (row == lyricsRows / 2)
                {
                    std::string msg = "♪  No lyrics for this track  ♪";
                    int ml = visLen(msg);
                    int pad = std::max(0, (gCols - 4 - ml) / 2);
                    buf_ += std::string((size_t)pad, ' ');
                    buf_ += A::DG;
                    buf_ += msg;
                    buf_ += A::MG;
                    buf_ += std::string((size_t)std::max(0, gCols - 4 - ml - pad), ' ');
                }
                else
                {
                    buf_ += std::string((size_t)(gCols - 4), ' ');
                }
                buf_ += " ║\n";
            }
        }
        else
        {
            int half = (lyricsRows - 1) / 2;
            int total = (int)lines.size();

            for (int row = 0; row < lyricsRows; row++)
            {
                int li = activeIdx - half + row;
                buf_ += A::MG;
                buf_ += "║ ";

                if (li >= 0 && li < total)
                {
                    bool isActive = (li == activeIdx);
                    bool isNext = (li == activeIdx + 1);
                    int dist = std::abs(li - activeIdx);

                    const std::string &text = lines[li].text;
                    int tl = visLen(text);
                    int pad = std::max(0, (gCols - 4 - tl) / 2);

                    buf_ += std::string((size_t)pad, ' ');

                    if (isActive)
                    {
                        buf_ += A::BYEL;
                        buf_ += A::BOLD;
                        buf_ += text;
                        buf_ += A::R;
                    }
                    else if (isNext)
                    {
                        buf_ += A::W;
                        buf_ += text;
                        buf_ += A::R;
                    }
                    else if (dist <= 2)
                    {
                        buf_ += A::LG;
                        buf_ += text;
                        buf_ += A::R;
                    }
                    else
                    {
                        buf_ += A::MG;
                        buf_ += text;
                        buf_ += A::R;
                    }

                    int used = pad + tl;
                    int rem = gCols - 4 - used;
                    if (rem > 0)
                        buf_ += std::string((size_t)rem, ' ');
                }
                else
                {
                    buf_ += std::string((size_t)(gCols - 4), ' ');
                }
                buf_ += A::MG;
                buf_ += " ║\n";
            }
        }

        // Spectrum bar
        {
            int specW = gCols - 4;
            std::string spec = spectrumBar(mp_.tick(), specW, mp_.isPlaying());
            buf_ += A::MG;
            buf_ += "║ ";
            buf_ += A::DIM;
            buf_ += A::CYN;
            buf_ += spec;
            buf_ += A::R;
            buf_ += A::MG;
            buf_ += " ║\n";
        }

        buf_ += A::MG;
        buf_ += "╠";
        buf_ += rep("═", gCols - 2);
        buf_ += "╣\n";

        // Controls footer
        std::string ctrls = "  SPC:play  ←→:skip  ↑↓:vol  f/B:seek±10s  c:exit  q:quit";
        buf_ += A::MG;
        buf_ += "║";
        buf_ += A::DG;
        buf_ += padR(ctrls, gCols - 2);
        buf_ += A::MG;
        buf_ += "║\n";
        buf_ += "╚";
        buf_ += rep("═", gCols - 2);
        buf_ += "╝\n";
        buf_ += A::R;
    }

    // ── Main player ───────────────────────────────────────────
    void renderMain()
    {
        buf_ += "\033[2J\033[H";
        renderTopBar();
        renderNowPlaying();
        renderSpectrum();
        renderProgressRow();
        renderTransportRow();
        bDiv('═');
        renderPlaylist();
        renderFooter();
    }

    void renderTopBar()
    {
        // Top border
        buf_ += A::MG;
        buf_ += "╔";
        buf_ += rep("═", gCols - 2);
        buf_ += "╗\n";

        // App name | date/time | track#
        std::string appName = " ♪ ANCEL";
        time_t now = time(nullptr);
        struct tm *tm_ = localtime(&now);
        char tbuf[32];
        strftime(tbuf, sizeof(tbuf), "%a %d %b %Y  %H:%M:%S", tm_);
        std::string dateStr(tbuf);
        int idx = mp_.currentIndex();
        std::string trkStr = (idx >= 0)
                                 ? " TRK " + std::to_string(idx + 1) + "/" + std::to_string(mp_.size()) + " "
                                 : " --/-- ";

        int inner = gCols - 2;
        int usedFixed = visLen(appName) + visLen(dateStr) + visLen(trkStr);
        int gaps = inner - usedFixed;
        int g1 = gaps / 2;
        int g2 = gaps - g1;
        if (g1 < 1)
            g1 = 1;
        if (g2 < 1)
            g2 = 1;

        buf_ += A::MG;
        buf_ += "║";
        buf_ += A::BCYN;
        buf_ += appName;
        buf_ += std::string((size_t)g1, ' ');
        buf_ += A::DG;
        buf_ += dateStr;
        buf_ += std::string((size_t)g2, ' ');
        buf_ += A::MG;
        buf_ += trkStr;
        buf_ += A::MG;
        buf_ += "║\n";
        buf_ += A::MG;
        buf_ += "╠";
        buf_ += rep("═", gCols - 2);
        buf_ += "╣\n";
        buf_ += A::R;
    }

    void renderNowPlaying()
    {
        const Song *s = mp_.currentSong();

        // Line 1: play icon + title + elapsed time
        buf_ += A::MG;
        buf_ += "║ ";
        if (!s)
        {
            std::string msg = "  No song loaded  —  press [a] to add songs";
            buf_ += A::MG;
            buf_ += padR(msg, gCols - 4);
        }
        else
        {
            std::string icon = mp_.isPlaying()
                                   ? std::string(A::BCYN) + "▶ "
                                   : std::string(A::MG) + "⏸ ";
            std::string elStr = fmtTime(mp_.elapsedSec());
            int iconVis = 2;
            int elStrVis = (int)elStr.size();
            int titleW = gCols - 4 - iconVis - 1 - elStrVis;
            if (titleW < 10)
                titleW = 10;

            buf_ += icon;
            buf_ += A::BW;
            buf_ += padR(s->title, titleW);
            buf_ += " ";
            buf_ += A::MG;
            buf_ += elStr;
        }
        buf_ += A::MG;
        buf_ += " ║\n";

        // Line 2: artist | genre | year | stars | tags
        buf_ += A::MG;
        buf_ += "║ ";
        if (s)
        {
            buf_ += "  ";
            buf_ += A::CYN;
            buf_ += padR(s->artist, 22);
            buf_ += A::DG;
            buf_ += padR(s->genre, 14);
            std::string yr = s->year > 0 ? std::to_string(s->year) : "    ";
            buf_ += yr;
            buf_ += "  ";
            buf_ += A::BYEL;
            buf_ += starStr(s->rating);
            buf_ += A::R;

            int used = 2 + 22 + 14 + (int)yr.size() + 2 + visLen(starStr(s->rating));
            if (!s->tags.empty())
            {
                std::string tagStr = "  #" + s->tags;
                int tagSpace = gCols - 4 - used;
                if (tagSpace > 4)
                {
                    buf_ += A::MG;
                    buf_ += padR(tagStr, tagSpace);
                    used += tagSpace;
                }
            }
            int rem = gCols - 4 - used;
            if (rem > 0)
                buf_ += std::string((size_t)rem, ' ');
        }
        else
        {
            buf_ += std::string((size_t)(gCols - 4), ' ');
        }
        buf_ += A::R;
        buf_ += A::MG;
        buf_ += " ║\n";
    }

    void renderSpectrum()
    {
        int innerW = gCols - 4;
        if (innerW < 1)
            innerW = 1;
        std::string wave = spectrumBar(mp_.tick(), innerW, mp_.isPlaying());
        buf_ += A::MG;
        buf_ += "║ ";
        buf_ += (mp_.isPlaying() ? A::CYN : A::DG);
        buf_ += A::DIM;
        buf_ += wave;
        buf_ += A::R;
        buf_ += A::MG;
        buf_ += " ║\n";
    }

    void renderProgressRow()
    {
        const Song *s = mp_.currentSong();
        int el = mp_.elapsedSec();
        int tot = s ? s->toSecs() : 0;
        std::string elStr = fmtTime(el);
        std::string totStr = fmtTime(tot);
        int barW = gCols - 4 - (int)elStr.size() - (int)totStr.size() - 4;
        if (barW < 4)
            barW = 4;
        std::string bar = pbar(mp_.progress(), barW);

        buf_ += A::MG;
        buf_ += "║ ";
        buf_ += A::DG;
        buf_ += elStr;
        buf_ += "  ";
        buf_ += A::CYN;
        buf_ += bar;
        buf_ += A::R;
        buf_ += A::MG;
        buf_ += "  ";
        buf_ += totStr;
        int used = (int)elStr.size() + 2 + barW + 2 + (int)totStr.size();
        int rem = gCols - 4 - used;
        if (rem > 0)
            buf_ += std::string((size_t)rem, ' ');
        buf_ += A::MG;
        buf_ += " ║\n";
    }

    void renderTransportRow()
    {
        std::string volPct = std::to_string(mp_.volume()) + "%";
        std::string rpt = std::string("RPT[") + rLabel(mp_.repeat()) + "]";

        // VOL[ + 12bar + ]  space  volPct  space  rpt  space  <status>
        int fixedVis = 4 + 12 + 1 + 1 + (int)volPct.size() + 2 + (int)rpt.size() + 2;
        int statusW = gCols - 4 - fixedVis;
        if (statusW < 0)
            statusW = 0;

        buf_ += A::MG;
        buf_ += "║ ";
        buf_ += A::MG;
        buf_ += "VOL[";
        buf_ += A::CYN;
        buf_ += vbar(mp_.volume(), 12);
        buf_ += A::R;
        buf_ += A::MG;
        buf_ += "] ";
        buf_ += A::DG;
        buf_ += volPct;
        buf_ += "  ";
        buf_ += A::MG;
        buf_ += rpt;
        buf_ += "  ";

        if (!statusMsg_.empty() && statusW > 0)
        {
            if (statusExpired())
            {
                statusMsg_.clear();
                buf_ += std::string((size_t)statusW, ' ');
            }
            else
            {
                // Clip to statusW visible chars
                int vis = 0;
                bool inEsc = false;
                std::string disp;
                for (size_t i = 0; i < statusMsg_.size() && vis < statusW;)
                {
                    unsigned char c = (unsigned char)statusMsg_[i];
                    if (c == '\033')
                    {
                        inEsc = true;
                        disp += statusMsg_[i++];
                        continue;
                    }
                    if (inEsc)
                    {
                        disp += statusMsg_[i];
                        if (std::isalpha(c))
                            inEsc = false;
                        ++i;
                        continue;
                    }
                    if ((c & 0xC0) != 0x80)
                        ++vis;
                    disp += statusMsg_[i++];
                }
                if (inEsc)
                    disp += A::R;
                buf_ += disp;
                int rem = statusW - vis;
                if (rem > 0)
                    buf_ += std::string((size_t)rem, ' ');
            }
        }
        else if (statusW > 0)
        {
            buf_ += std::string((size_t)statusW, ' ');
        }
        buf_ += A::R;
        buf_ += A::MG;
        buf_ += " ║\n";
    }

    void renderPlaylist()
    {
        // Column header
        buf_ += A::MG;
        buf_ += "  " + padR("#", 5) + padR("TITLE", 28) + padR("ARTIST", 20) + padR("DUR", 7) + "STARS  PLAYS\n";
        buf_ += A::R;
        bDiv();

        if (mp_.empty())
        {
            buf_ += A::MG;
            buf_ += "  Playlist is empty  —  press [a] to add songs.\n";
            buf_ += A::R;
            return;
        }

        // Compute how many rows we can show
        static const int kOverhead = 17; // rows taken by header/footer/chrome
        int available = gRows - kOverhead;
        if (available < 2)
            available = 2;

        int total = mp_.size();
        int curIdx = mp_.currentIndex();
        int offset = curIdx - available / 2;
        offset = std::max(0, std::min(offset, std::max(0, total - available)));

        Node *n = mp_.headNode();
        int idx = 0;
        do
        {
            if (idx >= offset && idx < offset + available)
            {
                bool ic = (idx == curIdx);
                std::string numStr = padR(std::to_string(idx + 1) + ".", 5);
                std::string row = "  " + numStr + padR(n->song.title, 28) + padR(n->song.artist, 20) + padR(n->song.duration, 7) + std::to_string(n->song.rating) + "  " + std::to_string(n->song.playCount);
                if (ic)
                {
                    buf_ += A::INV;
                    buf_ += row;
                    buf_ += A::R;
                    buf_ += "\n";
                }
                else if (idx % 2 == 0)
                {
                    buf_ += A::LG;
                    buf_ += row;
                    buf_ += A::R;
                    buf_ += "\n";
                }
                else
                {
                    buf_ += A::MG;
                    buf_ += row;
                    buf_ += A::R;
                    buf_ += "\n";
                }
            }
            n = n->next;
            ++idx;
        } while (n != mp_.headNode());

        bDiv();
        int pct = (total > 0 && curIdx >= 0)
                      ? (int)((float)(curIdx + 1) / (float)total * 100)
                      : 0;
        buf_ += A::MG;
        buf_ += "  " + std::to_string(total) + " songs" + "  |  #" + std::to_string(curIdx + 1) + "  " + std::to_string(pct) + "%\n";
        buf_ += A::R;
    }

    void renderFooter()
    {
        buf_ += A::MG;
        buf_ += "╠";
        buf_ += rep("═", gCols - 2);
        buf_ += "╣\n";
        std::string keys = "  SPC:play  ←→:skip  ↑↓:vol  r:rpt  s:shuf  c:karaoke  w:warp  f/B:seek  b:bmark  m:rate  a:add  /:search  ?:help  q:quit";
        buf_ += "║";
        buf_ += A::DG;
        buf_ += padR(keys, gCols - 2);
        buf_ += A::MG;
        buf_ += "║\n";
        buf_ += "╚";
        buf_ += rep("═", gCols - 2);
        buf_ += "╝\n";
        buf_ += A::R;
    }

    // ── WARP overlay ──────────────────────────────────────────
    void renderWarpOverlay()
    {
        mp_.calculateStats();

        int bw = std::min(72, gCols - 4);
        if (bw < 40)
            bw = 40;
        int bh = 28;

        int r0 = std::max(1, (gRows - bh) / 2);
        int c0 = std::max(1, (gCols - bw) / 2);

        auto emit = [&](int row, const std::string &content)
        {
            buf_ += A::go(row, c0);
            buf_ += A::MG;
            buf_ += content;
            buf_ += A::R;
        };

        int row = r0;
        emit(row++, boxTop(bw));
        emit(row++, boxRow(std::string(A::BCYN) + "  ♪ ANCEL — LISTENING STATS  [w=close  Esc=close]", bw));
        emit(row++, boxRow("", bw));

        // Top Artists
        emit(row++, boxRow(std::string(A::BYEL) + "  TOP ARTISTS", bw));
        {
            const auto &v = mp_.getTopArtists();
            if (v.empty())
            {
                emit(row++, boxRow(std::string(A::DG) + "  No play data yet — start listening!", bw));
            }
            else
            {
                for (size_t i = 0; i < v.size() && i < 3; i++)
                {
                    std::string s = "  " + std::to_string(i + 1) + ". " + padR(v[i].first, 24) + " " + std::to_string(v[i].second) + " plays";
                    emit(row++, boxRow(std::string(A::LG) + s, bw));
                }
            }
        }
        emit(row++, boxRow("", bw));

        // Top Songs
        emit(row++, boxRow(std::string(A::BYEL) + "  TOP SONGS", bw));
        {
            const auto &v = mp_.getTopSongs();
            if (v.empty())
            {
                emit(row++, boxRow(std::string(A::DG) + "  No play data yet", bw));
            }
            else
            {
                for (size_t i = 0; i < v.size() && i < 3; i++)
                {
                    std::string s = "  " + std::to_string(i + 1) + ". " + padR(v[i].first, 30) + " " + std::to_string(v[i].second) + "×";
                    emit(row++, boxRow(std::string(A::LG) + s, bw));
                }
            }
        }
        emit(row++, boxRow("", bw));

        // Top Genres with bar
        emit(row++, boxRow(std::string(A::BYEL) + "  TOP GENRES", bw));
        {
            const auto &v = mp_.getTopGenres();
            if (v.empty())
            {
                emit(row++, boxRow(std::string(A::DG) + "  No data yet", bw));
            }
            else
            {
                int maxP = v[0].second > 0 ? v[0].second : 1;
                int barW2 = std::min(24, bw - 32);
                if (barW2 < 4)
                    barW2 = 4;
                for (size_t i = 0; i < v.size() && i < 3; i++)
                {
                    int filled = (v[i].second * barW2) / maxP;
                    std::string s = "  " + padR(v[i].first, 14) + A::CYN + rep("█", filled) + A::MG + rep("░", barW2 - filled) + " " + A::LG + std::to_string(v[i].second);
                    emit(row++, boxRow(s, bw));
                }
            }
        }
        emit(row++, boxRow("", bw));

        // Listening vibe
        emit(row++, boxRow(std::string(A::BYEL) + "  LISTENING VIBE", bw));
        emit(row++, boxRow(std::string(A::BW) + "  " + mp_.getListeningVibe(), bw));
        {
            char cbuf[120];
            std::snprintf(cbuf, sizeof(cbuf),
                          "  Avg plays/song: %.1f   Avg rating: %.1f★   %d min listened",
                          mp_.getAvgPlaysPerSong(), mp_.getAvgRating(), mp_.getTotalMinutes());
            emit(row++, boxRow(std::string(A::DG) + cbuf, bw));
        }
        emit(row++, boxRow("", bw));

        // Seek input
        emit(row++, boxRow(std::string(A::MG) + "  ─────────────────────────────────────────", bw));
        emit(row++, boxRow(std::string(A::LG) + "  Seek to % (0-100):   " + A::W + warpInput_ + A::BCYN + "█", bw));
        emit(row++, boxRow(std::string(A::DG) + "  [0-9]=type  Enter=jump  Esc=cancel", bw));
        emit(row, boxBot(bw));
        buf_ += A::R;
    }

    // ── Modal helpers ─────────────────────────────────────────
    static void rawClear()
    {
        std::cout << "\033[2J\033[H" << std::flush;
    }
    static void modalHeader(const std::string &title)
    {
        int c = gCols;
        std::cout << A::MG << std::string((size_t)c, '═') << A::R << "\n";
        int tl = visLen(title);
        int p = std::max(0, (c - tl) / 2);
        std::cout << A::BCYN << std::string((size_t)p, ' ') << title << A::R << "\n";
        std::cout << A::MG << std::string((size_t)c, '═') << A::R << "\n\n";
    }
    static std::string fp(const std::string &label)
    {
        std::cout << A::MG << label << A::CYN << ": " << A::R << std::flush;
        return readLine();
    }
    static void waitPrompt()
    {
        std::cout << A::MG << "\n  [Enter to return] " << A::R << std::flush;
        readLine();
    }
};

// ═══════════════════════════════════════════════════════════════
//  Seed data  (20 classics)
// ═══════════════════════════════════════════════════════════════
static void seedDefaults(MusicPlayer &mp, const std::string &today)
{
    // Use a struct literal array for clarity; duration stays as string
    struct SeedRow
    {
        const char *title, *artist, *genre, *dur, *tags;
        int year, playCount, rating;
    };
    SeedRow d[] = {
        {"Echoes", "Pink Floyd", "Prog Rock", "23:31", "psychedelic,epic", 1971, 3, 5},
        {"The Becoming", "Nine Inch Nails", "Industrial", "05:26", "dark,electronic", 1994, 1, 4},
        {"Clair de Lune", "Claude Debussy", "Classical", "04:54", "piano,impressionist", 1905, 2, 5},
        {"Midnight Rambler", "The Rolling Stones", "Rock", "06:52", "blues,classic", 1969, 0, 3},
        {"Weightless", "Marconi Union", "Ambient", "08:10", "relaxing,atmospheric", 2011, 0, 4},
        {"Black", "Pearl Jam", "Grunge", "05:43", "emotional,seattle", 1991, 1, 4},
        {"Retrograde", "James Blake", "Electronic", "04:11", "soul,bass", 2013, 0, 3},
        {"In a Sentimental Mood", "John Coltrane", "Jazz", "04:17", "classic,saxophone", 1963, 2, 5},
        {"Welcome to the Machine", "Pink Floyd", "Prog Rock", "07:31", "electronic,dark", 1975, 0, 3},
        {"Hurt", "Nine Inch Nails", "Industrial", "03:44", "emotional,raw", 1994, 4, 5},
        {"Gymnopedie No.1", "Erik Satie", "Classical", "03:04", "piano,peaceful", 1888, 1, 5},
        {"Comfortably Numb", "Pink Floyd", "Prog Rock", "06:22", "guitar,classic", 1979, 5, 5},
        {"So What", "Miles Davis", "Jazz", "09:22", "modal,cool", 1959, 1, 4},
        {"Motion Picture OST", "Radiohead", "Alternative", "07:09", "electronic,strings", 2000, 0, 4},
        {"A Warm Place", "Nine Inch Nails", "Ambient", "04:43", "peaceful,texture", 1994, 0, 4},
        {"Pyramid Song", "Radiohead", "Alternative", "04:49", "piano,complex", 2001, 0, 5},
        {"Shine On You Crazy Dmd", "Pink Floyd", "Prog Rock", "13:30", "guitar,epic", 1975, 0, 5},
        {"Blue in Green", "Miles Davis", "Jazz", "05:26", "trumpet,modal", 1959, 0, 4},
        {"The Fragile", "Nine Inch Nails", "Industrial", "04:35", "orchestral,dark", 1999, 0, 3},
        {"On the Run", "Pink Floyd", "Prog Rock", "03:30", "electronic,synth", 1973, 0, 3},
    };
    for (const auto &r : d)
    {
        Song s;
        s.title = r.title;
        s.artist = r.artist;
        s.genre = r.genre;
        s.duration = r.dur;
        s.tags = r.tags;
        s.year = r.year;
        s.playCount = r.playCount;
        s.rating = r.rating;
        s.addedDate = today;
        mp.insertBack(s, true);
    }
}

// ═══════════════════════════════════════════════════════════════
//  main
// ═══════════════════════════════════════════════════════════════
int main(int argc, char *argv[])
{
    const std::string CSV_FILE = (argc > 1) ? argv[1] : "playlist.csv";
    const std::string BM_FILE = CSV_FILE + ".bookmarks";

    // Register SIGWINCH
    struct sigaction sa{};
    sa.sa_handler = sigwinchHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, nullptr);

    updateTermSize();
    initBuiltinLyrics();

    MusicPlayer mp;

    // Load or seed
    {
        std::ifstream test(CSV_FILE);
        if (!test.is_open())
        {
            seedDefaults(mp, mp.today());
            mp.saveCSV(CSV_FILE);
        }
    }
    mp.loadCSV(CSV_FILE);
    mp.loadBookmarks(BM_FILE);

    RawMode raw;
    gRaw = &raw;
    UI ui(mp);

    if (mp.dupesOnLoad() > 0)
        ui.setStatus("[!] " + std::to_string(mp.dupesOnLoad()) + " duplicate(s) skipped.", A::YEL);

    if (!mp.empty())
        mp.setPlaying(true);

    bool quit = false;
    while (!quit)
    {
        usleep(80000); // ~80 ms → ~12.5 fps
        if (gResized)
        {
            updateTermSize();
        }

        mp.advance(80);
        ui.draw();

        int k = readKey();
        if (k == 0)
            continue;

        // ── WARP mode ─────────────────────────────────────────
        if (ui.mode() == UI::Mode::WARP)
        {
            if (k == 27)
            {
                // Check for escape or arrow sequence
                int k2 = readKey();
                if (k2 == 0 || k2 == 27)
                {
                    ui.setMode(UI::Mode::MAIN);
                    ui.warpInput().clear();
                }
                // else: ignore arrow key sequences in warp
            }
            else if (k == '\r' || k == '\n')
            {
                ui.commitWarp();
            }
            else if (k == 127 || k == 8)
            {
                if (!ui.warpInput().empty())
                    ui.warpInput().pop_back();
            }
            else if ((k >= '0' && k <= '9') || k == '.')
            {
                if (ui.warpInput().size() < 6)
                    ui.warpInput() += (char)k;
            }
            else if (k == 'w' || k == 'W')
            {
                ui.setMode(UI::Mode::MAIN);
                ui.warpInput().clear();
            }
            else if (k == 'q' || k == 'Q')
            {
                quit = true;
            }
            continue;
        }

        // ── KARAOKE mode ──────────────────────────────────────
        if (ui.mode() == UI::Mode::KARAOKE)
        {
            if (k == 'c' || k == 'C')
            {
                ui.setMode(UI::Mode::MAIN);
            }
            else if (k == 'q' || k == 'Q')
            {
                quit = true;
            }
            else if (k == ' ')
            {
                mp.togglePlay();
                ui.setStatus(mp.isPlaying() ? "▶ Playing" : "⏸ Paused");
            }
            else if (k == 'f')
            {
                mp.seekSec(10);
                ui.setStatus("Seek +10s");
            }
            else if (k == 'B')
            {
                mp.seekSec(-10);
                ui.setStatus("Seek -10s");
            }
            else if (k == '+' || k == '=')
            {
                mp.volUp();
                ui.setStatus("Vol " + std::to_string(mp.volume()) + "%");
            }
            else if (k == '-' || k == '_')
            {
                mp.volDown();
                ui.setStatus("Vol " + std::to_string(mp.volume()) + "%");
            }
            else if (k == 'n')
            {
                mp.next();
            }
            else if (k == 'p')
            {
                mp.prev();
            }
            else if (k == 27)
            {
                int k2 = readKey();
                if (k2 == '[')
                {
                    int k3 = readKey();
                    if (k3 == 'C')
                        mp.next();
                    else if (k3 == 'D')
                        mp.prev();
                    else if (k3 == 'A')
                    {
                        mp.volUp();
                        ui.setStatus("Vol " + std::to_string(mp.volume()) + "%");
                    }
                    else if (k3 == 'B')
                    {
                        mp.volDown();
                        ui.setStatus("Vol " + std::to_string(mp.volume()) + "%");
                    }
                }
            }
            continue;
        }

        // ── Arrow / escape sequences ──────────────────────────
        if (k == 27)
        {
            int k2 = readKey();
            if (k2 == '[')
            {
                int k3 = readKey();
                if (k3 == 'C')
                {
                    mp.next();
                }
                else if (k3 == 'D')
                {
                    mp.prev();
                }
                else if (k3 == 'A')
                {
                    mp.volUp();
                    ui.setStatus("Vol " + std::to_string(mp.volume()) + "%");
                }
                else if (k3 == 'B')
                {
                    mp.volDown();
                    ui.setStatus("Vol " + std::to_string(mp.volume()) + "%");
                }
            }
            continue;
        }

        // ── Main mode keys ────────────────────────────────────
        switch (k)
        {
        case 'q':
        case 'Q':
            quit = true;
            break;
        case ' ':
            mp.togglePlay();
            ui.setStatus(mp.isPlaying() ? "▶ Playing" : "⏸ Paused");
            break;
        case 'n':
            mp.next();
            break;
        case 'p':
            mp.prev();
            break;
        case '+':
        case '=':
            mp.volUp();
            ui.setStatus("Vol " + std::to_string(mp.volume()) + "%");
            break;
        case '-':
        case '_':
            mp.volDown();
            ui.setStatus("Vol " + std::to_string(mp.volume()) + "%");
            break;
        case 'r':
        case 'R':
            mp.cycleRepeat();
            ui.setStatus(std::string("Repeat: ") + rLabel(mp.repeat()));
            break;
        case 's':
            mp.shuffle();
            ui.setStatus("Shuffled!");
            break;
        case 'z':
        case 'Z':
            mp.jumpRandom();
            ui.setStatus("Random jump");
            break;
        case 'f':
            mp.seekSec(10);
            ui.setStatus("Seek +10s");
            break;
        case 'B':
            mp.seekSec(-10);
            ui.setStatus("Seek -10s");
            break;
        case 'w':
        case 'W':
            ui.setMode(UI::Mode::WARP);
            break;
        case 'c':
        case 'C':
            ui.setMode(UI::Mode::KARAOKE);
            break;
        case 'b':
            mp.addBookmark("bm@" + fmtTime(mp.elapsedSec()));
            mp.saveBookmarks(BM_FILE);
            ui.setStatus("Bookmark saved at " + fmtTime(mp.elapsedSec()), A::CYN);
            break;
        case 'k':
        case 'K':
            ui.modalBookmarks();
            mp.saveBookmarks(BM_FILE);
            break;
        case 'm':
        case 'M':
            ui.modalRate();
            break;
        case 'a':
        case 'A':
            ui.modalAdd();
            break;
        case 'd':
        case 'D':
            ui.modalDelete();
            break;
        case 'e':
        case 'E':
            ui.modalEdit();
            break;
        case 'i':
        case 'I':
            ui.modalInfo();
            break;
        case '/':
            ui.modalSearch();
            break;
        case 'l':
        case 'L':
            ui.modalPlaylist();
            break;
        case 'h':
        case 'H':
            ui.modalHistory();
            break;
        case 't':
        case 'T':
            ui.modalStats();
            break;
        case 'S':
            ui.modalSort();
            break;
        case 'g':
        case 'G':
            ui.modalGenreFilter();
            break;
        case '?':
            ui.modalHelp();
            break;
        default:
            break;
        }
    }

    gRaw = nullptr;
    mp.saveCSV(CSV_FILE);
    mp.saveBookmarks(BM_FILE);

    std::cout << "\033[2J\033[H";
    std::cout << A::BCYN << "\n  ♪ ANCEL — Music Player\n\n"
              << A::R;
    std::cout << A::MG << "  Playlist saved  →  " << A::W << CSV_FILE << "\n";
    std::cout << A::MG << "  Bookmarks saved →  " << A::W << BM_FILE << "\n";
    std::cout << A::MG << "\n  Goodbye.\n\n"
              << A::R << std::flush;
    return 0;
}
