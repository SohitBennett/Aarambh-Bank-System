#include <iostream>
#include "sqlite3.h"
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cmath>

using namespace std;

int callbackReadAccount(void* data, int argc, char** argv, char** colNames);
void depositWithdraw(sqlite3* db, int acno, int option, int pin);
void displaySingleAccount(sqlite3* db, int acno, int pin);
void displayAllAccounts(sqlite3* db);
int callbackDisplayAllAccounts(void* data, int argc, char** argv, char** colNames);
void deleteAccount(sqlite3* db, int acno, int pin);
void transferFund(sqlite3* db);
int callbackCount(void* data, int argc, char** argv, char** colNames);

struct Account {
    int acno;
    string name;
    char type;
    int deposit;
    int pin;
};


int generateUniqueAccountNumber(sqlite3* db) {
    int newAccountNumber;
    bool isUnique;

    do {
        // Generate a random account number
        newAccountNumber = 1000 + rand() % 9000;

        // Check if the generated account number is already present in the database
        stringstream checkSql;
        checkSql << "SELECT COUNT(*) FROM accounts WHERE acno = " << newAccountNumber << ";";

        int count = 0;
        int rc = sqlite3_exec(db, checkSql.str().c_str(), callbackCount, &count, nullptr);

        if (rc != SQLITE_OK) {
            cerr << "Failed to check account number uniqueness: " << sqlite3_errmsg(db) << endl;
            exit(1);
        }

        // If count is 0, the account number is unique
        isUnique = (count == 0);

    } while (!isUnique);

    return newAccountNumber;
}

int callbackCount(void* data, int argc, char** argv, char** colNames) {
    int* count = static_cast<int*>(data);
    *count = stoi(argv[0]);
    return 0;
}




//int generateUniqueAccountNumber() {
    // You might want to implement a proper logic for generating unique account numbers
    // This is a simple placeholder, replace it with your own implementation
    //return 1000 + rand() % 9000;
//}

sqlite3* initializeDatabase() {
    sqlite3* db;
    int rc = sqlite3_open("bank.db", &db);

    if (rc != SQLITE_OK) {
        cerr << "Cannot open the database: " << sqlite3_errmsg(db) << endl;
        exit(1);
    }

    const char* sql = "CREATE TABLE IF NOT EXISTS accounts ("
                     "acno INTEGER PRIMARY KEY, "
                     "name TEXT, "
                     "type CHAR, "
                     "deposit INTEGER, "
                     "pin INTEGER);";

    rc = sqlite3_exec(db, sql, nullptr, nullptr, nullptr);

    if (rc != SQLITE_OK) {
        cerr << "Cannot create the table: " << sqlite3_errmsg(db) << endl;
        exit(1);
    }

    return db;
}

void writeAccountToDatabase(sqlite3* db, const Account& account) {
    stringstream sql;
    sql << "INSERT INTO accounts (acno, name, type, deposit, pin) VALUES ("
        << account.acno << ", '" << account.name << "', '" << account.type << "', "
        << account.deposit << ", " << account.pin << ");";

    int rc = sqlite3_exec(db, sql.str().c_str(), nullptr, nullptr, nullptr);

    if (rc != SQLITE_OK) {
        cerr << "Failed to write account to database: " << sqlite3_errmsg(db) << endl;
    }
}

int callbackReadAccount(void* data, int argc, char** argv, char** colNames) {
    Account* account = static_cast<Account*>(data);

    account->acno = stoi(argv[0]);
    account->name = argv[1];
    account->type = argv[2][0];
    account->deposit = stoi(argv[3]);
    account->pin = stoi(argv[4]);

    return 0;
}




Account readAccountFromDatabase(sqlite3* db, int acno, int pin) {
    Account account;

    stringstream sql;
    sql << "SELECT * FROM accounts WHERE acno = " << acno << ";";

    int rc = sqlite3_exec(db, sql.str().c_str(), callbackReadAccount, &account, nullptr);

    if (rc != SQLITE_OK) {
        cerr << "Failed to read account from database: " << sqlite3_errmsg(db) << endl;
        exit(1);
    }

    return account;
}



void updateAccountInDatabase(sqlite3* db, const Account& account) {
    stringstream sql;
    sql << "UPDATE accounts SET name = '" << account.name << "', type = '"
        << account.type << "', deposit = " << account.deposit << " WHERE acno = " << account.acno << ";";

    int rc = sqlite3_exec(db, sql.str().c_str(), nullptr, nullptr, nullptr);

    if (rc != SQLITE_OK) {
        cerr << "Failed to update account in database: " << sqlite3_errmsg(db) << endl;
    }
}

