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
std::vector<sockaddr_in> connections;

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
    return;
}

void updateTopic(int socketDescriptor, Topic topic){
    std::string response;

    response += "TopicName: " + topic.getName() + " \n";
    auto lastUpdated = topic.getTopicStatus().lastUpdated;
    response += "\n";
    response += "LastUpdated " + lastUpdated;
    response += "New Description - " + topic.getDescription();
    for (auto subscriber: topic.getTopicStatus().subscribers){
        sendMessage(socketDescriptor, response, subscriber);
    }
    return;
}
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
        std::cout << "Port Nummer: " << port << " ist verbunden und bereit.";
        bool portOccupied = false;
        for (auto connection : connections) {
            if (connection.sin_port == port) {
                portOccupied = true;
                break;
            }
        }
        if (portOccupied) {
            std::cerr << "Port " << port << " bereits belegt" << std::endl;
            sendMessage(clientSocket, "Port belegt", clientAddress);
            closesocket(clientSocket);
            continue;
        } else {
            connections.push_back(clientAddress);
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
            startPos += 2; // Skip "$p"
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
                            auto returnMsg = topic.publishTopic(topicDescription);
                            sendMessage(clientSocket, returnMsg, clientAddress);
                            if(returnMsg == "Topic erfolgreich aktualisiert"){
                                // Wenn Topic erfolgreich aktualisiert wurde, dann schicken wir Meldungen an alle unsere
                                // Subscriber
                                updateTopic(clientSocket, topic);
                            }
                        }
                    }
                    if(!topicFound){
                        sendMessage(clientSocket, "Topic: " + topicName + " existiert nicht", clientAddress);
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

        // Moechte der User alle Topics gelistet haben?
        size_t found = message.find("$l");
        // we found the identifier for listTopics()
        if (found != std::string::npos) {
            auto topics = listTopics();
            std::string response = "Liste von Topics: ";
            for (const auto& topic : topics) {
                response += topic + ", ";
            }
            response.pop_back();  // Remove the last comma
            response.pop_back();  // Remove the space after the last topic
            sendMessage(clientSocket, response, clientAddress);
        }

        // Moechte der User den Topic Status haben?
        found = message.find("$s");
        // we found the identifier for listTopics()
        if (found != std::string::npos) {
            std::string response = "TopicStatus \n";

            auto topicName = message.substr(found+2);

            for (auto topic: topics) {
                if(topic.getName() == topicName) {
                    auto subscribers = topic.getTopicStatus().subscribers;
                    auto lastUpdated = topic.getTopicStatus().lastUpdated;
                    response += "Liste von Subscribern";
                    for (auto curSub: subscribers){
                        response += " ";
                        response += curSub.sin_port;
                    }
                    response += "\n";
                    response += "LastUpdated ";
                    response += lastUpdated;
                }
            }
            sendMessage(clientSocket, response, clientAddress);
        }

        // Moechte der User Topics deabbonieren?
        startPos = message.find("$u");
        // Filtere Topics heraus welche unsubscribed werden sollen.
        std::vector<std::string> topic_array;
        if (startPos != std::string::npos) {
            startPos++; // Skip the space after "$u"
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
        for (auto filteredTopics: topic_array){
            for (auto allTopics: topics){
                if(filteredTopics == allTopics.getName()){
                    // Wenn Topic gefunden wurde, setze Client auf die Subscriber Liste
                    allTopics.unsubscribeTopic(clientAddress);
                    sendMessage(clientSocket, "Erfolgreich von Topic abgemeldet", clientAddress);
                }
            }
        }

        // Moechte der User Topics abbonieren?
        startPos = message.find("$t");
        // Filtere Topics heraus welche subscribed werden sollen.
        topic_array.clear(); // leere die Liste die wir vorhin schon benutzt haben könnten
        if (startPos != std::string::npos) {
            startPos+=2; // Skip the space after "$t"
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
        for (auto filteredTopics: topic_array){
            if(topics.empty()){
                topics.emplace_back(filteredTopics, "");
                sendMessage(clientSocket, "Topic nicht vorhanden, wurde neu erstellt.", clientAddress);
            } else {
                for (auto allTopics: topics){
                    if(filteredTopics == allTopics.getName()){
                        // Wenn Topic gefunden wurde, setze Client auf die Subscriber Liste
                        allTopics.subscribeTopic(clientAddress);
                        updateTopic(clientSocket, allTopics);
                    } else {
                        // Wenn Topic nicht gefunden wurde, erstelle ein neues (leeres Topic)
                        topics.emplace_back(filteredTopics, "");
                        sendMessage(clientSocket, "Topic nicht vorhanden, wurde neu erstellt.", clientAddress);
                    }
                }
            }
        }

    std::cout << "Schliessen des clientSockets...";
    // Schließen des Client-Sockets
    #ifdef _WIN32
    closesocket(clientSocket);
    #else
    close(clientSocket);
    #endif
}

std::cout << "Schliessen des Sockets...";
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
