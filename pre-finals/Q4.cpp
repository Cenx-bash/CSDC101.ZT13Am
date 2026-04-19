#include <iostream>
using namespace std;

class Rectangle {
    double w, h;

public:
    Rectangle(double a, double b) {
        w = a;
        h = b;
    }

    double area() { return w * h; }
    double perimeter() { return 2 * (w + h); }
    bool isSquare() { return w == h; }
};

int main() {
    double w, h;

    for (int i = 0; i < 3; i++) {
        cin >> w >> h;

        Rectangle r(w, h);

        cout << "Area: " << r.area() << endl;
        cout << "Perimeter: " << r.perimeter() << endl;
        cout << (r.isSquare() ? "Square" : "Not Square") << endl;
    }
}
