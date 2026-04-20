// ======================================================================
// Section I | All About Structures
// ======================================================================

// Question 1 | Student Grade Report
#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace std;

// Step 1: Define struct Student
struct Student {
    string name;
    float midterm;
    float final_exam;
    float average;
};

void runQuestion1() {
    const int NUM_STUDENTS = 5;
    Student students[NUM_STUDENTS];

    // Step 2: Input student data
    for (int i = 0; i < NUM_STUDENTS; i++) {
        cout << "Enter details for student " << i + 1 << ":\n";
        cout << "Name: ";
        cin.ignore();
        getline(cin, students[i].name);
        cout << "Midterm score: ";
        cin >> students[i].midterm;
        cout << "Final exam score: ";
        cin >> students[i].final_exam;

        // Step 3: Compute average
        students[i].average = (students[i].midterm + students[i].final_exam) / 2.0f;
        cout << endl;
    }

    // Step 4: Display formatted table
    cout << "\n" << string(70, '-') << endl;
    cout << left << setw(20) << "Name"
         << setw(10) << "Midterm"
         << setw(10) << "Final"
         << setw(10) << "Average"
         << setw(10) << "Remark" << endl;
    cout << string(70, '-') << endl;

    for (int i = 0; i < NUM_STUDENTS; i++) {
        cout << left << setw(20) << students[i].name
             << setw(10) << fixed << setprecision(1) << students[i].midterm
             << setw(10) << students[i].final_exam
             << setw(10) << students[i].average
             << setw(10) << (students[i].average >= 75.0f ? "Passed" : "Failed") << endl;
    }
    cout << string(70, '-') << endl;
}

// Question 2 | Product Inventory
struct Product {
    string name;
    int quantity;
    float price;
};

void runQuestion2() {
    const int MAX_PRODUCTS = 5;
    Product products[MAX_PRODUCTS];
    int productCount;

    // Step 2: Ask user how many products
    cout << "How many products to enter (max 5): ";
    cin >> productCount;
    if (productCount > MAX_PRODUCTS) productCount = MAX_PRODUCTS;

    // Input product details
    for (int i = 0; i < productCount; i++) {
        cout << "\nProduct " << i + 1 << ":\n";
        cout << "Name: ";
        cin.ignore();
        getline(cin, products[i].name);
        cout << "Quantity: ";
        cin >> products[i].quantity;
        cout << "Price: ";
        cin >> products[i].price;
    }

    // Step 3: Display all products with total value
    cout << "\n" << string(65, '-') << endl;
    cout << left << setw(20) << "Product Name"
         << setw(10) << "Quantity"
         << setw(10) << "Price"
         << setw(15) << "Total Value" << endl;
    cout << string(65, '-') << endl;

    float highestPrice = -1;
    string mostExpensiveProduct;

    for (int i = 0; i < productCount; i++) {
        float totalValue = products[i].quantity * products[i].price;
        cout << left << setw(20) << products[i].name
             << setw(10) << products[i].quantity
             << setw(10) << fixed << setprecision(2) << products[i].price
             << setw(15) << totalValue << endl;

        // Step 4: Find most expensive product
        if (products[i].price > highestPrice) {
            highestPrice = products[i].price;
            mostExpensiveProduct = products[i].name;
        }
    }
    cout << string(65, '-') << endl;
    cout << "\nMost expensive product: " << mostExpensiveProduct
         << " with price $" << fixed << setprecision(2) << highestPrice << endl;
}

// ======================================================================
// Section II | All About Classes
// ======================================================================

// Question 3 | Bank Account System
class BankAccount {
private:
    string accountHolder;
    double balance;

public:
    // Step 2: Constructor
    BankAccount(string holder, double initialDeposit) {
        accountHolder = holder;
        balance = initialDeposit;
    }

    // Step 3: Public methods
    void deposit(double amount) {
        if (amount > 0) {
            balance += amount;
            cout << "Deposited $" << amount << ". New balance: $" << balance << endl;
        } else {
            cout << "Invalid deposit amount!" << endl;
        }
    }

    void withdraw(double amount) {
        if (amount > balance) {
            cout << "Error: Insufficient funds! Current balance: $" << balance << endl;
        } else if (amount > 0) {
            balance -= amount;
            cout << "Withdrew $" << amount << ". New balance: $" << balance << endl;
        } else {
            cout << "Invalid withdrawal amount!" << endl;
        }
    }

