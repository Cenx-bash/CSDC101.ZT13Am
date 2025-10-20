#include <iostream>
#include <iomanip>
#include <cmath>
using namespace std;

// ======================= PROBLEM 1: ODD AND EVEN COUNTER =======================
void problem1() {
    int num, evenCount = 0, oddCount = 0;
    cout << "\n[ Problem 1: Odd and Even Counter ]\n";
    cout << "Enter 10 integers:\n";

    for (int i = 0; i < 10; i++) {
        cin >> num;
        if (num % 2 == 0)
            evenCount++;
        else
            oddCount++;
    }

    cout << "Even numbers: " << evenCount << endl;
    cout << "Odd numbers: " << oddCount << endl;
}

// ======================= PROBLEM 2: FACTORIAL CALCULATOR =======================
void problem2() {
    int n;
    long long factorial = 1;
    cout << "\n[ Problem 2: Factorial Calculator ]\n";
    cout << "Enter a number: ";
    cin >> n;

    for (int i = 1; i <= n; i++) {
        factorial *= i;
    }

    cout << "Factorial = " << factorial << endl;
}

// ======================= PROBLEM 3: SUM OF NATURAL NUMBERS =======================
void problem3() {
    int n, sum = 0;
    cout << "\n[ Problem 3: Sum of Natural Numbers ]\n";
    cout << "Enter a positive integer: ";
    cin >> n;

    for (int i = 1; i <= n; i++) {
        sum += i;
    }

    cout << "Sum = " << sum << endl;
}

// ======================= PROBLEM 4: BASIC EXPONENT CALCULATOR =======================
void problem4() {
    double base;
    int exp;
    double result = 1;

    cout << "\n[ Problem 4: Basic Exponent Calculator ]\n";
    cout << "Enter base: ";
    cin >> base;
    cout << "Enter exponent: ";
    cin >> exp;

    for (int i = 0; i < exp; i++) {
        result *= base;
    }

    cout << base << "^" << exp << " = " << result << endl;
}

// ======================= PROBLEM 5: DISPLAY EXPONENT STEPS =======================
void problem5() {
    double base;
    int exp;
    double result = 1;

    cout << "\n[ Problem 5: Display Exponent Steps ]\n";
    cout << "Enter base: ";
    cin >> base;
    cout << "Enter exponent: ";
    cin >> exp;

    for (int i = 0; i < exp; i++) {
        double prev = result;
        result *= base;
        cout << prev << " x " << base << " = " << result << endl;
    }

    cout << base << "^" << exp << " = " << result << endl;
}

// ======================= PROBLEM 6: NEGATIVE EXPONENT SUPPORT =======================
void problem6() {
    double base;
    int exp;
    double result = 1;

    cout << "\n[ Problem 6: Negative Exponent Support ]\n";
    cout << "Enter base: ";
    cin >> base;
    cout << "Enter exponent: ";
    cin >> exp;

    int limit = abs(exp);
    for (int i = 0; i < limit; i++) {
        result *= base;
    }

    if (exp < 0)
        result = 1.0 / result;

    cout << base << "^" << exp << " = " << result << endl;
}

// ======================= PROBLEM 7: ROW-BASED TRIANGLE =======================
void problem7() {
    int rows;
    cout << "\n[ Problem 7: Row-Based Numerical Triangle ]\n";
    cout << "Enter number of rows: ";
    cin >> rows;

    for (int i = 1; i <= rows; i++) {
        for (int j = 1; j <= i; j++) {
            cout << i << " ";
        }
        cout << endl;
    }
}

// ================================ MAIN MENU ================================
int main() {
    int choice;
    do {
        cout << "\n================ LOOP PRACTICE PROGRAM ================\n";
        cout << "1. Odd and Even Counter\n";
        cout << "2. Factorial Calculator\n";
        cout << "3. Sum of Natural Numbers\n";
        cout << "4. Basic Exponent Calculator\n";
        cout << "5. Display Exponent Steps\n";
        cout << "6. Negative Exponent Support\n";
        cout << "7. Row-Based Numerical Triangle\n";
        cout << "0. Exit\n";
        cout << "Select a problem: ";
        cin >> choice;

        switch (choice) {
            case 1: problem1(); break;
            case 2: problem2(); break;
            case 3: problem3(); break;
            case 4: problem4(); break;
            case 5: problem5(); break;
            case 6: problem6(); break;
            case 7: problem7(); break;
            case 0: cout << "\nProgram terminated.\n"; break;
            default: cout << "Invalid choice.\n";
        }
    } while (choice != 0);

    return 0;
}
