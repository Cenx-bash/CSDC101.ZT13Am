/*
 - Sequences (Fibonacci iterative & recursive)
 - Golden Ratio
 - Factorial
 - Prime checks and prime listing
 - GCD / LCM
 - Calculator
 - Digit operations (sum, reverse, palindrome)
 - Pascal's Triangle
 - Many star shapes (10+)
 - Array utilities (input, print, max/min, sort, avg)
 - Structs (Student example)
 - Clean menu driven program, cross-platform clear
 - Functions for each feature with comments
*/

#include <bits/stdc++.h>
#include <iostream>
using namespace std;

// ---------------------- Utilities ----------------------
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pressEnterToContinue() {
    cout << "\nPress Enter to return to the menu...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

long long readPositiveLongLong(const string &prompt) {
    long long x;
    while (true) {
        cout << prompt;
        if (cin >> x && x >= 0) break;
        cout << "Please enter a non-negative integer.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    return x;
}

int readInt(const string &prompt) {
    int x;
    while (true) {
        cout << prompt;
        if (cin >> x) break;
        cout << "Invalid input. Try again.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    return x;
}

// ---------------------- Fibonacci ----------------------
vector<long long> fibonacciIterative(int n) {
    vector<long long> seq;
    long long a = 0, b = 1;
    for (int i = 0; i < n; ++i) {
        seq.push_back(a);
        long long nxt = a + b;
        a = b; b = nxt;
    }
    return seq;
}

long long fibonacciRecursive(int n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    return fibonacciRecursive(n-1) + fibonacciRecursive(n-2);
}

bool isPerfectSquare(long long x) {
    if (x < 0) return false;
    long long s = (long long)floor(sqrt((long double)x));
    return s*s == x;
}

bool isFibonacciNumber(long long n) {
    // n is Fibonacci if and only if one of (5*n*n + 4) or (5*n*n - 4) is a perfect square
    long long a = 5*n*n + 4;
    long long b = 5*n*n - 4;
    return isPerfectSquare(a) || isPerfectSquare(b);
}

// Golden ratio approximation using fibonacci
long double goldenRatioApprox(int n) {
    if (n < 2) return 0.0L;
    long double a = 0.0L, b = 1.0L;
    for (int i = 1; i < n; ++i) {
        long double nxt = a + b;
        a = b; b = nxt;
    }
    return b / a;
}

// ---------------------- Math Helpers ----------------------
long long factorial(long long n) {
    if (n <= 1) return 1;
    long long res = 1;
    for (long long i = 2; i <= n; ++i) res *= i;
    return res;
}

bool isPrime(long long n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0) return false;
    for (long long i = 3; i*i <= n; i += 2) if (n % i == 0) return false;
    return true;
}

vector<int> listPrimesUpTo(int n) {
    vector<int> primes;
    if (n < 2) return primes;
    vector<bool> sieve(n+1, true);
    sieve[0] = sieve[1] = false;
    for (int p = 2; p*p <= n; ++p) if (sieve[p])
        for (int q = p*p; q <= n; q += p) sieve[q] = false;
    for (int i = 2; i <= n; ++i) if (sieve[i]) primes.push_back(i);
    return primes;
}

long long gcdLL(long long a, long long b) {
    while (b != 0) {
        long long t = b; b = a % b; a = t;
    }
    return llabs(a);
}

long long lcmLL(long long a, long long b) {
    if (a == 0 || b == 0) return 0;
    return llabs(a / gcdLL(a,b) * b);
}

// ---------------------- Digit Operations ----------------------
int sumOfDigits(long long n) {
    n = llabs(n);
    int s = 0;
    while (n) { s += n % 10; n /= 10; }
    return s;
}

long long reverseNumber(long long n) {
    bool neg = n < 0; n = llabs(n);
    long long rev = 0;
    while (n) { rev = rev*10 + (n%10); n /= 10; }
    return neg ? -rev : rev;
}

bool isPalindromeNumber(long long n) {
    return n == reverseNumber(n);
}

bool isPalindromeString(const string &s) {
    int i = 0, j = (int)s.size()-1;
    while (i < j) {
        if (s[i] != s[j]) return false;
        ++i; --j;
    }
    return true;
}

// ---------------------- Pascal's Triangle ----------------------
void printPascalsTriangle(int rows) {
    vector<long long> cur;
    for (int r = 0; r < rows; ++r) {
        cur.assign(r+1, 1);
        for (int i = 1; i < r; ++i) cur[i] = cur[i-1] * (r - i + 1) / i; // compute safely
        // spacing for triangle look
        int spaces = rows - r;
        cout << string(spaces, ' ');
        for (long long val : cur) cout << val << " ";
        cout << '\n';
    }
}

// ---------------------- Calculator ----------------------
void simpleCalculator() {
    double a, b; char op;
    cout << "Enter expression (e.g. 2.5 + 3): ";
    if (!(cin >> a >> op >> b)) { cout << "Invalid input.\n"; cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); return; }
    switch (op) {
        case '+': cout << (a + b) << '\n'; break;
        case '-': cout << (a - b) << '\n'; break;
        case '*': cout << (a * b) << '\n'; break;
        case '/': if (b == 0) cout << "Division by zero!\n"; else cout << (a / b) << '\n'; break;
        default: cout << "Unknown operator. Use + - * /.\n";
    }
}

// ---------------------- Shapes ----------------------
// Each shape prints using '*' and spaces. Many shapes accept 'size' parameter.

void shapeSquare(int n) {
    for (int i = 0; i < n; ++i) { for (int j = 0; j < n; ++j) cout << "* "; cout << '\n'; }
}

void shapeHollowSquare(int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i==0 || i==n-1 || j==0 || j==n-1) cout << "* "; else cout << "  ";
        }
        cout << '\n';
    }
}

