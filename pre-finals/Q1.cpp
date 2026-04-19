#include <iostream>
#include <iomanip>
#include <string>
using namespace std;

struct Student {
    string name;
    float midterm, final_exam, average;
};

int main() {
    const int N = 5;
    Student s[N];

    for (int i = 0; i < N; i++) {
        cout << "Student " << i + 1 << ":\n";
        cin.ignore();
        getline(cin, s[i].name);
        cin >> s[i].midterm >> s[i].final_exam;
        s[i].average = (s[i].midterm + s[i].final_exam) / 2;
    }

    cout << "\nName\tAverage\tRemark\n";
    for (int i = 0; i < N; i++) {
        cout << s[i].name << "\t" << s[i].average
             << "\t" << (s[i].average >= 75 ? "Passed" : "Failed") << endl;
    }
}
