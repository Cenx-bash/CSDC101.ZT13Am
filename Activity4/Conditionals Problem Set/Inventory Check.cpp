#include <iostream>
using namespace std;

int main() {
    int lemons = 0, sugar = 0;
    cout << "Lemons: ";
    cin >> lemons;
    cout << "Sugar: ";
    cin >> sugar;

    if (lemons <= 0 || sugar <= 0) {
        cout << "You can't make lemonade!" << endl;
    } else {
        cout << "You're ready to sell lemonade!" << endl;
    }

    return 0;
}
