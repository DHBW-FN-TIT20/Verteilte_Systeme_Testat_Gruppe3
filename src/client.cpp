#include <iostream>
#include <cstring>
#include <winsock2.h>
<<<<<<< Updated upstream
#include <tclap/CmdLine.h>

const int BUFFER_SIZE = 1024;
=======
#include <vector>
#include <sstream>
#include <string>
#include <algorithm>

const int BUFFER_SIZE = 1024;
using namespace std;

bool isAllAscii(const std::string& str)
{
    return std::all_of(
                str.begin(),
                str.end(),
                [](const unsigned char& ch){
                    return ch <= 127;
                });
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
>>>>>>> Stashed changes

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

    TCLAP::CmdLine cmd("Publisher/Subscriber System", ' ', "1");
    TCLAP::SwitchArg listArg ("", "list", "Liste alle Topics auf", false);
    TCLAP::ValueArg<std::string> portArg("", "port", "Verbindungsport", false, "", "string");
    TCLAP::MultiArg<std::string> topicArg("", "topic", "Topic zum abbonieren", false, "string");
    TCLAP::MultiArg<std::string> publishArg("", "publish", "Veröffentlichung von Topic-Nachrichten Syntax: TOPIC§NACHRICHT", false, "string");

    cmd.add(portArg);
    cmd.add(topicArg);
    cmd.add(publishArg);
    cmd.add(listArg);
    cmd.parse(argc, argv);

    std::uint16_t port = 12345;
    std::string ip = "127.0.0.1";
    char buffer[BUFFER_SIZE];
    //use port for every connection
    if (portArg.isSet()) 
    {
        port = std::stoi(portArg.getValue());
    } else {
        std::cout << "Kein Port festgelegt. Verwende Standard Port: " + std::to_string(port) << std::endl;
    }

    SOCKET clientSocket = createSocket();
    std::cout << "Verbinde zum Server: " + ip + ":" + std::to_string(port) << std::endl;
    bool success = connectToServer(clientSocket, ip, port);

<<<<<<< Updated upstream
    if (listArg.isSet())
    {
        // list all topics
        // Message to server: list all
        sendToServer(clientSocket, "$l");
        closeConnection(clientSocket);
        bool response = receiveFromServer(clientSocket, buffer);
        if (response) {
            std::cout << buffer << std::endl;
=======
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
        if (isAllAscii(input)) { 
        } else {
            std::cout << "Ungueltiger Befehl. Es werden ausschliesslich ASCII-Zeichen akzeptiert." << std::endl;
            continue;
        }
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
>>>>>>> Stashed changes
        } else {
            std::cout << "Keine Antwort vom Server" << std::endl;
        }
    } else if (topicArg.isSet())
    {
        // subscribe to topic
        // $t Topic1;Topic2 etc.
        std::vector<std::string> topics = topicArg.getValue();
        std::string message = "$t ";
        for (std::string i : topics) {
            message += i + ";";
        }
        message.pop_back();
        sendToServer(clientSocket, message);
        closeConnection(clientSocket);
        bool response = receiveFromServer(clientSocket, buffer);
        if (!response) {
            std::cout << "Keine Antwort vom Server" << std::endl;
        } else {
            //Ausgabe der Texte
        }
    } else if (publishArg.isSet())
    {
        // publish message
        // t$Topic1 m$ Message
        // $p TOPIC§MESSAGE;TOPIC§MESSAGE etc.;
        std::string message = "$p ";
        for (std::string i : publishArg.getValue()) {
            message += i + ";";
        }
        sendToServer(clientSocket, message);
        closeConnection(clientSocket);
        bool response = receiveFromServer(clientSocket, buffer);
        std::cout << "dieser";
        while(true){
            if (!response) {
                std::cout << "Keine Antwort vom Server" << std::endl;
            } else {
                std::cout << "buffer: " << buffer << std::endl;
                continue;
            }
        }
    } else
    {
        std::cout << "Kein Kommando festgelegt" << std::endl;
    }

    return 0;
}