    void displayBalance() {
        cout << "Account holder: " << accountHolder << ", Balance: $" << balance << endl;
    }
};

void runQuestion3() {
    string name;
    double initialDeposit;

    cout << "Enter account holder name: ";
    cin.ignore();
    getline(cin, name);
    cout << "Enter initial deposit amount: $";
    cin >> initialDeposit;

    BankAccount account(name, initialDeposit);

    int choice;
    double amount;

    do {
        // Step 4: Menu loop
        cout << "\n===== MENU =====" << endl;
        cout << "[1] Deposit" << endl;
        cout << "[2] Withdraw" << endl;
        cout << "[3] Check Balance" << endl;
        cout << "[4] Exit" << endl;
        cout << "Enter choice: ";
        cin >> choice;

        switch (choice) {
            case 1:
                cout << "Enter amount to deposit: $";
                cin >> amount;
                account.deposit(amount);
                break;
            case 2:
                cout << "Enter amount to withdraw: $";
                cin >> amount;
                account.withdraw(amount);
                break;
            case 3:
                account.displayBalance();
                break;
            case 4:
                cout << "Thank you for using our banking system!" << endl;
                break;
            default:
                cout << "Invalid choice! Please try again." << endl;
        }
    } while (choice != 4);
}

// Question 4 | Shape Calculator
class Rectangle {
private:
    double width;
    double height;

public:
    // Step 2: Constructor and methods
    Rectangle(double w, double h) {
        width = w;
        height = h;
    }

    double getArea() {
        return width * height;
    }

    double getPerimeter() {
        return 2 * (width + height);
    }

    bool isSquare() {
        return width == height;
    }
};

void runQuestion4() {
    double w, h;

    // Step 3: Create 3 rectangles
    for (int i = 0; i < 3; i++) {
        cout << "\nRectangle " << i + 1 << ":\n";
        cout << "Enter width: ";
        cin >> w;
        cout << "Enter height: ";
        cin >> h;

        Rectangle rect(w, h);

        cout << fixed << setprecision(2);
        cout << "Area: " << rect.getArea() << endl;
        cout << "Perimeter: " << rect.getPerimeter() << endl;
        cout << "Is square? " << (rect.isSquare() ? "Yes" : "No") << endl;
    }
}

// ======================================================================
// Section III | All About Pointers
// ======================================================================

// Question 5 | Pointer-Based Sorting
void bubbleSort(int* arr, int size) {
    // Using pointer arithmetic only - no bracket indexing
    for (int* i = arr; i < arr + size - 1; i++) {
        for (int* j = arr; j < arr + size - (i - arr) - 1; j++) {
            if (*j > *(j + 1)) {
                // Swap using pointers
                int temp = *j;
                *j = *(j + 1);
                *(j + 1) = temp;
            }
        }
    }
}

void runQuestion5() {
    const int SIZE = 5;
    int numbers[SIZE];

    // Step 1: Input 5 integers
    cout << "Enter 5 integers:\n";
    for (int i = 0; i < SIZE; i++) {
        cout << "Number " << i + 1 << ": ";
        cin >> numbers[i];
    }

    // Display original array
    cout << "Before: ";
    for (int i = 0; i < SIZE; i++) {
        cout << numbers[i] << " ";
    }
    cout << endl;

    // Step 2: Sort using pointer function
    bubbleSort(numbers, SIZE);

    // Step 3: Display sorted array
    cout << "After:  ";
    for (int i = 0; i < SIZE; i++) {
        cout << numbers[i] << " ";
    }
    cout << endl;
}

// Question 6 | Dynamic Student List
void runQuestion6() {
    int numStudents;

    // Step 1: Ask how many students
    cout << "How many students? ";
    cin >> numStudents;

    // Step 2: Dynamically allocate array of strings
    string* studentNames = new string[numStudents];

    // Step 3: Enter and display names
    cin.ignore();
    for (int i = 0; i < numStudents; i++) {
        cout << "Enter name of student " << i + 1 << ": ";
        getline(cin, studentNames[i]);
    }

    cout << "\nStudent list (in order entered):\n";
    for (int i = 0; i < numStudents; i++) {
        cout << i + 1 << ". " << studentNames[i] << endl;
    }

    // Step 4: Free allocated memory
    delete[] studentNames;
    cout << "\nMemory freed successfully." << endl;
}

