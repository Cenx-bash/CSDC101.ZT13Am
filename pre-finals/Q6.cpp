#include <iostream>
#include <string>
using namespace std;

int main() {
    int n;
    cin >> n;

    string* names = new string[n];
    cin.ignore();

    for (int i = 0; i < n; i++) {
        getline(cin, names[i]);
    }

    for (int i = 0; i < n; i++) {
        cout << names[i] << endl;
    }

    delete[] names;
}
