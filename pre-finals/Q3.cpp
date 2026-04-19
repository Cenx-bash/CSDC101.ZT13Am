#include <iostream>
using namespace std;

class BankAccount {
    string name;
    double balance;

public:
    BankAccount(string n, double b) {
        name = n;
        balance = b;
    }

    void deposit(double x) {
        if (x > 0) balance += x;
    }

    void withdraw(double x) {
        if (x <= balance) balance -= x;
    }

    void show() {
        cout << name << ": $" << balance << endl;
    }
};

int main() {
    string name;
    double bal;

    cin.ignore();
    getline(cin, name);
    cin >> bal;

    BankAccount acc(name, bal);

    int choice;
    double amt;

    do {
        cout << "\n1 Deposit 2 Withdraw 3 Show 4 Exit\n";
        cin >> choice;

        if (choice == 1) {
            cin >> amt;
            acc.deposit(amt);
        } else if (choice == 2) {
            cin >> amt;
            acc.withdraw(amt);
        } else if (choice == 3) {
            acc.show();
        }

    } while (choice != 4);
}