void depositWithdraw(sqlite3* db, int acno, int option, int pin) {
    Account account = readAccountFromDatabase(db, acno, pin);

    if (account.acno != acno){
        account.acno=0;
    }

    if (account.acno != 0 && account.pin != pin) {
        account.pin = 0; // Set acno to 0 to indicate incorrect PIN
    }

    if (account.acno == 0) {
        cout << "\n\n\t\t\tAccount number does not exist";
    } else if (account.pin == 0) {
        cout << "\n\n\t\t\tIncorrect PIN";
    } else {
        int amt;
        switch (option) {
            case 1:
                cout << "\n\n\t\t\tTO DEPOSIT AMOUNT";
                cout << "\n\n\t\t\tEnter The amount to be deposited: ";
                cin >> amt;
                amt = abs(amt);
                account.deposit += amt;
                break;
            case 2:
                cout << "\n\n\t\t\tTO WITHDRAW AMOUNT";
                cout << "\n\n\t\t\tEnter The amount to be withdrawn: ";
                cin >> amt;
                amt=abs(amt);
                if (account.deposit < amt) {
                    cout << "\n\t\t\tInsufficient balance";
                } else {
                    account.deposit -= amt;
                }
                break;
            default:
                cout << "\n\t\t\tInvalid option";
                return;
        }

        updateAccountInDatabase(db, account);
        cout << "\n\n\t\t\tRecord Updated";
    }
}



void displaySingleAccount(sqlite3* db, int acno, int pin) {
    Account account = readAccountFromDatabase(db, acno, pin);

    if (account.acno != 0 && account.pin != pin) {
        account.pin = 0; // Set acno to 0 to indicate incorrect PIN
    }

    if (account.acno == 0) {
        cout << "\n\n\t\t\tAccount number does not exist";
    } else if (account.pin == 0) {
        cout << "\n\n\t\t\tIncorrect PIN";
    } else {
        cout << "\n\t\t\tAccount No. : " << account.acno;
        cout << "\n\t\t\tAccount Holder Name : " << account.name;
        cout << "\n\t\t\tType of Account : " << account.type;
        cout << "\n\t\t\tBalance amount : " << account.deposit;
    }
}

void displayAllAccounts(sqlite3* db) {
    cout << "\n\n\t\tACCOUNT HOLDER LIST\n\n";
    cout << "====================================================\n";
    cout << "A/c no.      NAME           Type  Balance\n";
    cout << "====================================================\n";

    const char* sql = "SELECT * FROM accounts;";
    int rc = sqlite3_exec(db, sql, callbackDisplayAllAccounts, nullptr, nullptr);

    if (rc != SQLITE_OK) {
        cerr << "Failed to display all accounts: " << sqlite3_errmsg(db) << endl;
    }
}

int callbackDisplayAllAccounts(void* data, int argc, char** argv, char** colNames) {
    cout << argv[0] << setw(10) << " " << argv[1] << setw(10) << " " << argv[2] << setw(6) << argv[3] << endl;
    return 0;
}

void deleteAccount(sqlite3* db, int acno, int pin) {
    Account account = readAccountFromDatabase(db, acno, pin);


    if (account.acno != acno){
        account.acno=0;
    }

    if (account.acno != 0 && account.pin != pin) {
        account.pin = 0;
    }

    if (account.acno == 0) {
        cout << "\n\n\t\t\tAccount number does not exist";
    } else if (account.pin == 0) {
        cout << "\n\n\t\t\tIncorrect PIN";
    } else {
        const char* sql = "DELETE FROM accounts WHERE acno = ?;";
        sqlite3_stmt* stmt;

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

        if (rc != SQLITE_OK) {
            cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << endl;
            return;
        }

        sqlite3_bind_int(stmt, 1, acno);

        rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE) {
            cerr << "Failed to delete account: " << sqlite3_errmsg(db) << endl;
        } else {
            cout << "\n\n\t\t\tRecord Deleted";
        }

        sqlite3_finalize(stmt);
    }
}

