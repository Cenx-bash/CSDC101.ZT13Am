#include <bits/stdc++.h>
#include <filesystem>
#include <random>
#include <chrono>
#include <optional>
using namespace std;

namespace fs = std::filesystem;

vector<string> banatPool = {
    "Ikaw ulit? Grabe consistency mo, sana all consistent.",
    "Pili mo ko? Solid. Pero di pa rin kita pipiliin sa future HAHA.",
    "Ikaw? sige, pero last ka na ha. Pagod na puso ko.",
    "Ay ikaw pala… kala ko iba… sayang.",
    "Bakit ako? May utang ka ba ng pagmamahal?",
    "Ikaw? wag ako. Marupok ako eh tangina mo.",
    "Pinili mo ko? Good choice. Bad timing.",
    "Ikaw? HAHAHA sige pero walang iyakan sa dulo.",
    "Ikaw na naman? Kala ko nakapag-move on na tayo?",
    "Ikaw pinili mo? Sure ka? Final answer? Lock-in?",
    "Ako ba talaga o wala lang ibang option?",
    "Ikaw? Sige. Pero backup pa rin kita. Fair lang.",
    "Pinipili mo ko, pero di kita mahal. Enjoy!",
    "Ok ikaw… pero baka sa dulo iba pipiliin ko.",
    "Ikaw na yan? Noted. Seen. Ignored. Char.",
    "Ikaw pinili? Lakas mo ah, lakas ng loob.",
    "Ikaw? Delikado yan. Baka mahulog ka sakin.",
    "Sige ikaw… pero wag ka mag-alala, di ako mag-stay.",
    "Ikaw? Tangina mo. Pero thank you.",
    "Pinili mo ko? Mahal mo ba ko o bored ka lang?",
    "Ikaw pala choice. Pero ako pala ending mo. HAHA.",
    "Ikaw ulit? Sige. Pero sa second life na yung effort."

};

// ANSI color helpers (works on most terminals). Toggle with --colors in runtime menu.
struct Colors {
    bool enabled = true;
    const string R = "\033[31m", G = "\033[32m", Y = "\033[33m", B = "\033[34m";
    const string M = "\033[35m", C = "\033[36m", W = "\033[37m", RESET = "\033[0m";
    string col(const string &s) const { return enabled ? s : ""; }
    string end() const { return enabled ? RESET : ""; }
} colors;

// Simple logger 
struct Logger {
    bool enabled = true;
    fs::path logfile = "logs/loops_master.log";
    Logger() {
        try {
            if (!fs::exists("logs")) fs::create_directories("logs");
            if (!fs::exists(logfile)) {
                ofstream(logfile.string(), ios::app) << "=== New session ===\n";
            }
        } catch(...) { enabled = false; }
    }
    void log(const string &s) {
        if (!enabled) return;
        try {
            ofstream ofs(logfile.string(), ios::app);
            auto now = chrono::system_clock::now();
            auto time_t_now = chrono::system_clock::to_time_t(now);
            ofs << "[" << time_t_now << "] " << s << "\n";
        } catch(...) { /* swallow */ }
    }
} logger;

