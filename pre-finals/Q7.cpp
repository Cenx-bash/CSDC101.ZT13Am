#include <iostream>
#include <fstream>
using namespace std;

int main() {
    ofstream out("students.txt");

    string name;
    float grade;

    for (int i = 0; i < 3; i++) {
        cin >> name >> grade;
        out << name << " " << grade << endl;
    }
    out.close();

    ifstream in("students.txt");

    while (in >> name >> grade) {
        cout << name << " " << grade << endl;
    }

    in.close();
}