void shapeRectangle(int width, int height) {
    for (int i = 0; i < height; ++i) { for (int j = 0; j < width; ++j) cout << "* "; cout << '\n'; }
}

void shapeRightTriangle(int n) {
    for (int i = 1; i <= n; ++i) { for (int j = 0; j < i; ++j) cout << "* "; cout << '\n'; }
}

void shapeInvertedRightTriangle(int n) {
    for (int i = n; i >= 1; --i) { for (int j = 0; j < i; ++j) cout << "* "; cout << '\n'; }
}

void shapePyramid(int n) {
    for (int i = 1; i <= n; ++i) {
        cout << string(n - i, ' ');
        for (int j = 0; j < 2*i - 1; ++j) cout << '*';
        cout << '\n';
    }
}

void shapeInvertedPyramid(int n) {
    for (int i = n; i >= 1; --i) {
        cout << string(n - i, ' ');
        for (int j = 0; j < 2*i - 1; ++j) cout << '*';
        cout << '\n';
    }
}

void shapeDiamond(int n) {
    // n is half-height; better with odd sizes
    shapePyramid(n);
    for (int i = n-1; i >= 1; --i) {
        cout << string(n - i, ' ');
        for (int j = 0; j < 2*i - 1; ++j) cout << '*';
        cout << '\n';
    }
}

void shapeX(int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) cout << ((j==i || j==n-i-1) ? '*' : ' ');
        cout << '\n';
    }
}

void shapePlus(int n) {
    int mid = n/2;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) cout << ((i==mid || j==mid) ? '*' : ' ');
        cout << '\n';
    }
}

void shapeHeart(int n) {
    // simple heart scaled by n; n controls width a bit
    for (int i = n/2; i <= n; i += 2) {
        for (int j = 1; j < n-i; j += 2) cout << ' ';
        for (int j = 1; j <= i; ++j) cout << '*';
        for (int j = 1; j <= n-i; ++j) cout << ' ';
        for (int j = 1; j <= i; ++j) cout << '*';
        cout << '\n';
    }
    for (int i = n; i >= 1; --i) {
        for (int j = i; j < n; ++j) cout << ' ';
        for (int j = 1; j <= (i*2)-1; ++j) cout << '*';
        cout << '\n';
    }
}

void shapeHourglass(int n) {
    for (int i = 0; i < n; ++i) {
        cout << string(i, ' ');
        for (int j = 0; j < 2*(n-i)-1; ++j) cout << '*';
        cout << '\n';
    }
    for (int i = 2; i <= n; ++i) {
        cout << string(n - i, ' ');
        for (int j = 0; j < 2*i - 1; ++j) cout << '*';
        cout << '\n';
    }
}

void shapeCheckerboard(int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) cout << (((i+j)%2==0) ? "* " : "  ");
        cout << '\n';
    }
}

