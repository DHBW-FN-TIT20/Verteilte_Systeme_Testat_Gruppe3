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

// Die Liste, in welcher unsere Topics gespeichert werden
std::vector<Topic> topics;
std::vector<int> connections;

// Function to retrieve the list of available topics
std::vector<std::string> listTopics()
{
        std::vector<std::string> topicNames;
    
    if (topics.empty()) {
        topicNames.push_back("Derzeit keine Topics vorhanden.");
    } else {
        for (const Topic& topic : topics) {
            topicNames.push_back(topic.getName());
        }
    }
    
    return topicNames;
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

    std::cout << "Message: "<< message << " sent successfully" << std::endl;
}


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

        // Bevor wir die Verbindung Akzeptieren 

        int port = ntohs(clientAddress.sin_port);
        bool portOccupied = false;
        for (const int& connection : connections) {
            if (connection == port) {
                portOccupied = true;
                break;
            }
        }

        if (portOccupied) {
            std::cerr << "Port " << port << " bereits belegt" << std::endl;
            sendMessage(clientSocket, "Port belegt", clientAddress);
            closesocket(clientSocket);
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
        
        // Moechte der User Nachrichten publishen

        // Wenn publishen 
            // Überprüfen ob es dieses Topic schon gibt, sonst neu erstellen
            // Ansonsten neue Nachricht auf Topic publishen (Funktion aufrufen)
        size_t startPos = message.find("$p");
        if (startPos != std::string::npos) {
            startPos += 3; // Skip "$p"
            size_t endPos = message.find(";", startPos);
            while (endPos != std::string::npos) {
                std::string topicDesc = message.substr(startPos, endPos - startPos);
                size_t sepPos = topicDesc.find("#");
                if (sepPos != std::string::npos) {
                    const std::string& topicName = topicDesc.substr(0, sepPos);
                    const std::string& topicDescription = topicDesc.substr(sepPos + 1);
                    bool topicFound = false;
                    // wenn es das Topic schon gibt, publish topic und verteile an alle subscriber
                    for (auto topic: topics) {
                        if(topic.getName() == topicName) {
                            topicFound = true;
                            //topic.publishTopic(topicDescription);
                            auto var = topic.unsubscribeTopic(topicName);
                        }
                    }
                    if(!topicFound){
                        topics.emplace_back(topicName, topicDescription);
                    }
                }
                startPos = endPos + 1;
                endPos = message.find(";", startPos);
            }
            // Handle the last topic after the last semicolon
            std::string lastTopicDesc = message.substr(startPos);
            size_t sepPos = lastTopicDesc.find("#");
            if (sepPos != std::string::npos) {
                std::string lastTopicName = lastTopicDesc.substr(0, sepPos);
                std::string lastTopicDescription = lastTopicDesc.substr(sepPos + 1);
                topics.emplace_back(lastTopicName, lastTopicDescription);
            }
        }
            // Print the topics
        for (const Topic& topic : topics) {
            std::cout << "Topic: " << topic.getName() << "\n";
            std::cout << "Description: " << topic.getDescription() << "\n";
            std::cout << "------------\n";
        }


        // Moechte der User alle Topics gelistet haben?
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

        // Moechte der User Topics abbonieren?
        startPos = message.find(" ");
        // Array in welchem wir die Topics, welche subscribed werden sollen abonnieren
        std::vector<std::string> topic_array;
        if (startPos != std::string::npos) {
            startPos++; // Skip the space after "$t"
            size_t endPos = message.find(";", startPos);
            while (endPos != std::string::npos) {
                std::string topic = message.substr(startPos, endPos - startPos);
                topic_array.push_back(topic);
                startPos = endPos + 1;
                endPos = message.find(";", startPos);
            }
            // Handle the last topic after the last semicolon
            std::string lastTopic = message.substr(startPos);
            topic_array.push_back(lastTopic);
        }

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