// Utilities 
namespace input {
    void clear_stdin() {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    pair<bool,long long> read_int(const string &prompt, long long minv = LLONG_MIN, long long maxv = LLONG_MAX) {
        long long value;
        while (true) {
            cout << colors.col(colors.C) << prompt << colors.end();
            if (!(cin >> value)) {
                cout << colors.col(colors.Y) << "Ayos lang yan — invalid input. Try ulit.\n" << colors.end();
                clear_stdin();
                return {false,0};
            }
            if (value < minv || value > maxv) {
                cout << colors.col(colors.Y) << "Out of range. Must be between " << minv << " and " << maxv << ".\n" << colors.end();
                clear_stdin();
                return {false,0};
            }
            clear_stdin();
            return {true, value};
        }
    }

    pair<bool,double> read_double(const string &prompt) {
        double value;
        cout << colors.col(colors.C) << prompt << colors.end();
        if (!(cin >> value)) { 
            cout << colors.col(colors.Y) << "Invalid double input.\n" << colors.end(); 
            clear_stdin(); 
            return {false,0.0}; 
        }
        clear_stdin();
        return {true, value};
    }

    bool read_n_ints(int n, vector<long long> &out) {
        out.clear();
        out.reserve(n);
        cout << colors.col(colors.C) << "Enter " << n << " integers separated by spaces: " << colors.end();
        string line;
        if (!std::getline(cin, line)) return false;
        istringstream iss(line);
        long long x;
        while ((int)out.size() < n && (iss >> x)) out.push_back(x);
        if ((int)out.size() != n) {
            cout << colors.col(colors.Y) << "Kulang o sobra — kailangan eksaktong " << n << " numbers.\n" << colors.end();
            return false;
        }
        return true;
    }
}

// App class organizes all problems
class LoopsMaster {
public:
    void run() {
        banner();
        while (true) {
            show_menu();
            auto choice = menu_choice();
            if (!choice.has_value()) continue;
            switch (choice.value()) {
                case 0: farewell(); return;
                case 1: problem1(); break;
                case 2: problem2(); break;
                case 3: problem3(); break;
                case 4: problem4(); break;
                case 5: problem5(); break;
                case 6: problem6(); break;
                case 7: problem7(); break;
                case 8: toggle_colors(); break;
                case 9: toggle_logging(); break;
                default: cout << colors.col(colors.Y) << "Walang ganitong option, beshie.\n" << colors.end();
            }
            pause();
        }
    }

private:
    void banner() {
        cout << colors.col(colors.M)
             << "========================================\n"
             << "   LOOPS In SI PLUS PLUS \n"
             << "========================================\n"
             << colors.end();
        logger.log("Started session");
    }

    void show_menu() {
        cout << colors.col(colors.G)
             << "\nMenu — pili ka:\n"
             << "1) Odd & Even Counter (Enter 10 ints)\n"
             << "2) Factorial Calculator\n"
             << "3) Sum of Natural Numbers\n"
             << "4) Basic Exponent Calculator (no pow())\n"
             << "5) Exponent Steps Display\n"
             << "6) Negative Exponent Support\n"
             << "7) Row-Based Numerical Triangle\n"
             << "8) Toggle ANSI colors (current: " << (colors.enabled ? "ON" : "OFF") << ")\n"
             << "9) Toggle logging (current: " << (logger.enabled ? "ON" : "OFF") << ")\n"
             << "0) Exit\n"
             << colors.end();
    }

    optional<int> menu_choice() {
        cout << colors.col(colors.C) << "Select option: " << colors.end();
        string choice;
        cin >> choice;
        input::clear_stdin();

        if (choice == "ME" || choice == "me" || choice == "Me" ||
            choice == "AKO" || choice == "ako" || choice == "Ako") {

            // randomizer
            static random_device rd;
            static mt19937 gen(rd());
            uniform_int_distribution<> dist(0, banatPool.size() - 1);

            cout << colors.col(colors.Y)
                 << banatPool[dist(gen)] << "\n"
                 << colors.end();
            logger.log("User triggered ME/AKO easter egg");
            return nullopt; 
        }

        try {
            int num = stoi(choice);
            if (num < 0 || num > 9) {
                cout << colors.col(colors.Y) << "Walang ganon besh\n" << colors.end();
                return nullopt;
            }
            return num;
        } catch (...) {
            cout << colors.col(colors.Y) << "Di ko gets yan lods\n" << colors.end();
            return nullopt;
        }
    }

    void pause() {
        cout << colors.col(colors.B) << "\nPress Enter to return to menu..." << colors.end();
        cin.get();
    }

    void farewell() {
        cout << colors.col(colors.G) << "\nAyos ka ahh, Bye tang-galing\n" << colors.end();
        logger.log("Ended session");
    }