void transferFund(sqlite3* db) {
    int fromAcno, toAcno, amount, pin;

    cout << "Enter the account number to transfer from: ";
    cin >> fromAcno;
    cout << "Enter the account number to transfer to: ";
    cin >> toAcno;
    cout << "Enter the amount to transfer: ";
    cin >> amount;
    cout << "Enter the PIN of your account: ";
    cin >> pin;

    if (fromAcno == toAcno) {
        cout << "\t\t\tAccount numbers are the same! ";
        return;
    }

    Account fromAccount = readAccountFromDatabase(db, fromAcno, pin);
    Account toAccount = readAccountFromDatabase(db, toAcno, pin);

    if(fromAccount.acno != fromAcno){
        fromAccount.acno=0;
    }
    if(toAccount.acno != toAcno){
        toAccount.acno=0;
    }
    if (fromAccount.acno != 0 && fromAccount.pin != pin) {
        fromAccount.pin = 0;
    }




    if (fromAccount.acno == 0 || toAccount.acno == 0) {
        cout << "\n\n\t\t\tOne or both accounts were not found.";
    }else if(fromAccount.pin==0){
        cout << "\n\n\t\t\tIncorrect PIN";
    }
    else {
        if (fromAccount.deposit >= amount) {
            fromAccount.deposit -= amount;
            toAccount.deposit += amount;

            updateAccountInDatabase(db, fromAccount);
            updateAccountInDatabase(db, toAccount);

            cout << "\n\t\t\tTransfer Successful!";
        } else {
            cout << "\n\t\t\tInsufficient balance in the source account.";
        }
    }
}

int main() {
    sqlite3* db = initializeDatabase();
    char ch;
    int num;
    int p;

    do {
        system("CLS");
        cout << "\n\n\t\t\t\t======================\n";
        cout << "\t\t\t\t     AARAMBH BANK";
        cout << "\n\t\t\t\t======================\n";

        cout << "\t\t\t\t    ::MAIN MENU::\n";
        cout << "\n\t\t\t\t1. NEW ACCOUNT";
        cout << "\n\t\t\t\t2. DEPOSIT AMOUNT";
        cout << "\n\t\t\t\t3. WITHDRAW AMOUNT";
        cout << "\n\t\t\t\t4. BALANCE ENQUIRY";
        cout << "\n\t\t\t\t5. ALL ACCOUNT HOLDER LIST";
        cout << "\n\t\t\t\t6. CLOSE AN ACCOUNT";
        cout << "\n\t\t\t\t7. TRANSFER FUND";
        cout << "\n\t\t\t\t8. EXIT";
        cout << "\n\n\t\t\t\tSelect Your Option (1-8): ";
        cin >> ch;

        switch (ch) {
            case '1': {
                system("CLS");
                Account newAccount;
                cout << "\n\n\t\t\tEnter the Name of the Account holder: ";
                cin.ignore();
                getline(cin, newAccount.name);
                cout << "\n\t\t\tEnter Type of the Account (C/S): ";
                cin >> newAccount.type;
                newAccount.type = toupper(newAccount.type);
                cout << "\n\t\t\tEnter The Initial amount: ";
                cin >> newAccount.deposit;
                cout << "\n\t\t\tSet PIN: ";
                cin.ignore();
                cin >> newAccount.pin;

                if(newAccount.type=='S' && newAccount.deposit<0){
                    cout<<"\n\t\t\tSavings Account Can't have Negative Balance! ";
                    break;
                }

                // Generate a unique account number (you might want to implement your logic here)
                newAccount.acno = generateUniqueAccountNumber(db);
                cout << "\n\t\t\tYour Account Number: "<<newAccount.acno<<endl;
                // Now, write the new account to the database
                writeAccountToDatabase(db, newAccount);
                cout << "\n\n\t\t\tAccount Created..";
                break;
            }
            case '2':
                system("CLS");
                cout << "\n\n\t\t\tEnter The account No. : ";
                cin >> num;
                cout << "\n\n\t\t\tEnter PIN : ";
                cin >> p;
                depositWithdraw(db, num, 1, p);
                break;
            case '3':
                system("CLS");
                cout << "\n\n\t\t\tEnter The account No. : ";
                cin >> num;
                cout << "\n\n\t\t\tEnter PIN : ";
                cin >> p;
                depositWithdraw(db, num, 2, p);
                break;
            case '4':
                system("CLS");
                cout << "\n\n\t\t\tEnter The account No. : ";
                cin >> num;
                cout << "\n\n\t\t\tEnter PIN : ";
                cin >> p;
                displaySingleAccount(db, num, p);
                break;
            case '5':
                displayAllAccounts(db);
                break;
            case '6':
                system("CLS");
                cout << "\n\n\t\t\tEnter The account No. : ";
                cin >> num;
                cout << "\n\n\t\t\tEnter PIN : ";
                cin >> p;
                deleteAccount(db, num, p);
                break;
            case '7':
                system("CLS");
                transferFund(db);
                break;
            case '8':
                system("CLS");
                break;
            default:
                cout << "\a";
        }

        cin.ignore();
        cin.get();
    } while (ch != '8');

    sqlite3_close(db);
    return 0;
}

