#include <iostream>
using namespace std;

int main() {
    int choice;
    float temperature, result;
    
    cout << "=== TEMPERATURE CONVERTER (No Functions) ===" << endl;
    cout << "1 - Celsius to Fahrenheit" << endl;
    cout << "2 - Fahrenheit to Celsius" << endl;
    cout << "Enter your choice (1 or 2): ";
    cin >> choice;
    
    if (choice == 1) {
        // Celsius to Fahrenheit conversion
        cout << "Enter temperature in Celsius: ";
        cin >> temperature;
        result = (temperature * 9.0/5.0) + 32;
        cout << temperature << "°C = " << result << "°F" << endl;
    }
    else if (choice == 2) {
        // Fahrenheit to Celsius conversion
        cout << "Enter temperature in Fahrenheit: ";
        cin >> temperature;
        result = (temperature - 32) * 5.0/9.0;
        cout << temperature << "°F = " << result << "°C" << endl;
    }
    else {
        cout << "Invalid choice! Please enter 1 or 2." << endl;
    }
    
    return 0;
}