    // Problem 1: Odd and Even Counter (10 integers)
    void problem1() {
        cout << colors.col(colors.Y) << "\nProblem 1 — Odd and Even Counter\n" << colors.end();
        vector<long long> nums;
        bool ok = false;
        for (int tries = 0; tries < 3 && !ok; ++tries) {
            ok = input::read_n_ints(10, nums);
            if (!ok && tries < 2) cout << "Sige try ulit.\n";
        }
        if (!ok) { cout << colors.col(colors.R) << "Failed to read 10 ints. Returning.\n" << colors.end(); logger.log("P1: failed input"); return; }
        int even = 0, odd = 0;
        for (auto v : nums) (v % 2 == 0 ? even : odd)++;
        cout << colors.col(colors.G) << "Even numbers: " << even << "\nOdd numbers: " << odd << colors.end() << "\n";
        logger.log("P1: counted evens=" + to_string(even) + " odds=" + to_string(odd));
    }

    // Problem 2: Factorial Calculator (iterative, handles up to 20-ish safely with 64-bit)
    void problem2() {
        cout << colors.col(colors.Y) << "\nProblem 2 — Factorial Calculator\n" << colors.end();
        auto p = input::read_int("Enter a non-negative integer (<=20 recommended): ", 0, 20);
        if (!p.first) { cout << colors.col(colors.R) << "Invalid input. Returning.\n" << colors.end(); logger.log("P2: invalid input"); return; }
        long long n = p.second;
        unsigned long long fact = 1;
        for (unsigned long long i = 1; i <= (unsigned long long)n; ++i) fact *= i;
        cout << colors.col(colors.G) << "Factorial(" << n << ") = " << fact << colors.end() << "\n";
        logger.log("P2: n=" + to_string(n) + " result=" + to_string(fact));
    }

    // Problem 3: Sum of Natural Numbers
    void problem3() {
        cout << colors.col(colors.Y) << "\nProblem 3 — Sum of Natural Numbers\n" << colors.end();
        auto p = input::read_int("Enter a positive integer n: ", 1, LLONG_MAX);
        if (!p.first) { cout << colors.col(colors.R) << "Input fail. Returning.\n" << colors.end(); logger.log("P3: invalid"); return; }
        long long n = p.second;
        unsigned long long sum_loop = 0;
        for (long long i = 1; i <= n; ++i) sum_loop += i;
        unsigned long long sum_formula = (unsigned long long)n * (unsigned long long)(n + 1) / 2ULL;
        cout << colors.col(colors.G) << "Sum (loop) = " << sum_loop << "\nSum (formula) = " << sum_formula << colors.end() << "\n";
        logger.log("P3: n=" + to_string(n) + " sum=" + to_string(sum_loop));
    }

    // Problem 4: Basic Exponent Calculator (no pow())
    void problem4() {
        cout << colors.col(colors.Y) << "\nProblem 4 — Basic Exponent Calculator\n" << colors.end();
        auto pb = input::read_int("Enter integer base: ", LLONG_MIN, LLONG_MAX);
        auto pe = input::read_int("Enter non-negative integer exponent: ", 0, 1024); // safety cap
        if (!pb.first || !pe.first) { cout << colors.col(colors.R) << "Bad input.\n" << colors.end(); logger.log("P4: bad input"); return; }
        long long base = pb.second;
        long long exp = pe.second;
        long double res = 1.0L;
        bool overflow = false;
        for (long long i = 0; i < exp; ++i) {
            res *= (long double)base;
            if (!isfinite(res)) { overflow = true; break; }
        }
        if (overflow) cout << colors.col(colors.R) << "Result overflowed (too big). Try smaller numbers.\n" << colors.end();
        else cout << colors.col(colors.G) << base << "^" << exp << " = " << fixed << setprecision(0) << res << colors.end() << "\n";
        logger.log("P4: base=" + to_string(base) + " exp=" + to_string(exp) + " res=" + (overflow ? "OVERFLOW" : to_string((long long)res)));
    }

