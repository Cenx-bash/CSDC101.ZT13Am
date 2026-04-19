#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

int main() {
    vector<pair<string, int>> scores;

    string name;
    int score;

    ifstream in("scores.txt");
    while (in >> name >> score) {
        scores.push_back({name, score});
    }
    in.close();

    cin >> name >> score;
    scores.push_back({name, score});

    ofstream out("scores.txt");
    for (auto s : scores) {
        out << s.first << " " << s.second << endl;
    }
    out.close();

    int maxScore = -1;
    string best;

    for (auto s : scores) {
        if (s.second > maxScore) {
            maxScore = s.second;
            best = s.first;
        }
    }

    cout << "Top: " << best << " " << maxScore << endl;
}
