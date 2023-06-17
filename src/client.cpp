#include <iostream>
#include <cstring>
#include <winsock2.h>

const int BUFFER_SIZE = 1024;
using namespace std;

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
    } else {
        std::cout << "Antwort vom Server: " << buffer << std::endl;
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
    if (argc > 0 && arg.empty() != true) {
        port = stoi(arg);
    } else {
        cout << "Kein Port festgelegt. Verwende Standard Port: " + std::to_string(port) << std::endl;
    }
    SOCKET clientSocket = createSocket();
    std::cout << "Verbinde zum Server: " + ip + ":" + std::to_string(port) << std::endl;
    bool success = connectToServer(clientSocket, ip, port);


    while (run) {
        //Namen maximal 32 ASCII-Zeichen; Nachrichten maximal 255 Zeichen
        std::string input = "";
        input << std::cin;
        std::string delimiter = " ";
        command = input.substr(0, input.find(delimiter));
        if (command == "list") {
            cout << "test";
            // list all topics
            // Message to server: list all
            sendToServer(clientSocket, "$l");
            //closeConnection(clientSocket);
            bool response = receiveFromServer(clientSocket, buffer);
            if (response) {
                std::cout << buffer << std::endl;
            } else {
                std::cout << "Keine Antwort vom Server" << std::endl;
            }
        } else if (command == "topic") {
            // subscribe to topic
            // $t Topic1;Topic2 etc.

            std::vector<std::string> topics;
            
            std::string message = "$t ";
            for (std::string i : topics) {
                message += i + ";";
            }
            message.pop_back();
            sendToServer(clientSocket, message);
            //closeConnection(clientSocket);
            bool response = receiveFromServer(clientSocket, buffer);
            if (!response) {
                std::cout << "Keine Antwort vom Server" << std::endl;
            } else {
                //Ausgabe der Texte
            }
        } else if (command == "publish") {
            // publish message
            // t$Topic1 m$ Message
            // $p TOPIC§MESSAGE;TOPIC§MESSAGE etc.;
            std::string message = "$p ";
            for (std::string i : publishArg.getValue()) {
                message += i + ";";
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
            run = false;
        } else {
            std::cout << "Unbekannter Befehl" << std::endl;
        }
    }
    return 0;
}
    
    