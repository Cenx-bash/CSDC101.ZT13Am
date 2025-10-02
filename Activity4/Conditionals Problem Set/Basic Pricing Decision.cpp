#include <iostream>
using namespace std;

int main() {
    double money = 0;
    cout << "Enter your money: ";
    cin >> money;

    if (money >= 1.0) {
        cout << "Enjoy your lemonade!" << endl;
    } else {
        cout << "Sorry, you need more money." << endl;
    }

    return 0;
}
