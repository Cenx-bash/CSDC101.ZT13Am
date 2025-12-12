// PassMethodDemo.cpp
// Demonstrates the difference between pass by value and pass by reference
#include <iostream>
using namespace std;

// Function using pass by value
// Parameter is copied, modifications won't affect original variable
void passByValue(int num) {
    cout << "Inside passByValue(): Received value = " << num << endl;
    num = num * 3;  // Cube the value (n * n * n = n^3)
    cout << "Inside passByValue(): After modification = " << num << endl;
}

// Function using pass by reference
// Parameter is reference to original variable, modifications affect it
void passByReference(int &num) {
    cout << "Inside passByReference(): Received value = " << num << endl;
    num = num * 3;  // Cube the value (n * n * n = n^3)
    cout << "Inside passByReference(): After modification = " << num << endl;
}

int main() {
    // Declare two integer variables with the same initial value
    int value1 = 5;
    int value2 = 5;
    
    cout << "=== PASS BY VALUE DEMONSTRATION ===" << endl;
    cout << "Before passByValue(): value1 = " << value1 << endl;
    passByValue(value1);
    cout << "After passByValue(): value1 = " << value1 << endl;
    
    cout << "\n=== PASS BY REFERENCE DEMONSTRATION ===" << endl;
    cout << "Before passByReference(): value2 = " << value2 << endl;
    passByReference(value2);
    cout << "After passByReference(): value2 = " << value2 << endl;
    
    cout << "\n=== SUMMARY ===" << endl;
    cout << "Original value1 remains: " << value1 << " (pass by value)" << endl;
    cout << "Original value2 changed: " << value2 << " (pass by reference)" << endl;
    
    return 0;
}
