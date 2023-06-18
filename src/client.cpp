#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <vector>
#include <sstream>

const int BUFFER_SIZE = 1024;
using namespace std;

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
    int bytesRead;
    bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
    if (bytesRead == SOCKET_ERROR) {
        std::cerr << "Fehler beim Empfangen der Daten: " << WSAGetLastError() << std::endl;
        return false;
    } 
    return true;
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
int main(int argc, char** argv) {
    std::uint16_t port = 12345;
    std::string ip = "127.0.0.1";
    char buffer[BUFFER_SIZE];
    bool run = true;
    string arg = argv[0];
    if (argc > 1) {
        std::string arg = argv[1];
        try {
            port = std::stoi(arg);
        } catch (const std::invalid_argument& e) {
            std::cout << "Ungültiges Argument für Port: " << arg << std::endl;
            return 1;
        } catch (const std::out_of_range& e) {
            std::cout << "Überlauf beim Umwandeln des Ports: " << arg << std::endl;
            return 1;
        }
    } else {
        std::cout << "Kein Port festgelegt. Verwende Standard Port: " << port << std::endl;
    }
    SOCKET clientSocket = createSocket();
    std::cout << "Verbinde zum Server: " + ip + ":" + std::to_string(port) << std::endl;

    bool success = connectToServer(clientSocket, ip, port);

    //  Willkommens Nachricht
    bool response = receiveFromServer(clientSocket, buffer);
    if (response) {
        std::cout << buffer << std::endl;
    }

    std::cout << "\n------------\nWelchen Befehl moechtest du ausfuehren? Mit \"help\" kannst du dir alle Befehle anzeigen lassen!\n---------\n";
    while (run) {

        // Buffer säubern
        memset(buffer, 0, sizeof(buffer));

        //Namen maximal 32 ASCII-Zeichen; Nachrichten maximal 255 Zeichen
        std::string input = "";
        std::cin >> input;
        std::string delimiter = ":";
        string command = input.substr(0, input.find(delimiter));
        if (command == "list") {
            // list all topics
            // Aufruf: list
            // Message to server: $l
            sendToServer(clientSocket, "$l");
            //closeConnection(clientSocket);
            bool response = receiveFromServer(clientSocket, buffer);
            if (response) {
                std::cout << buffer << std::endl;
            } else {
                std::cout << "Keine Antwort vom Server" << std::endl;
            }
        } else if (command == "sub") {
            // subscribe to topic
            // Aufruf: sub:Topic1;Topic2 etc.
            // An Server: $t Topic1;Topic2 etc.

            // Extrahiere die Topics aus dem Eingabestring
            size_t pos = input.find(":");
            std::string topicsString = input.substr(pos + 1); // "Topic1;Topic2"
            std::string message = "$t " + topicsString;
            sendToServer(clientSocket, message);
            //closeConnection(clientSocket);
            bool response = receiveFromServer(clientSocket, buffer);
            if (!response) {
                std::cout << "Keine Antwort vom Server" << std::endl;
            } else {
                std::cout << buffer << std::endl;
            }
        } else if (command == "publish") {
            // publish message
            // Aufruf: publish:Topic1#Message1;Topic2#Message2;etc.
            // An Server: $p TOPIC#MESSAGE;TOPIC#MESSAGE etc.;
            size_t pos = input.find(":");
            std::string publish = input.substr(pos + 1); // "Topic1;Topic2"
            std::string message = "$p ";
            if (!publish.empty() && publish.back() != ';') {
                message += publish + ";";
            } else {
                message += publish;
            }

            sendToServer(clientSocket, message);
            //closeConnection(clientSocket);
            bool response = receiveFromServer(clientSocket, buffer);
            if (!response) {
                std::cout << "Keine Antwort vom Server" << std::endl;
            } else {
                std::cout << buffer << std::endl;
            }
        } else if (command == "exit") {
            // exit program
            // Aufruf: exit
            run = false;
        } else if (command == "status") {
            // get topic status
            // Aufruf: status:Topicname
            // An Server: $s TOPICNAME
            size_t pos = input.find(":");
            std::string topicsString = input.substr(pos + 1); // "Topic1;Topic2"
            std::string message = "$s " + topicsString;
            sendToServer(clientSocket, message);
            bool response = receiveFromServer(clientSocket, buffer);
            if (!response) {
                std::cout << "Keine Antwort vom Server" << std::endl;
            } else {
                std::cout << buffer << std::endl;
            }
            
        } else if (command == "help") {
            // Aufruf: help
            std::cout << "Folgende Befehle sind verfuegbar:" << std::endl;
            std::cout << "list" << std::endl;
            std::cout << "topic Topic1 Topic2 etc." << std::endl;
            std::cout << "publish Topic1#Message1 Topic2#Message2 etc." << std::endl;
            std::cout << "exit" << std::endl;
        } else {
            std::cout << "Unbekannter Befehl" << std::endl;
        }
    }
    return 0;
}
    
    