// ======================================================================
// Section IV | All About File Handling
// ======================================================================

// Question 7 | Student Record File Writer & Reader
void runQuestion7() {
    ofstream outFile("students.txt");
    if (!outFile) {
        cout << "Error creating file!" << endl;
        return;
    }

    string name;
    float grade;

    // Step 1: Enter 3 students
    cout << "Enter 3 student records (name and grade):\n";
    for (int i = 0; i < 3; i++) {
        cout << "Student " << i + 1 << " name: ";
        cin >> name;
        cout << "Grade: ";
        cin >> grade;
        outFile << name << " " << grade << endl;
    }

    outFile.close();
    // Step 4: Confirmation message
    cout << "\nRecords saved successfully." << endl;

    // Step 3: Reopen and read back
    ifstream inFile("students.txt");
    if (!inFile) {
        cout << "Error opening file for reading!" << endl;
        return;
    }

    cout << "\n--- Student Records ---" << endl;
    cout << left << setw(20) << "Name" << "Grade" << endl;
    cout << string(30, '-') << endl;

    while (inFile >> name >> grade) {
        cout << left << setw(20) << name << fixed << setprecision(2) << grade << endl;
    }
    inFile.close();
}

// Question 8 | Student Score Writer & Reader
void runQuestion8() {
    vector<pair<string, int>> scores;  // (player name, score)
    string playerName;
    int playerScore;

    // Step 1: Read existing scores if file exists
    ifstream inFile("scores.txt");
    if (inFile) {
        while (inFile >> playerName >> playerScore) {
            scores.push_back({playerName, playerScore});
        }
        inFile.close();
    }

    // Step 2: Display existing scores
    cout << "\n--- Current Scores ---" << endl;
    if (scores.empty()) {
        cout << "No scores found. Starting with empty list." << endl;
    } else {
        for (const auto& entry : scores) {
            cout << entry.first << ": " << entry.second << endl;
        }
    }

    // Step 3: Ask for new score and append
    cout << "\nEnter new player name: ";
    cin >> playerName;
    cout << "Enter score: ";
    cin >> playerScore;
    scores.push_back({playerName, playerScore});

    // Step 4: Write entire updated list back to file
    ofstream outFile("scores.txt");
    for (const auto& entry : scores) {
        outFile << entry.first << " " << entry.second << endl;
    }
    outFile.close();

    // Find highest score
    string highestPlayer;
    int highestScore = -1;
    for (const auto& entry : scores) {
        if (entry.second > highestScore) {
            highestScore = entry.second;
            highestPlayer = entry.first;
        }
    }

    cout << "\nScores saved successfully!" << endl;
    cout << "Highest score: " << highestScore << " by " << highestPlayer << endl;
}

// ======================================================================
// Main function to run all questions
// ======================================================================
int main() {
    cout << "============================================================\n";
    cout << "Section I | All About Structures\n";
    cout << "============================================================\n";
    cout << "\n--- Question 1: Student Grade Report ---\n";
    runQuestion1();

    cout << "\n--- Question 2: Product Inventory ---\n";
    runQuestion2();

    cout << "\n============================================================\n";
    cout << "Section II | All About Classes\n";
    cout << "============================================================\n";
    cout << "\n--- Question 3: Bank Account System ---\n";
    runQuestion3();

    cout << "\n--- Question 4: Shape Calculator ---\n";
    runQuestion4();

    cout << "\n============================================================\n";
    cout << "Section III | All About Pointers\n";
    cout << "============================================================\n";
    cout << "\n--- Question 5: Pointer-Based Sorting ---\n";
    runQuestion5();

    cout << "\n--- Question 6: Dynamic Student List ---\n";
    runQuestion6();

    cout << "\n============================================================\n";
    cout << "Section IV | All About File Handling\n";
    cout << "============================================================\n";
    cout << "\n--- Question 7: Student Record File Writer & Reader ---\n";
    runQuestion7();

    cout << "\n--- Question 8: Student Score Writer & Reader ---\n";
    runQuestion8();

    return 0;
}