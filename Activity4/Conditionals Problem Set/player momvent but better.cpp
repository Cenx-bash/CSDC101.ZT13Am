#include <iostream>
#include <string>
using namespace std;

struct Player {
    int x;
    int y;
};

void movePlayer(Player &p, char command) {
    switch(command) {
        case 'W': case 'w': p.y += 1; break;              // Up
        case 'S': case 's': p.y -= 1; break;              // Down
        case 'A': case 'a': p.x -= 1; break;              // Left
        case 'D': case 'd': p.x += 1; break;              // Right
        case 'Q': case 'q': p.x -= 1; p.y += 1; break;    // Up-Left (NW)
        case 'E': case 'e': p.x += 1; p.y += 1; break;    // Up-Right (NE)
        case 'Z': case 'z': p.x -= 1; p.y -= 1; break;    // Down-Left (SW)
        case 'C': case 'c': p.x += 1; p.y -= 1; break;    // Down-Right (SE)
        default:
            cout << "Invalid move command!" << endl;
    }
}

int main() {
    Player player = {0, 0};  // starting at (0,0)
    char input;

    cout << "Player Movement Program" << endl;
    cout << "Controls:" << endl;
    cout << " W - Up, S - Down, A - Left, D - Right" << endl;
    cout << " Q - NW, E - NE, Z - SW, C - SE" << endl;

    cout << "\nEnter your move: ";
    cin >> input;

    movePlayer(player, input);

    cout << "The location of the player is ( " 
         << player.x << ", " << player.y << " )" << endl;

    return 0;
}
