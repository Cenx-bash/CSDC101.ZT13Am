#include <iostream>
#include <string>
using namespace std;

struct Product {
    string name;
    int quantity;
    float price;
};

int main() {
    Product p[5];
    int n;

    cout << "Number of products: ";
    cin >> n;

    for (int i = 0; i < n; i++) {
        cin.ignore();
        getline(cin, p[i].name);
        cin >> p[i].quantity >> p[i].price;
    }

    float maxPrice = -1;
    string expensive;

    for (int i = 0; i < n; i++) {
        if (p[i].price > maxPrice) {
            maxPrice = p[i].price;
            expensive = p[i].name;
        }
    }

    cout << "Most expensive: " << expensive << endl;
}
