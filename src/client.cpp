#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <tclap/CmdLine.h>

const int BUFFER_SIZE = 1024;

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
    TCLAP::MultiArg<std::string> publishArg("", "publish", "Veröffentlichung von Topic-Nachrichten Syntax: TOPIC#NACHRICHT", false, "string");

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

    if (listArg.isSet())
    {
        // list all topics
        // Message to server: list all
        sendToServer(clientSocket, "$l");
        closeConnection(clientSocket);
        bool response = receiveFromServer(clientSocket, buffer);
        if (response) {
            std::cout << buffer << std::endl;
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
        // $p TOPIC#MESSAGE;TOPIC#MESSAGE etc.;
        std::string message = "$p ";
        for (std::string i : publishArg.getValue()) {
            message += i + ";";
        }
        sendToServer(clientSocket, message);
        closeConnection(clientSocket);
        bool response = receiveFromServer(clientSocket, buffer);
        auto output = buffer;
        bool bufferHasChanged = true;
        while(true){
            if (!response) {
                std::cout << "Keine Antwort vom Server" << std::endl;
            } else {
                // Wir wollen den Buffer nur einmal anzeigen, wenn er sich ändert.
                if(output == buffer && bufferHasChanged){
                    std::cout << "Antwort: " << output << std::endl;
                    bufferHasChanged = false;
                } else {
                    output = buffer;
                    bufferHasChanged = true;
                }
                continue;
            }
        }
    } else
    {
        std::cout << "Kein Kommando festgelegt" << std::endl;
    }

    return 0;
}