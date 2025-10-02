#include <iostream>
#include <string>
using namespace std;

int main() {
    int x = 0, y = 0;
    string move;
    cout << "Enter move (W/A/S/D): ";
    cin >> move;

    if (move == "W" || move == "w") {
        y += 1;
    } else if (move == "S" || move == "s") {
        y -= 1;
    } else if (move == "A" || move == "a") {
        x -= 1;
    } else if (move == "D" || move == "d") {
        x += 1;
    } else {
        cout << "Invalid move!" << endl;
    }

    cout << "The location of the player is ( " << x << ", " << y << " )" << endl;

    return 0;
}
