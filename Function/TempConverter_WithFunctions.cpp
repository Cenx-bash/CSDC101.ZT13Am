#include <iostream>
using namespace std;

// Function prototypes
float celsiusToFahrenheit(float celsius);
float fahrenheitToCelsius(float fahrenheit);

// Function to convert Celsius to Fahrenheit
float celsiusToFahrenheit(float celsius) {
    return (celsius * 9.0/5.0) + 32;
}

// Function to convert Fahrenheit to Celsius
float fahrenheitToCelsius(float fahrenheit) {
    return (fahrenheit - 32) * 5.0/9.0;
}

int main() {
    int userChoice;
    float inputTemp, convertedTemp;
    
    cout << "=== TEMPERATURE CONVERTER (With Functions) ===" << endl;
    cout << "Conversion Options:" << endl;
    cout << "1 - Celsius to Fahrenheit" << endl;
    cout << "2 - Fahrenheit to Celsius" << endl;
    cout << "Enter your choice (1 or 2): ";
    cin >> userChoice;
    
    if (userChoice == 1) {
        cout << "Enter temperature in Celsius: ";
        cin >> inputTemp;
        convertedTemp = celsiusToFahrenheit(inputTemp);
        cout << inputTemp << "°C = " << convertedTemp << "°F" << endl;
    }
    else if (userChoice == 2) {
        cout << "Enter temperature in Fahrenheit: ";
        cin >> inputTemp;
        convertedTemp = fahrenheitToCelsius(inputTemp);
        cout << inputTemp << "°F = " << convertedTemp << "°C" << endl;
    }
    else {
        cout << "Invalid choice! Please enter 1 or 2." << endl;
    }
    
    return 0;
}
