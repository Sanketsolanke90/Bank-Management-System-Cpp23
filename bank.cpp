// Bank Management System (C++23) with Password/PIN Protection
// Author: Sanket Solanke
// Features: File persistence, account operations, password hashing, C++23 modern design
// Suitable for GitHub portfolio

#include <iostream>
#include <vector>
#include <fstream>
#include <optional>
#include <algorithm>
#include <filesystem>
#include <ranges>
#include <limits>
#include <iomanip>
#include <string>
#include <functional>

namespace fs = std::filesystem;
using namespace std;

// ---------------- Enum for Menu ----------------
enum class Menu : int {
    CreateAccount = 1,
    ShowAll = 2,
    Search = 3,
    Deposit = 4,
    Withdraw = 5,
    Transfer = 6,
    CloseAccount = 7,
    UpdateName = 8,
    HighBalance = 9,
    SortAccounts = 10,
    Exit = 0
};

// ---------------- Input Helpers ----------------
bool getInt(const string& prompt, int& value, int min = numeric_limits<int>::min(), int max = numeric_limits<int>::max()) {
    while (true) {
        cout << prompt;
        if (cin >> value && value >= min && value <= max) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return true;
        }
        cout << "Invalid input. Please enter a valid number.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

bool getDouble(const string& prompt, double& value, double min = numeric_limits<double>::lowest(), double max = numeric_limits<double>::max()) {
    while (true) {
        cout << prompt;
        if (cin >> value && value >= min && value <= max) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return true;
        }
        cout << "Invalid input. Please enter a valid number.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

bool getNonEmptyString(const string& prompt, string& value) {
    while (true) {
        cout << prompt;
        getline(cin, value);
        if (!value.empty()) return true;
        cout << "Input cannot be empty. Please try again.\n";
    }
}

// ---------------- BankAccount Class ----------------
class BankAccount {
private:
    string name;
    int accountNum{};
    double balance{};
    size_t pinHash{}; // store hash of PIN

    static size_t hashPIN(const string& pin) {
        return hash<string>{}(pin);
    }

public:
    BankAccount() = default;
    BankAccount(string n, int ac, double bal, const string& pin)
        : name(std::move(n)), accountNum(ac), balance(bal), pinHash(hashPIN(pin)) {}

    [[nodiscard]] string getName() const { return name; }
    [[nodiscard]] int getAccountNum() const { return accountNum; }
    [[nodiscard]] double getBalance() const { return balance; }

    bool verifyPIN(const string& pin) const {
        return pinHash == hashPIN(pin);
    }

    void deposit(double amount) {
        if (amount <= 0) throw invalid_argument("Deposit must be positive");
        balance += amount;
    }

    void withdraw(double amount) {
        if (amount <= 0) throw invalid_argument("Withdrawal must be positive");
        if (balance < amount) throw runtime_error("Insufficient balance");
        balance -= amount;
    }

    void updateName(const string& newName) {
        if (newName.empty()) throw invalid_argument("Name cannot be empty");
        name = newName;
    }

    void save(ofstream& out) const {
        // Store format: accountNum balance pinHash "name"
        out << accountNum << ' ' << balance << ' ' << pinHash << ' ' << quoted(name) << '\n';
    }

    static optional<BankAccount> load(ifstream& in) {
        int ac{};
        double bal{};
        size_t hash{};
        string n;
        if (in >> ac >> bal >> hash >> quoted(n)) {
            BankAccount acc;
            acc.accountNum = ac;
            acc.balance = bal;
            acc.pinHash = hash;
            acc.name = n;
            return acc;
        }
        return nullopt;
    }
};

// ---------------- BankManagement Class ----------------
class BankManagement {
private:
    vector<BankAccount> accounts;

    static bool authenticate(const BankAccount& acc) {
        string pin;
        cout << "Enter PIN for account " << acc.getAccountNum() << ": ";
        getline(cin, pin);
        if (!acc.verifyPIN(pin)) {
            cout << "Authentication failed. Invalid PIN.\n";
            return false;
        }
        return true;
    }

public:
    void addAccount(const string& name, int accountNum, double balance, const string& pin) {
        if (ranges::any_of(accounts, [&](const BankAccount& acc) { return acc.getAccountNum() == accountNum; }))
            throw runtime_error("Account number already exists");
        accounts.emplace_back(name, accountNum, balance, pin);
        cout << "Account created successfully.\n";
    }

    void showAllAccounts() const {
        cout << "\n--- All Accounts ---\n";
        if (accounts.empty()) {
            cout << "No accounts available.\n";
            return;
        }
        for (const auto& acc : accounts) {
            cout << "Name: " << acc.getName()
                 << " | Account: " << acc.getAccountNum()
                 << " | Balance: " << acc.getBalance() << '\n';
        }
    }

    [[nodiscard]] optional<reference_wrapper<BankAccount>> findAccount(int accountNum) {
        auto it = ranges::find_if(accounts, [&](BankAccount& acc) { return acc.getAccountNum() == accountNum; });
        if (it != accounts.end()) return std::ref(*it);
        return nullopt;
    }

    void deposit(int accNum, double amount) {
        auto acc = findAccount(accNum);
        if (!acc) return (void)(cout << "Account not found.\n");
        if (!authenticate(acc->get())) return;
        acc->get().deposit(amount);
        cout << "Deposit successful.\n";
    }

    void withdraw(int accNum, double amount) {
        auto acc = findAccount(accNum);
        if (!acc) return (void)(cout << "Account not found.\n");
        if (!authenticate(acc->get())) return;
        acc->get().withdraw(amount);
        cout << "Withdrawal successful.\n";
    }

    void transfer(int fromAcc, int toAcc, double amount) {
        auto from = findAccount(fromAcc);
        auto to = findAccount(toAcc);
        if (!from || !to) throw runtime_error("One or both accounts not found");
        if (fromAcc == toAcc) throw runtime_error("Cannot transfer to same account");
        if (!authenticate(from->get())) return;
        from->get().withdraw(amount);
        to->get().deposit(amount);
        cout << "Transfer successful.\n";
    }

    void updateName(int accNum, const string& newName) {
        auto acc = findAccount(accNum);
        if (!acc) return (void)(cout << "Account not found.\n");
        if (!authenticate(acc->get())) return;
        acc->get().updateName(newName);
        cout << "Account name updated.\n";
    }

    void closeAccount(int accNum) {
        auto it = ranges::find_if(accounts, [&](const BankAccount& acc) {
            return acc.getAccountNum() == accNum;
        });
        if (it == accounts.end()) return (void)(cout << "Account not found.\n");

        if (!authenticate(*it)) return;
        accounts.erase(it);
        cout << "Account closed successfully.\n";
    }

    void showHighBalance(double threshold) const {
        cout << "--- Accounts above " << threshold << " ---\n";
        bool found = false;
        for (const auto& acc : accounts) {
            if (acc.getBalance() >= threshold) {
                found = true;
                cout << "Name: " << acc.getName()
                     << " | Account: " << acc.getAccountNum()
                     << " | Balance: " << acc.getBalance() << '\n';
            }
        }
        if (!found) cout << "No accounts meet the threshold.\n";
    }

    void sortAccountsByBalance() {
        ranges::sort(accounts, {}, &BankAccount::getBalance);
        cout << "Accounts sorted by balance.\n";
    }

    void saveToFile(const string& filename) const {
        ofstream out(filename);
        if (!out) throw runtime_error("Cannot open file for saving");
        for (const auto& acc : accounts) acc.save(out);
    }

    void loadFromFile(const string& filename) {
        if (!fs::exists(filename)) return;
        ifstream in(filename);
        accounts.clear();
        while (true) {
            auto acc = BankAccount::load(in);
            if (!acc) break;
            accounts.push_back(*acc);
        }
    }
};

// ---------------- Menu Helper ----------------
inline void printMenu() {
    cout << "\n=== Bank Management System (C++23) with PIN ===\n";
    cout << "1. Create Account\n"
         << "2. Show All Accounts\n"
         << "3. Search Account\n"
         << "4. Deposit Money\n"
         << "5. Withdraw Money\n"
         << "6. Transfer Money\n"
         << "7. Close Account\n"
         << "8. Update Account Name\n"
         << "9. Show High Balance Accounts\n"
         << "10. Sort Accounts by Balance\n"
         << "0. Exit\n";
}

// ---------------- Main ----------------
int main() {
    BankManagement bank;
    const string filename = "accounts_secure.txt";

    bank.loadFromFile(filename);

    int choice{};
    do {
        printMenu();
        if (!getInt("Enter choice: ", choice, 0, 10)) continue;
        try {
            switch (static_cast<Menu>(choice)) {
                case Menu::CreateAccount: {
                    string name, pin;
                    int num;
                    double bal;
                    getNonEmptyString("Name: ", name);
                    getInt("Account Number: ", num, 1);
                    getDouble("Initial Balance: ", bal, 0.0);
                    getNonEmptyString("Set 4-digit PIN: ", pin);
                    if (pin.size() != 4 || !ranges::all_of(pin, ::isdigit))
                        throw invalid_argument("PIN must be 4 digits.");
                    bank.addAccount(name, num, bal, pin);
                    break;
                }
                case Menu::ShowAll: bank.showAllAccounts(); break;
                case Menu::Search: {
                    int num;
                    getInt("Enter account number: ", num, 1);
                    if (auto acc = bank.findAccount(num))
                        cout << "Found -> " << acc->get().getName()
                             << " | Balance: " << acc->get().getBalance() << '\n';
                    else
                        cout << "Account not found.\n";
                    break;
                }
                case Menu::Deposit: {
                    int num;
                    double amt;
                    getInt("Account number: ", num, 1);
                    getDouble("Amount: ", amt, 0.01);
                    bank.deposit(num, amt);
                    break;
                }
                case Menu::Withdraw: {
                    int num;
                    double amt;
                    getInt("Account number: ", num, 1);
                    getDouble("Amount: ", amt, 0.01);
                    bank.withdraw(num, amt);
                    break;
                }
                case Menu::Transfer: {
                    int from, to;
                    double amt;
                    getInt("From account: ", from, 1);
                    getInt("To account: ", to, 1);
                    getDouble("Amount: ", amt, 0.01);
                    bank.transfer(from, to, amt);
                    break;
                }
                case Menu::CloseAccount: {
                    int num;
                    getInt("Enter account to close: ", num, 1);
                    bank.closeAccount(num);
                    break;
                }
                case Menu::UpdateName: {
                    int num;
                    string newName;
                    getInt("Enter account number: ", num, 1);
                    getNonEmptyString("New Name: ", newName);
                    bank.updateName(num, newName);
                    break;
                }
                case Menu::HighBalance: {
                    double threshold;
                    getDouble("Enter threshold: ", threshold, 0.0);
                    bank.showHighBalance(threshold);
                    break;
                }
                case Menu::SortAccounts: bank.sortAccountsByBalance(); break;
                case Menu::Exit:
                    cout << "Saving data...\n";
                    bank.saveToFile(filename);
                    return 0;
            }
        } catch (const exception& e) {
            cerr << "Error: " << e.what() << '\n';
        }
    } while (true);
}