    // Problem 5: Display Exponent Steps
    void problem5() {
        cout << colors.col(colors.Y) << "\nProblem 5 — Display Exponent Steps\n" << colors.end();
        auto pb = input::read_int("Enter integer base: ", LLONG_MIN, LLONG_MAX);
        auto pe = input::read_int("Enter non-negative integer exponent: ", 0, 60); 
        if (!pb.first || !pe.first) { cout << colors.col(colors.R) << "Bad input.\n" << colors.end(); logger.log("P5: bad input"); return; }
        long long base = pb.second;
        long long exp = pe.second;
        long long runningInt = 1;
        long double runningD = 1.0L;
        cout << colors.col(colors.G);
        for (long long i = 1; i <= exp; ++i) {
            long long beforeInt = runningInt;
            runningInt *= base;
            runningD *= base;
            cout << beforeInt << " x " << base << " = " << runningInt << "\n";
        }
        cout << base << "^" << exp << " = " << runningD << colors.end() << "\n";
        logger.log("P5: base=" + to_string(base) + " exp=" + to_string(exp) + " final=" + to_string((long long)runningD));
    }

    // Problem 6: Negative Exponent Support
    void problem6() {
        cout << colors.col(colors.Y) << "\nProblem 6 — Negative Exponent Support\n" << colors.end();
        auto pb = input::read_double("Enter base (can be fractional): ");
        auto pe = input::read_int("Enter integer exponent (can be negative): ", INT_MIN, INT_MAX);
        if (!pb.first || !pe.first) { cout << colors.col(colors.R) << "Bad input.\n" << colors.end(); logger.log("P6: bad input"); return; }
        double base = pb.second;
        long long exp = pe.second;
        long long absExp = llabs(exp);
        long double result = 1.0L;
        for (long long i = 0; i < absExp; ++i) result *= base;
        if (exp < 0) {
            if (result == 0.0L) {
                cout << colors.col(colors.R) << "Divide by zero situation! base^neg where base==0.\n" << colors.end();
                logger.log("P6: div by zero attempt base=0 exp=" + to_string(exp));
                return;
            }
            result = 1.0L / result;
        }
        cout << colors.col(colors.G) << base << "^" << exp << " = ";
        if (fabs((double)result) < 1e-9) cout << setprecision(12) << scientific << (double)result;
        else cout << fixed << setprecision(12) << (double)result;
        cout << colors.end() << "\n";
        logger.log("P6: base=" + to_string(base) + " exp=" + to_string(exp) + " result=" + to_string((double)result));
    }

    // Problem 7: Row-Based Numerical Triangle
    void problem7() {
        cout << colors.col(colors.Y) << "\nProblem 7 — Row-Based Numerical Triangle\n" << colors.end();
        auto p = input::read_int("Enter number of rows (1..50): ", 1, 50);
        if (!p.first) { cout << colors.col(colors.R) << "Bad input.\n" << colors.end(); logger.log("P7: bad input"); return; }
        int n = (int)p.second;
        cout << colors.col(colors.G);
        for (int r = 1; r <= n; ++r) {
            for (int c = 1; c <= r; ++c) {
                cout << r;
                if (c < r) cout << " ";
            }
            cout << "\n";
        }
        cout << colors.end();
        logger.log("P7: rows=" + to_string(n));
    }

    void toggle_colors() {
        colors.enabled = !colors.enabled;
        cout << (colors.enabled ? colors.col(colors.G) + "Colors ON\n" + colors.end() : "Colors OFF\n");
        logger.log(string("Toggle colors -> ") + (colors.enabled ? "ON" : "OFF"));
    }

    void toggle_logging() {
        logger.enabled = !logger.enabled;
        cout << colors.col(colors.M) << "Logging " << (logger.enabled ? "enabled" : "disabled") << colors.end() << "\n";
        logger.log(string("Toggle logging -> ") + (logger.enabled ? "ON" : "OFF"));
    }
};

int main(int argc, char** argv) {
    // Optionally allow disabling colors via CLI flag
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--no-colors" || arg == "--nocolors") colors.enabled = false;
        if (arg == "--no-log") logger.enabled = false;
    }

    cout << (colors.enabled ? "\033[36m" : "") 
         << "\nWelcome to the Loops dumbass\n"
         << (colors.enabled ? "\033[0m" : "") << endl;

    LoopsMaster app;
    app.run();

    return 0;
}
