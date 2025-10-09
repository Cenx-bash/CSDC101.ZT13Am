#include <iostream>
using namespace std;

int main() {
    int choice;
    cout << "Choose a program to run:\n";
    cout << "1 - For Loop\n";
    cout << "2 - While Loop\n";
    cout << "3 - Do-While Loop\n";
    cout << "4 - Fibonacci Sequence\n";
    cout << "5 - Right Triangle\n";
    cout << "6 - Inverted Right Triangle\n";
    cout << "7 - Right-Aligned Triangle\n";
    cout << "8 - Pyramid Triangle\n";
    cout << "9 - Inverted Pyramid\n";
    cout << "10 - Diamond Shape\n";
    cout << "11 - Heart Shape\n";
    cout << "12 - Factorial\n";
    cout << "13 - Star Shape\n";
    cout << "Enter your choice: ";
    cin >> choice;

    switch (choice) {

        // 1️⃣ For Loop
        case 1: {
            for (int i = 1; i <= 5; i++)
                cout << i << " ";
            break;
        }

        // 2️⃣ While Loop
        case 2: {
            int i = 1;
            while (i <= 5) {
                cout << i << " ";
                i++;
            }
            break;
        }

        // 3️⃣ Do-While Loop
        case 3: {
            int i = 1;
            do {
                cout << i << " ";
                i++;
            } while (i <= 5);
            break;
        }

        // 4️⃣ Fibonacci
        case 4: {
            int n = 10, a = 0, b = 1;
            cout << a << " " << b << " ";
            for (int i = 2; i < n; i++) {
                int c = a + b;
                cout << c << " ";
                a = b;
                b = c;
            }
            break;
        }

        // 5️⃣ Right Triangle
        case 5: {
            for (int i = 1; i <= 5; i++) {
                for (int j = 1; j <= i; j++)
                    cout << "*";
                cout << endl;
            }
            break;
        }

        // 6️⃣ Inverted Right Triangle
        case 6: {
            for (int i = 5; i >= 1; i--) {
                for (int j = 1; j <= i; j++)
                    cout << "*";
                cout << endl;
            }
            break;
        }

        // 7️⃣ Right-Aligned Triangle
        case 7: {
            int n = 5;
            for (int i = 1; i <= n; i++) {
                for (int j = i; j < n; j++)
                    cout << " ";
                for (int k = 1; k <= i; k++)
                    cout << "*";
                cout << endl;
            }
            break;
        }

        // 8️⃣ Pyramid Triangle
        case 8: {
            int n = 5;
            for (int i = 1; i <= n; i++) {
                for (int j = i; j < n; j++)
                    cout << " ";
                for (int k = 1; k <= (2 * i - 1); k++)
                    cout << "*";
                cout << endl;
            }
            break;
        }

        // 9️⃣ Inverted Pyramid
        case 9: {
            int n = 5;
            for (int i = n; i >= 1; i--) {
                for (int j = i; j < n; j++)
                    cout << " ";
                for (int k = 1; k <= (2 * i - 1); k++)
                    cout << "*";
                cout << endl;
            }
            break;
        }

        // 🔟 Diamond Shape
        case 10: {
            int n = 5;
            // Upper
            for (int i = 1; i <= n; i++) {
                for (int j = i; j < n; j++)
                    cout << " ";
                for (int k = 1; k <= (2 * i - 1); k++)
                    cout << "*";
                cout << endl;
            }
            // Lower
            for (int i = n - 1; i >= 1; i--) {
                for (int j = i; j < n; j++)
                    cout << " ";
                for (int k = 1; k <= (2 * i - 1); k++)
                    cout << "*";
                cout << endl;
            }
            break;
        }

        // ❤️ Heart Shape
        case 11: {
            int n = 6;
            for (int i = n / 2; i <= n; i += 2) {
                for (int j = 1; j < n - i; j += 2)
                    cout << " ";
                for (int j = 1; j <= i; j++)
                    cout << "*";
                for (int j = 1; j <= n - i; j++)
                    cout << " ";
                for (int j = 1; j <= i; j++)
                    cout << "*";
                cout << endl;
            }
            for (int i = n; i >= 1; i--) {
                for (int j = i; j < n; j++)
                    cout << " ";
                for (int j = 1; j <= (i * 2) - 1; j++)
                    cout << "*";
                cout << endl;
            }
            break;
        }

        // 🧮 Factorial
        case 12: {
            int num, fact = 1;
            cout << "Enter a number: ";
            cin >> num;
            for (int i = 1; i <= num; i++)
                fact *= i;
            cout << "Factorial of " << num << " is " << fact;
            break;
        }

        // 🌟 Star Shape
        case 13: {
            int n = 7;
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    if (
                        i == n / 2 || j == n / 2 ||
                        i == j || i + j == n - 1 ||
                        (i == 1 && (j == 2 || j == 4)) ||
                        (i == n - 2 && (j == 2 || j == 4))
                    )
                        cout << "*";
                    else
                        cout << " ";
                }
                cout << endl;
            }
            break;
        }

        default:
            cout << "Invalid choice! Pick a number between 1–13.\n";
            break;
    }

    return 0;
}
