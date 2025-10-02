#include <iostream>
using namespace std;

int main() {
    int temp = 0;
    cout << "Enter today's temperature in Celsius: ";
    cin >> temp;

    if (temp >= 30) {
        cout << "It's hot! Lemonade costs $0.80 today." << endl;
    } else {
        cout << "Lemonade costs $1.00 today." << endl;
    }

    return 0;
}
