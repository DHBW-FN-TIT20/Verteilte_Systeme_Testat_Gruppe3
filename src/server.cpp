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
#include <algorithm>
#include <cstring>

#include "topic.h"

// Die Liste, in welcher unsere Topics gespeichert werden
std::vector<Topic> topics;
struct connections
{
    std::vector<sockaddr_in> connections;
    std::vector<int> socket_connections;
};

std::string timeToString(time_t timeValue, const std::string& format) {
    char buffer[80]; // Assuming the formatted time string will fit within 80 characters

    std::tm* timeinfo = std::localtime(&timeValue);
    std::strftime(buffer, sizeof(buffer), format.c_str(), timeinfo);

    return buffer;
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


    std::cout << "\nMessage: "<< message << " was successfully sent to Port Number: " << ntohs(clientAddress.sin_port) << std::endl;
    return;
}

void updateTopic(int socketDescriptor, Topic topic){
    std::string response;

    response += "TopicName: " + topic.getName() + " \n";
    auto lastUpdated = topic.getTopicStatus().lastUpdated;
    response += "\n";
    response += "LastUpdated " + timeToString(lastUpdated, "%Y-%m-%d %H:%M:%S");
    response += "New Description - " + topic.getDescription();
    for (auto subscriber: topic.getTopicStatus().subscribers){
        std::cout << "Sending message to - " << ntohs(subscriber.sin_port);
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


bool isEqualSockaddrIn(const sockaddr_in& a, const sockaddr_in& b) {
    // Compare the sockaddr_in structs byte by byte
    return std::memcmp(&a, &b, sizeof(sockaddr_in)) == 0;
}




// ----------------------------------------------------
// SECTION START SERVER
// ----------------------------------------------------
void startServer() {
    connections allConnections; // speichern aller eingehenden Verbindungen
    int index = 0; // index welcher im Programm für Schleifen verwendet wird.

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
        // Erstelle eine fd_set-Menge
        fd_set readSet;
        FD_ZERO(&readSet);

        // Füge den Server-Socket zur Menge hinzu
        FD_SET(serverSocket, &readSet);
        int maxSocket = serverSocket;

        // Füge die Client-Sockets zur Menge hinzu und aktualisiere den Wert von maxSocket
        for (const auto& client_socket: allConnections.socket_connections){
            FD_SET(client_socket, &readSet);
            maxSocket = std::max(maxSocket, client_socket);
        }

        // Verwende select() zum Warten auf eingehende Daten auf den Sockets
        if (select(maxSocket + 1, &readSet, nullptr, nullptr, nullptr) < 0) {
            std::cout << "Fehler bei select()" << std::endl;
            continue;
        }



        // Überprüfe, ob der Server-Socket Daten zum Lesen bereit hat (neue Verbindung)
        if (FD_ISSET(serverSocket, &readSet)) {
            // Akzeptieren einer eingehenden Verbindung
            sockaddr_in clientAddress;
            socklen_t clientAddressSize = sizeof(clientAddress);
            int clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientAddressSize);
            if (clientSocket < 0) {
                std::cerr << "Fehler beim Akzeptieren der Verbindung" << std::endl;
                continue;
            }

            // Verbindung erfolgreich akzeptiert
            allConnections.connections.push_back(clientAddress);
            allConnections.socket_connections.push_back(clientSocket);
            int port = ntohs(clientAddress.sin_port);
            std::cout << "\n---------\nPort Nummer: " << port << " ist verbunden und bereit.\n";
            std::cout << "Socket Nummer: " << clientSocket << "\n--------\n";
            sendMessage(clientSocket, "\n------------\nHallo, du bist nun verbunden!\n------------\n", clientAddress);
        }

        // Füge die Client-Sockets zur Menge hinzu und aktualisiere den Wert von maxSocket
        for (const auto& client_socket: allConnections.socket_connections){
            FD_SET(client_socket, &readSet);
            maxSocket = std::max(maxSocket, client_socket);
        }


        index = 0; // Zurücksetzen 
        char buffer[1024];  // Puffer zum Speichern der empfangenen Daten
        int bytesRead;
        for(auto current_socket: allConnections.socket_connections){
            // Buffer säubern
            memset(buffer, 0, sizeof(buffer));
            // Füge die Client-Sockets zur Menge hinzu und aktualisiere den Wert von maxSocket
            for (const auto& client_socket: allConnections.socket_connections){
            FD_SET(client_socket, &readSet);
            maxSocket = std::max(maxSocket, client_socket);
            }

            // Verwende select() zum Warten auf eingehende Daten auf den Sockets
            if (select(maxSocket + 1, &readSet, nullptr, nullptr, nullptr) < 0) {
                std::cout << "Fehler bei select()" << std::endl;
                continue;
            }
            if(FD_ISSET(current_socket, &readSet)){
                bytesRead = recv(current_socket, buffer, sizeof(buffer), 0);
                if (bytesRead == 0 ||bytesRead < 0) {
                    std::cout << "Verbindung von Port: " << ntohs(allConnections.connections[index].sin_port) << " wurde geschlossen.";
 
                    // User von allen Topics Unsubscriben
                    for (auto deleteUserTopic : topics) {
                        auto subs = deleteUserTopic.getTopicStatus().subscribers;
                        auto it = std::find_if(subs.begin(), subs.end(), [&](const sockaddr_in& subscriber) {
                            return isEqualSockaddrIn(subscriber, allConnections.connections[index]);
                        });
                        if (it != subs.end()) {
                            deleteUserTopic.unsubscribeTopic(allConnections.connections[index]);
                        }
                    }
                    auto itCon = std::begin(allConnections.connections);
                    std::advance(itCon,index);
                    allConnections.connections.erase(itCon);
                    auto itSocket = std::begin(allConnections.socket_connections);
                    std::advance(itSocket,index);
                    allConnections.socket_connections.erase(itSocket);
                    closesocket(allConnections.socket_connections[index]);
                    continue;
                } else if (bytesRead > 0) {
                    // Die empfangene Nachricht befindet sich im 'buffer'
                    std::string message(buffer, bytesRead);
                    std::cout << "\n---------\nEmpfangene Nachricht: " << message << "\n------\n" << std::endl;

                    // Moechte der User Nachrichten publisen
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
                                std::cout << "\n----\n" << topicName << "\n" << topicDescription << "\n--------";
                                bool topicFound = false;
                                // wenn es das Topic schon gibt, publish topic und verteile an alle subscriber
                                for (auto topic: topics) {
                                    if(topic.getName() == topicName) {
                                        topicFound = true;
                                        auto returnMsg = topic.publishTopic(topicDescription);
                                        sendMessage(current_socket, returnMsg, allConnections.connections[index]);
                                        if(returnMsg == "Topic erfolgreich aktualisiert"){
                                            // Wenn Topic erfolgreich aktualisiert wurde, dann schicken wir Meldungen an alle unsere
                                            // Subscriber
                                            std::cout << "Updating Topic!";
                                            updateTopic(current_socket, topic);
                                        }
                                    }
                                }
                                if(!topicFound){
                                    sendMessage(current_socket, "Topic: " + topicName + " existiert nicht", allConnections.connections[index]);
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
                        std::string response = "\n----------\nListe von Topics:\n ";
                        for (const auto& topic : topics) {
                            response += topic + "\n";
                        }
                        response += "----------\n";
                        sendMessage(current_socket, response, allConnections.connections[index]);
                    }

                    // Moechte der User den Topic Status haben?
                    found = message.find("$s");
                    // we found the identifier for listTopics()
                    if (found != std::string::npos) {
                        std::string response = "TopicStatus \n";

                        auto topicName = message.substr(found+3);

                        for (auto topic: topics) {
                            if(topic.getName() == topicName) {
                                auto status = topic.getTopicStatus();
                                auto subs = status.subscribers;
                                auto lastUpdated = status.lastUpdated;
                                response += "Liste von Subscribern -";
                                for (auto curSub: subs){
                                    response += " - ";
                                    response += ntohs(curSub.sin_port);
                                }
                                response += "\n";
                                response += "LastUpdated - ";
                                response += timeToString(lastUpdated, "%Y-%m-%d %H:%M:%S");
                            }
                        }
                        sendMessage(current_socket, response, allConnections.connections[index]);
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
                                // Wenn Topic gefunden wurde, setze Client weg von Subscriber Liste
                                allTopics.unsubscribeTopic(allConnections.connections[index]);
                                sendMessage(current_socket, "Erfolgreich von Topic abgemeldet", allConnections.connections[index]);
                            }
                        }
                    }


                    // Moechte der User Topics abbonieren?
                    startPos = message.find("$t");
                    // Filtere Topics heraus welche subscribed werden sollen.
                    topic_array.clear(); // leere die Liste die wir vorhin schon benutzt haben könnten
                    if (startPos != std::string::npos) {
                        startPos+=3; // Skip the space after "$t"
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
                        for (auto filteredTopics: topic_array){
                                        // Wenn noch kein Topic existiert, dann erstelle ein Topic mit leerer Beschreibung
                                        if(topics.empty()){
                                            topics.emplace_back(filteredTopics, "");
                                            auto createdTopic = topics.back();
                                            createdTopic.subscribeTopic(allConnections.connections[index]);
                                            sendMessage(current_socket, "Topic nicht vorhanden, wurde neu erstellt.", allConnections.connections[index]);
                                        } else {
                                            for (auto allTopics: topics){
                                                if(filteredTopics == allTopics.getName()){
                                                    // Wenn Topic gefunden wurde, setze Client auf die Subscriber Liste
                                                    allTopics.subscribeTopic(allConnections.connections[index]);
                                                    updateTopic(current_socket, allTopics);
                                                } else {
                                                    // Wenn Topic nicht gefunden wurde, erstelle ein neues (leeres Topic)
                                                    topics.emplace_back(filteredTopics, "");
                                                    auto createdTopic = topics.back();
                                                    createdTopic.subscribeTopic(allConnections.connections[index]);
                                                    sendMessage(current_socket, "\nTopic nicht vorhanden, wurde neu erstellt.\n", allConnections.connections[index]);
                                                }
                                            }
                                        }
                        }                
                    }
                }
            }


            
            

            index++;
        }


        
        

        

        

        

        

  /*  // Schließen des Client-Sockets
    #ifdef _WIN32
    closesocket(clientSocket);
    #else
    close(clientSocket);
    #endif */
}

std::cout << "Schliessen des Sockets...\n";
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
