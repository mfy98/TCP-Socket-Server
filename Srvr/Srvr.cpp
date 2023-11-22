#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <winsock2.h>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int PORT = 12345;

struct UserInfo {
    string username;
    string password;
    string bank;
    string accountNumber;
    string card;
    int balance;
};

vector<UserInfo> userDatabase;

SOCKET initializeServerSocket() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed." << endl;
        exit(EXIT_FAILURE);
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Error creating socket: " << WSAGetLastError() << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Bind failed with error: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed with error: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    return serverSocket;
}

void readUserDataFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        exit(EXIT_FAILURE);
    }

    string line;
    while (getline(file, line)) {
        UserInfo user;
        stringstream ss(line);
        getline(ss, user.username, ',');
        getline(ss, user.password, ',');
        getline(ss, user.bank, ',');
        getline(ss, user.accountNumber, ',');
        getline(ss, user.card, ',');
        ss >> user.balance;

        userDatabase.push_back(user);
    }


}

UserInfo authenticateUser(const string& username, const string& password) {
    for (const auto& user : userDatabase) {
        if (user.username == username && user.password == password) {
            return user;
        }
    }
    return UserInfo();
}

int main() {
    SOCKET serverSocket = initializeServerSocket();
    readUserDataFromFile("data.txt");

    while (true) {
        cout << "Istemci baglantisi bekleniyor..." << endl;

        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Accept hatasi: " << WSAGetLastError() << endl;
            closesocket(serverSocket);
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        cout << "Istemci baglandi." << endl;

        char usernameBuffer[100];
        char passwordBuffer[100];

        recv(clientSocket, usernameBuffer, sizeof(usernameBuffer), 0);
        recv(clientSocket, passwordBuffer, sizeof(passwordBuffer), 0);

        string username(usernameBuffer);
        string password(passwordBuffer);

        UserInfo user = authenticateUser(username, password);
        int charge = 2;
        if (!user.username.empty()) {
            send(clientSocket, "Hosgeldin", sizeof("Hosgeldin"), 0);

            while (true) {

                char clientMessage[100];
                recv(clientSocket, clientMessage, sizeof(clientMessage), 0);
                int choice = atoi(clientMessage);


                switch (choice) {
                case 1:

                    send(clientSocket, ("Bakiye: " + to_string(user.balance)).c_str(), sizeof("Bakiye: " + to_string(user.balance)), 0);
                    break;
                case 2:

                    int withdrawalAmount;
                    recv(clientSocket, (char*)&withdrawalAmount, sizeof(withdrawalAmount), 0);

                    if (withdrawalAmount > 0 && withdrawalAmount <= user.balance) {

                        user.balance -= withdrawalAmount;


                        for (auto& u : userDatabase) {
                            if (u.username == user.username) {
                                u.balance = user.balance;
                                break;
                            }
                        }


                        ofstream outFile("data.txt");
                        if (outFile.is_open()) {
                            for (const auto& u : userDatabase) {
                                outFile << u.username << ',' << u.password << ',' << u.bank << ',' << u.accountNumber << ',' << u.card << ',' << u.balance << '\n';
                            }
                            outFile.close();
                        }
                        else {
                            cerr << "data.txt guncellenemedi." << endl;
                        }

                        send(clientSocket, ("Kalan Bakiye: " + to_string(user.balance)).c_str(), sizeof("Kalan Bakiye: " + to_string(user.balance)), 0);
                    }
                    else {
                        cout << "Hatali islem. Yetersiz bakiye." << endl;
                    }
                    break;
                case 3:
                {
                    char recipientAccountNumber[100];
                    recv(clientSocket, recipientAccountNumber, sizeof(recipientAccountNumber), 0);

                    int amount;
                    recv(clientSocket, (char*)&amount, sizeof(amount), 0);


                    UserInfo sender = authenticateUser(username, password);


                    UserInfo recipient;
                    bool recipientFound = false;

                    for (auto& u : userDatabase) {
                        if (u.accountNumber == recipientAccountNumber) {
                            recipient = u;
                            recipientFound = true;
                            break;
                        }
                    }

                    if (recipientFound) {

                        if (amount <= sender.balance) {

                            sender.balance -= amount;


                            recipient.balance += amount;

                            //Bankalar farklı ise kesinti yapıyoruz.
                            if (sender.bank != recipient.bank) {
                                sender.balance -= charge;
                            }

                            for (auto& u : userDatabase) {
                                if (u.username == sender.username) {
                                    u.balance = sender.balance;
                                    break;
                                }
                            }
                            ofstream outFile("data.txt");
                            if (outFile.is_open()) {
                                for (const auto& u : userDatabase) {
                                    outFile << u.username << ',' << u.password << ',' << u.bank << ',' << u.accountNumber << ',' << u.card << ',' << u.balance << '\n';
                                }
                                outFile.close();
                            }
                            else {
                                cerr << "data.txt guncellenemedi." << endl;
                        
                            }
 
                            for (auto& u : userDatabase) {
                                if (u.accountNumber == recipient.accountNumber) {
                                    u.balance = recipient.balance;
                                    break;
                                }
                            }



                            if (outFile.is_open()) {
                                for (const auto& u : userDatabase) {
                                    outFile << u.username << ',' << u.password << ',' << u.bank << ',' << u.accountNumber << ',' << u.card << ',' << u.balance << '\n';
                                }
                                outFile.close();
                            }
                            else {
                                cerr << "data.txt guncellenemedi." << endl;
                            }

                            send(clientSocket, ("Transfer basarili. Kalan bakiye: " + to_string(sender.balance)).c_str(), sizeof("Transfer basarili. Kalan bakiye: " + to_string(sender.balance)), 0);
                        }
                        else {
                            send(clientSocket, "Transfer icin yetersiz bakiye.", sizeof("Transfer icin yetersiz bakiye."), 0);
                        }
                    }
                    else {
                        send(clientSocket, "Alici hesap bulunamadi.", sizeof("Alici hesap bulunamadi."), 0);
                    }
                    break;
                }

                case 4:

                    int depositAmount;
                    recv(clientSocket, (char*)&depositAmount, sizeof(depositAmount), 0);

                    if (true) {

                        user.balance += depositAmount;


                        for (auto& u : userDatabase) {
                            if (u.username == user.username) {
                                u.balance = user.balance;
                                break;
                            }
                        }

                        ofstream outFile("data.txt");
                        if (outFile.is_open()) {
                            for (const auto& u : userDatabase) {
                                outFile << u.username << ',' << u.password << ',' << u.bank << ',' << u.accountNumber << ',' << u.card << ',' << u.balance << '\n';
                            }
                            outFile.close();
                        }
                        else {
                            cerr << "data.txt guncellenemedi." << endl;
                        }

                        send(clientSocket, ("Bakiyeniz: " + to_string(user.balance)).c_str(), sizeof("Bakiyeniz: " + to_string(user.balance)), 0);
                    }
                    else {
                        cout << "Hatali islem. Yetersiz bakiye." << endl;
                    }
                    break;
                default:
                    send(clientSocket, "Hatali secim. Tekrar deneyin.", sizeof("Hatali secim. Tekrar deneyin."), 0);
                    break;
                }
            }
        }
        else {

            send(clientSocket, "Hatali Giris", sizeof("Hatali Giris"), 0);
            cout << "Hatali giris. Baglanti kapatiliyor." << endl;
            closesocket(clientSocket);
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
