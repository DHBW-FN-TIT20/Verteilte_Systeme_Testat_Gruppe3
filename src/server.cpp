#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib") // Linken der Windows-Sockets-Bibliothek
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <ctime>

#include "topic.h"

// Function to retrieve the list of available topics
std::vector<std::string> listTopics()
{
    std::vector<std::string> topics;

    // Add sample topics to the list
    topics.push_back("Topic 1");
    topics.push_back("Topic 2");
    topics.push_back("Topic 3");

    return topics;
}

// Function to send a message through a specific port
void sendMessage(int socketDescriptor, const std::string& message, const sockaddr_in& clientAddress) {
    const int port = 12345;

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (sendto(socketDescriptor, message.c_str(), message.size(), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress)) < 0) {
        std::cerr << "Failed to send message" << std::endl;
        return;
    }

    std::cout << "Message sent successfully" << std::endl;
}


// Globale Map zum Speichern der Topics
std::map<std::string, Topic> topics;

// ----------------------------------------------------
// SECTION START SERVER
// ----------------------------------------------------
void startServer() {
    // Windows-spezifische Initialisierung der Winsock-Bibliothek
    #ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Fehler beim Initialisieren von Winsock" << std::endl;
        return;
    }
    #endif

    // Erstellen eines Sockets
    int serverSocket;
    #ifdef _WIN32
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    #else
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    #endif
    if (serverSocket < 0) {
        std::cerr << "Fehler beim Erstellen des Sockets" << std::endl;
        return;
    }

    // Binden des Sockets an eine IP-Adresse und Port
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    // serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(12345); // Beispielport, hier anpassen wenn gewünscht

    if (bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Fehler beim Binden des Sockets" << std::endl;
        return;
    }

    // Lauschen auf eingehende Verbindungen
    if (listen(serverSocket, SOMAXCONN) < 0) {
        std::cerr << "Fehler beim Lauschen auf Verbindungen" << std::endl;
        return;
    }

    // Ausgabe der Adresse, an der der Server gestartet wurde
    sockaddr_in addr{};
    socklen_t addrLength = sizeof(addr);
    if (getsockname(serverSocket, reinterpret_cast<sockaddr*>(&addr), &addrLength) == -1) {
        std::cerr << "Fehler beim Abrufen der Serveradresse." << std::endl;
    } else {
        char ipAddress[INET_ADDRSTRLEN];
        std::string ipString = inet_ntoa(addr.sin_addr);
        strncpy(ipAddress, ipString.c_str(), INET_ADDRSTRLEN);
        std::cout << "Server gestartet an Adresse: " << ipAddress << std::endl;
    }

    std::cout << "Server gestartet. Warte auf Verbindungen..." << std::endl;

    // Endlosschleife zum Akzeptieren von Verbindungen
    while (true) {
        // Akzeptieren einer eingehenden Verbindung
        sockaddr_in clientAddress;
        socklen_t clientAddressSize = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientAddressSize);
        if (clientSocket < 0) {
            std::cerr << "Fehler beim Akzeptieren der Verbindung" << std::endl;
            continue;
        }

        char buffer[1024];  // Puffer zum Speichern der empfangenen Daten
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead < 0) {
            std::cerr << "Fehler beim Empfangen der Nachricht" << std::endl;
            continue;
        }

        // Die empfangene Nachricht befindet sich im 'buffer'
        std::string message(buffer, bytesRead);
        std::cout << "Empfangene Nachricht: " << message << std::endl;
        
        size_t found = message.find("$l");
        // we found the identifier for listTopics()
        if (found != std::string::npos) {
            auto topics = listTopics();
            std::string response = "List of topics: ";
            for (const auto& topic : topics) {
                response += topic + ", ";
            }
            response.pop_back();  // Remove the last comma
            response.pop_back();  // Remove the space after the last topic
            sendMessage(clientSocket, response, clientAddress);
        }

        // Verbindung behandeln (Hier Implementierung der Logik zur Kommunikation mit dem Client)
        std::cout << "Verbindung!!!" << std::endl;


            // Schließen des Client-Sockets
    #ifdef _WIN32
    closesocket(clientSocket);
    #else
    close(clientSocket);
    #endif
}

// Schließen des Server-Sockets
#ifdef _WIN32
closesocket(serverSocket);
WSACleanup();
#else
close(serverSocket);
#endif
}
int main() {
startServer();
return 0;
}
