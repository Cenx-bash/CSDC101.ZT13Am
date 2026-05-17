#include <iostream>
#include <fstream>
using namespace std;

// ---------------- VARIABLES ----------------
void demoVariables() {
    int age = 18;
    double grade = 91.5;
    char letter = 'A';
    string name = "Zenn";
    bool isPassed = true;

    cout << "\n[Variables]\n";
    cout << name << " " << age << " " << grade << " " << letter << " " << isPassed << endl;
}

// ---------------- CONDITION ----------------
void demoCondition() {
    cout << "\n[Condition]\n";
    int score = 75;

    if (score >= 75)
        cout << "Passed\n";
    else
        cout << "Failed\n";
}

// ---------------- LOOPS ----------------
void demoLoops() {
    cout << "\n[Loops]\nFor Loop: ";
    for (int i = 1; i <= 5; i++)
        cout << i << " ";

    cout << "\nWhile Loop: ";
    int i = 1;
    while (i <= 5) {
        cout << i << " ";
        i++;
    }
    cout << endl;
}

// ---------------- FUNCTIONS ----------------
int add(int a, int b) {
    return a + b;
}

void demoFunctions() {
    cout << "\n[Functions]\n";
    cout << "5 + 3 = " << add(5, 3) << endl;
}

// ---------------- ARRAYS ----------------
void demoArrays() {
    cout << "\n[Arrays]\n";
    int numbers[] = {10, 20, 30, 40};

    for (int i = 0; i < 4; i++)
        cout << numbers[i] << " ";
    cout << endl;
}

// ---------------- STRINGS ----------------
void demoString() {
    cout << "\n[Strings]\n";
    string text = "Hello World";

    cout << text << endl;
    cout << "Length: " << text.length() << endl;
    cout << "Substring: " << text.substr(0, 5) << endl;
}

// ---------------- OOP ----------------
class Car {
public:
    string brand = "Toyota";

    void drive() {
        cout << "Car is driving\n";
    }
};

void demoClass() {
    cout << "\n[Class & Object]\n";
    Car c;
    cout << c.brand << endl;
    c.drive();
}

// ---------------- INHERITANCE & POLYMORPHISM ----------------
class Animal {
public:
    virtual void sound() {
        cout << "Animal sound\n";
    }
};

class Dog : public Animal {
public:
    void sound() override {
        cout << "Bark\n";
    }
};

void demoPolymorphism() {
    cout << "\n[Polymorphism]\n";
    Animal* a = new Dog();
    a->sound();
    delete a;
}

// ---------------- SEARCH ----------------
void demoSearch() {
    cout << "\n[Linear Search]\n";
    int arr[] = {5, 10, 15, 20};
    int target = 15;
    int found = -1;

    for (int i = 0; i < 4; i++) {
        if (arr[i] == target) {
            found = i;
            break;
        }
    }

    cout << "Index: " << found << endl;
}

// ---------------- SORT ----------------
void demoSort() {
    cout << "\n[Bubble Sort]\n";
    int arr[] = {5, 3, 8, 1};
    int n = 4;

    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                swap(arr[j], arr[j + 1]);
            }
        }
    }

    for (int i = 0; i < n; i++)
        cout << arr[i] << " ";
    cout << endl;
}

// ---------------- FILE HANDLING ----------------
void demoFile() {
    cout << "\n[File Handling]\n";
    ofstream file("data.txt");
    file << "Hello File Handling in C++";
    file.close();

    cout << "File written successfully.\n";
}

// ---------------- EXCEPTION HANDLING ----------------
void demoException() {
    cout << "\n[Exception Handling]\n";

    try {
        int x = 10, y = 0;
        if (y == 0)
            throw "Division by zero error";

        cout << x / y << endl;
    }
    catch (const char* msg) {
        cout << "Error: " << msg << endl;
    }
}

// ---------------- MAIN ----------------
int main() {
    demoVariables();
    demoCondition();
    demoLoops();
    demoFunctions();
    demoArrays();
    demoString();
    demoClass();
    demoPolymorphism();
    demoSearch();
    demoSort();
    demoFile();
    demoException();

    return 0;
}
