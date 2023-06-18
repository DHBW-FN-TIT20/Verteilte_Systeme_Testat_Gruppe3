#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <vector>
#include <sstream>
#include <windows.h>
#include <regex>
const int BUFFER_SIZE = 1024;
using namespace std;


bool isValidIPAddress(const std::string& address) {
    // Überprüfung der IP-Adresse mit einer einfachen Regular Expression
    std::regex ipRegex(R"(\b(?:\d{1,3}\.){3}\d{1,3}\b)");
    return std::regex_match(address, ipRegex);
}

bool isValidPort(const std::string& port) {
    // Überprüfung des Ports auf eine gültige Ganzzahl im Bereich von 1 bis 65535
    try {
        int portNumber = std::stoi(port);
        return (portNumber >= 1 && portNumber <= 65535);
    } catch (...) {
        return false;
    }
}

// Farbige Textausgaben
void printColoredText(const std::string& text, int colorCode) {
    // Escape-Sequenz für die Farbänderung
    std::cout << "\E[" << colorCode << "m" << text << "\E[0m" << std::endl;
}

std::vector<std::string> split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

bool connectToServer(SOCKET clientSocket, std::string ip, std::uint16_t port)
{
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(ip.c_str());
    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Fehler beim Herstellen der Verbindung: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return false;
    }
    return true;
}
bool sendToServer(SOCKET clientSocket, std::string message)
{
    if (send(clientSocket, message.c_str(), strlen(message.c_str()), 0) == SOCKET_ERROR) {
        std::cerr << "Fehler beim Senden der Daten: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return false;
    }
    return true;
}
bool receiveFromServer(SOCKET clientSocket, char* buffer)
{
    int totalBytesRead = 0;
    int bytesRead;
    bool endOfMessage = false;

    while (!endOfMessage) {
        bytesRead = recv(clientSocket, buffer + totalBytesRead, BUFFER_SIZE - totalBytesRead - 1, 0);
        if (bytesRead == SOCKET_ERROR) {
            std::cerr << "Fehler beim Empfangen der Daten: " << WSAGetLastError() << std::endl;
            return false;
        }
        if (bytesRead == 0) {
            // Die Verbindung wurde geschlossen
            break;
        }
        totalBytesRead += bytesRead;
        buffer[totalBytesRead] = '\0';

        // Überprüfen, ob das Ende der Nachricht erreicht wurde
        for (int i = totalBytesRead - bytesRead; i < totalBytesRead; i++) {
            if (buffer[i] == '\n') {
                endOfMessage = true;
                break;
            }
        }

        if (endOfMessage) {
            // Nachricht vollständig empfangen
            break;
        }

        if (totalBytesRead >= BUFFER_SIZE - 1) {
            // Der Puffer ist voll, keine weiteren Daten empfangen
            break;
        }
    }

    return true;
}

bool isAllAscii(const std::string& str)
{
    return std::all_of(
                str.begin(),
                str.end(),
                [](const unsigned char& ch){
                    return ch <= 127;
                });
}
void closeConnection(SOCKET ConnectSocket) {
    int iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
    }
}
SOCKET createSocket()
{
    // Winsock initialisieren
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Fehler beim Initialisieren von Winsock." << std::endl;
        return 1;
    }
    // Socket erstellen
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Fehler beim Erstellen des Sockets: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    return clientSocket;
}


// Thread-Funktion für den Thread, der auf Nachrichten vom Socket wartet
DWORD WINAPI receiveThread(LPVOID lpParameter) {
    SOCKET clientSocket = *(reinterpret_cast<SOCKET*>(lpParameter));
    char buffer[BUFFER_SIZE];
    bool run = true;
    while (run) {
        // Empfange Nachricht vom Socket
        bool response = receiveFromServer(clientSocket, buffer);
        if (response) {
            std::string output = std::string("Antwort vom Server: ") + buffer;
            printColoredText(output, 44);
        } else {
            printColoredText("Keine Antwort vom Server. Versuche vielleicht das Programm neu zu starten.", 31);
            run = false;
            break;
        }
    }
    return 0;
}