// A master shape menu
void shapesMenu() {
    clearScreen();
    cout << "Shapes Menu - choose a pattern to print:\n";
    cout << "1 Square\n2 Hollow Square\n3 Rectangle\n4 Right Triangle\n5 Inverted Triangle\n6 Pyramid\n";
    cout << "7 Inverted Pyramid\n8 Diamond\n9 X Shape\n10 Plus Sign\n11 Heart\n12 Hourglass\n13 Checkerboard\n0 Return\n";
    int ch = readInt("Enter choice: ");
    if (ch == 0) return;
    int size;
    switch (ch) {
        case 1: size = readInt("Enter size: "); shapeSquare(size); break;
        case 2: size = readInt("Enter size: "); shapeHollowSquare(size); break;
        case 3: { int w = readInt("Width: "); int h = readInt("Height: "); shapeRectangle(w,h); break; }
        case 4: size = readInt("Enter size: "); shapeRightTriangle(size); break;
        case 5: size = readInt("Enter size: "); shapeInvertedRightTriangle(size); break;
        case 6: size = readInt("Enter size: "); shapePyramid(size); break;
        case 7: size = readInt("Enter size: "); shapeInvertedPyramid(size); break;
        case 8: size = readInt("Enter size: "); shapeDiamond(size); break;
        case 9: size = readInt("Enter odd size: "); shapeX(size); break;
        case 10: size = readInt("Enter odd size: "); shapePlus(size); break;
        case 11: size = readInt("Enter size (suggest 6-14): "); shapeHeart(size); break;
        case 12: size = readInt("Enter size: "); shapeHourglass(size); break;
        case 13: size = readInt("Enter size: "); shapeCheckerboard(size); break;
        default: cout << "Invalid choice."; break;
    }
    cout << '\n';
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    pressEnterToContinue();
}

