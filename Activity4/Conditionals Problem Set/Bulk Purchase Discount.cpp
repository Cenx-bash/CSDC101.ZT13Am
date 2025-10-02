#include <iostream>
using namespace std;

int main() {
    int cups = 0;
    cout << "How many cups? ";
    cin >> cups;

    double total = cups * 1.0;  

    if (cups >= 10) {
        total *= 0.8; 
    } else if (cups >= 5) {
        total *= 0.9; 
    } else {
        total *= 1;   
    }

    cout << "Total cost: $" << total << endl;

    return 0;
}