// Thread-Funktion für den Thread, der auf die Benutzereingabe wartet
DWORD WINAPI userInputThread(LPVOID lpParameter) {
    SOCKET clientSocket = *(reinterpret_cast<SOCKET*>(lpParameter));
    bool run = true;
    std::string input;
    while (run) {
        printColoredText("\n------------\nWelchen Befehl moechtest du ausfuehren? Mit \"help\" kannst du dir alle Befehle anzeigen lassen!\n---------\n", 32);
        //Namen maximal 32 ASCII-Zeichen; Nachrichten maximal 255 Zeichen
        std::string input = "";
        // Eingabeaufforderung für Command
        while (true) {
            std::getline(std::cin, input);

            if (!isAllAscii(input)) {
                printColoredText("\n------\nBitte nur ASCII (7 Bit) Zeichen eingeben.", 31);
                continue;
            }

            if (input.find(' ') != std::string::npos) {
                printColoredText("\n------\nBitte keine Leerzeichen eingeben.", 31);
                continue;
            }

            // Wenn die Eingabe gültig ist, die Schleife verlassen
            break;
        }
        std::string delimiter = ":";
        string command = input.substr(0, input.find(delimiter));
        if (command == "list") {
            // list all topics
            // Aufruf: list
            // Message to server: $l
            sendToServer(clientSocket, "$l");
        } else if (command == "sub") {
            // Aufruf: sub:Topic1;Topic2 etc.
            // An Server: $t Topic1;Topic2 etc.
            // Extrahiere die Topics aus dem Eingabestring
            size_t pos = input.find(":");
            std::string topicsString = input.substr(pos + 1); // "Topic1;Topic2"
            std::string message = "$t " + topicsString;
            sendToServer(clientSocket, message);
        } else if(command == "unsub"){
            // Aufruf: unsub:Topic1;Topic2 etc.
            // An Server: $u Topic1;Topic2 etc.
            // Extrahiere die Topics aus dem Eingabestring
            size_t pos = input.find(":");
            std::string topicsString = input.substr(pos + 1); // "Topic1;Topic2"
            std::string message = "$u " + topicsString;
            sendToServer(clientSocket, message);
        } else if (command == "publish") {
            // Aufruf: publish:Topic1#Message1;Topic2#Message2;etc.
            // An Server: $p Topic1#Message1;Topic2#Message2 etc.;
            size_t pos = input.find(":");
            std::string publish = input.substr(pos + 1); // "Topic1;Topic2"
            std::string message = "$p ";
            // Wenn kein Semicolon am Ende, füge an.
            if (!publish.empty() && publish.back() != ';') {
                message += publish + ";";
            } else {
                message += publish;
            }
            sendToServer(clientSocket, message);
        } else if (command == "exit") {
            // Programm beenden
            run = false;
        } else if (command == "status") {
            // Aufruf: status:Topicname
            // An Server: $s Topicname
            size_t pos = input.find(":");
            std::string topicsString = input.substr(pos + 1);
            std::string message = "$s " + topicsString;
            sendToServer(clientSocket, message);
            
        } else if (command == "help") {
            auto helpString = "\n----\nFolgende Befehle sind verfuegbar\n- list\n- sub:Topic1;Topic2\n- unsub:Topic1;Topic2\n- publish:Topic1#Nachricht_1;Topic2#Nachricht_2\n- status:Topic\n- exit\n----\n";
            printColoredText(helpString, 32);
        } else {
            printColoredText("Unbekannter Befehl - gib bitte \"help\" ein um dir alle Befehle anzeigen zu lassen.", 31);        
        }

    }
}


int main(int argc, char** argv) {
    char buffer[BUFFER_SIZE];
    bool run = true;

    std::string serverAddressInput;
    std::string portInput;

    // Eingabeaufforderung für die Serveradresse
    printColoredText("\n------\n(Hinweis: Keine Eingabe benutzt den Standardserver \"127.0.0.1\")\nBitte gib die Addresse an, wo der Server gestartet werden soll: ", 32);
    std::getline(std::cin, serverAddressInput);

    // Überprüfung der Serveradresse
    while (!isValidIPAddress(serverAddressInput)) {
        if (serverAddressInput.empty()) {
            serverAddressInput = "127.0.0.1";
            break;
        }
        printColoredText("\n------\n(Hinweis: Keine Eingabe benutzt den Standardserver \"127.0.0.1\")\nUngueltige Serveradresse! Bitte erneut eingeben: ", 32);
        std::getline(std::cin, serverAddressInput);
    }

    // Eingabeaufforderung für den Port
    printColoredText("\n-----\n(Hinweis: Keine Eingabe benutzt den Standardport \"12345\")\nBitte gib den Port an, auf dem der Server gestartet werden soll: ", 32);
    std::getline(std::cin, portInput);

    // Überprüfung des Ports
    while (!isValidPort(portInput)) {
        if (portInput.empty()) {
            portInput = "12345";
            break;
        }
        printColoredText("\n-----\n(Hinweis: Keine Eingabe benutzt den Standardport \"12345\")\nUngueltiger Port! Bitte erneut eingeben: ", 32);
        std::getline(std::cin, portInput);
    }

    // Verbinde zum Server
    SOCKET clientSocket = createSocket();
    auto output = "Verbinde zum Server: " + serverAddressInput + ":" + portInput;
    printColoredText(output,32);

    // Hat Verbindung geklappt?
    bool success = connectToServer(clientSocket, serverAddressInput, std::stoi(portInput));
    if(!success){
        printColoredText("Verbindung hat nicht geklappt!", 31);
        return 0;
    }

    //  Willkommens Nachricht
    bool response = receiveFromServer(clientSocket, buffer);
    if (response) {
            std::string output = std::string("\nInitialnachricht vom Server: ") + buffer;
            printColoredText(output, 44);
    }

    // Erstelle und starte die Threads
    HANDLE userInputThreadHandle = CreateThread(NULL, 0, userInputThread, &clientSocket, 0, NULL);
    HANDLE receiveThreadHandle = CreateThread(NULL, 0, receiveThread, &clientSocket, 0, NULL);

    // Warte auf Beendigung der Threads
    WaitForSingleObject(userInputThreadHandle, INFINITE);
    WaitForSingleObject(receiveThreadHandle, INFINITE);

    // Schließe Handle der Threads
    CloseHandle(userInputThreadHandle);
    CloseHandle(receiveThreadHandle);
    // Socket ordnungsgemäß beenden
    closesocket(clientSocket);
    return 0;
}
    
    