// ---------------------- Array Utilities ----------------------
void arrayUtilitiesMenu() {
    clearScreen();
    cout << "Array Utilities - enter number of elements: ";
    int n; cin >> n; if (n <= 0) { cout << "Invalid size.\n"; pressEnterToContinue(); return; }
    vector<long long> a(n);
    cout << "Enter " << n << " integers:\n";
    for (int i = 0; i < n; ++i) cin >> a[i];

    bool running = true;
    while (running) {
        clearScreen();
        cout << "Array: "; for (auto v: a) cout << v << " "; cout << '\n';
        cout << "1 Max\n2 Min\n3 Sort Asc\n4 Sort Desc\n5 Average\n6 Show Range-for Example\n0 Return\n";
        int ch = readInt("Choice: ");
        switch (ch) {
            case 1: cout << "Max = " << *max_element(a.begin(), a.end()) << '\n'; break;
            case 2: cout << "Min = " << *min_element(a.begin(), a.end()) << '\n'; break;
            case 3: sort(a.begin(), a.end()); cout << "Sorted ascending.\n"; break;
            case 4: sort(a.rbegin(), a.rend()); cout << "Sorted descending.\n"; break;
            case 5: {
                long double s = 0; for (auto v: a) s += v; cout << "Average = " << (s / a.size()) << '\n'; break;
            }
            case 6: {
                cout << "Range-for iteration: "; for (long long v : a) cout << v << " "; cout << '\n'; break;
            }
            case 0: running = false; break;
            default: cout << "Invalid option.\n"; break;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        if (running) pressEnterToContinue();
    }
}

// ---------------------- Structures ----------------------
struct Student { string name; int age; double grade; };

void studentStructMenu() {
    clearScreen();
    int n = readInt("How many students to input? ");
    if (n <= 0) { cout << "Nothing to do.\n"; pressEnterToContinue(); return; }
    vector<Student> list(n);
    for (int i = 0; i < n; ++i) {
        cout << "Student #" << (i+1) << " name: ";
        cin >> ws; getline(cin, list[i].name);
        list[i].age = readInt("Age: ");
        cout << "Grade: "; cin >> list[i].grade;
    }
    clearScreen();
    cout << "Students Entered:\n";
    for (auto &s : list) cout << s.name << " | Age: " << s.age << " | Grade: " << s.grade << '\n';
    pressEnterToContinue();
}

// ---------------------- Main Menu Actions ----------------------
void menuFibonacci() {
    clearScreen();
    int n = readInt("How many Fibonacci numbers to display? ");
    if (n <= 0) { cout << "Nothing to show.\n"; pressEnterToContinue(); return; }
    auto seq = fibonacciIterative(n);
    cout << "Iterative: "; for (auto v: seq) cout << v << " "; cout << '\n';

    cout << "Recursive (first min(40,n) to avoid long recursion): ";
    int upto = min(n, 40);
    for (int i = 0; i < upto; ++i) cout << fibonacciRecursive(i) << " ";
    if (n > 40) cout << "... (truncated due to recursion cost)";
    cout << '\n';
    pressEnterToContinue();
}

void menuGoldenRatio() {
    clearScreen();
    int n = readInt("Use which Fibonacci index to approximate golden ratio (n>=2 recommended): ");
    if (n < 2) { cout << "Need n >= 2 for approximation.\n"; pressEnterToContinue(); return; }
    cout << "Approx Golden Ratio ~ " << fixed << setprecision(12) << (double)goldenRatioApprox(n) << "\n";
    pressEnterToContinue();
}

void menuFactorial() {
    clearScreen();
    long long n = readPositiveLongLong("Enter non-negative integer: ");
    cout << n << "! = " << factorial(n) << "\n";
    pressEnterToContinue();
}

void menuPrimes() {
    clearScreen();
    cout << "1 Check prime\n2 List primes up to N\n0 Return\n";
    int ch = readInt("Choice: ");
    if (ch == 1) {
        long long x = readPositiveLongLong("Enter number to check: ");
        cout << x << (isPrime(x) ? " is prime.\n" : " is not prime.\n");
    } else if (ch == 2) {
        int n = readInt("List primes up to: ");
        auto primes = listPrimesUpTo(n);
        cout << "Primes up to " << n << ":\n";
        for (int p: primes) cout << p << " "; cout << '\n';
    }
    pressEnterToContinue();
}

void menuGcdLcm() {
    clearScreen();
    long long a = readPositiveLongLong("Enter first integer: ");
    long long b = readPositiveLongLong("Enter second integer: ");
    cout << "GCD = " << gcdLL(a,b) << "\n";
    cout << "LCM = " << lcmLL(a,b) << "\n";
    pressEnterToContinue();
}

void menuCalculator() {
    clearScreen();
    simpleCalculator();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    pressEnterToContinue();
}

void menuDigits() {
    clearScreen();
    cout << "1 Sum digits\n2 Reverse number\n3 Palindrome check\n0 Return\n";
    int ch = readInt("Choice: ");
    if (ch == 1) {
        long long x = readPositiveLongLong("Enter number: "); cout << "Sum = " << sumOfDigits(x) << '\n';
    } else if (ch == 2) {
        long long x = readPositiveLongLong("Enter number: "); cout << "Reverse = " << reverseNumber(x) << '\n';
    } else if (ch == 3) {
        cout << "Check numeric palindrome or string? (1 numeric / 2 string): "; int t; cin >> t;
        if (t == 1) { long long x = readPositiveLongLong("Enter number: "); cout << (isPalindromeNumber(x) ? "Palindrome\n" : "Not palindrome\n"); }
        else { cout << "Enter string (single line): "; cin >> ws; string s; getline(cin, s); cout << (isPalindromeString(s) ? "Palindrome\n" : "Not palindrome\n"); }
    }
    pressEnterToContinue();
}

void menuPascals() {
    clearScreen();
    int r = readInt("Rows to print: ");
    if (r <= 0) { cout << "Nothing to show.\n"; pressEnterToContinue(); return; }
    printPascalsTriangle(r);
    pressEnterToContinue();
}

void menuShapes() { shapesMenu(); }
void menuArrays() { arrayUtilitiesMenu(); }
void menuStructs() { studentStructMenu(); }

void menuFiboCheck() {
    clearScreen();
    long long x = readPositiveLongLong("Enter number to check if it's Fibonacci: ");
    cout << x << (isFibonacciNumber(x) ? " is a Fibonacci number.\n" : " is NOT a Fibonacci number.\n");
    pressEnterToContinue();
}

// ---------------------- Main Program ----------------------
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    bool running = true;
    while (running) {
        clearScreen();
        cout << "==== C++ ====" << '\n';
        cout << "1 Fibonacci (iter & recursive)\n2 Fibonacci (recursive only)\n3 Check if Fibonacci\n4 Golden Ratio\n";
        cout << "5 Factorial\n6 Prime checks / list\n7 GCD & LCM\n8 Calculator\n9 Digit ops (sum/reverse/palindrome)\n";
        cout << "10 Pascal's Triangle\n11 Shapes (many)\n12 Array utilities\n13 Student struct demo\n0 Exit\n";
        int choice = readInt("Enter choice: ");
        switch (choice) {
            case 1: menuFibonacci(); break;
            case 2: { clearScreen(); int n = readInt("Recursive Fibonacci: compute F(n) for n = "); cout << "F("<<n<<") = "<<fibonacciRecursive(n)<<"\n"; pressEnterToContinue(); break; }
            case 3: menuFiboCheck(); break;
            case 4: menuGoldenRatio(); break;
            case 5: menuFactorial(); break;
            case 6: menuPrimes(); break;
            case 7: menuGcdLcm(); break;
            case 8: menuCalculator(); break;
            case 9: menuDigits(); break;
            case 10: menuPascals(); break;
            case 11: menuShapes(); break;
            case 12: menuArrays(); break;
            case 13: menuStructs(); break;
            case 0: running = false; break;
            default: cout << "Invalid option."; pressEnterToContinue(); break;
        }
    }

    clearScreen();
    cout << "Thank you — program ended.\n";
    return 0;
}
