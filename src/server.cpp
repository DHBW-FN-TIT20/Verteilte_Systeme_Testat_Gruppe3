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
#include <regex>
#include <chrono>
#include <windows.h>
#include <fstream>


#include "topic.h"

// Die Liste, in welcher unsere Topics gespeichert werden
std::vector<Topic> topics;
struct connections
{
    std::vector<sockaddr_in> connections;
    std::vector<int> socket_connections;
};

std::string timeToString(time_t timeValue, const std::string& format) {
    char buffer[80]; // Time String sollte in 80 Character passen

    std::tm* timeinfo = std::localtime(&timeValue);
    std::strftime(buffer, sizeof(buffer), format.c_str(), timeinfo);

    return buffer;
}

bool compareSockaddr(const sockaddr_in& addr1, const sockaddr_in& addr2) {
    // Vergleiche die wichtigen Felder der sockaddr_in-Struktur
    return (addr1.sin_family == addr2.sin_family &&
            addr1.sin_port == addr2.sin_port &&
            addr1.sin_addr.s_addr == addr2.sin_addr.s_addr);
}

void printColoredText(const std::string& text, int colorCode) {
    // Ausgabe auf der Konsole
    std::cout << "\E[" << colorCode << "m" << text << "\E[0m" << std::endl;

    // Ausgabe in der Datei
    std::ostringstream logStream;
    logStream << text << std::endl;
    std::ofstream logFile("Log.txt", std::ios::app);
    if (logFile.is_open()) {
        logFile << logStream.str();
        logFile.close();
    } else {
        std::cout << "Fehler beim Öffnen der Log-Datei!" << std::endl;
    }
}

// Funktion um an ein Client/Socket eine Nachricht zu senden
void sendMessage(int socketDescriptor, const std::string& message, const sockaddr_in& clientAddress) {

    if (sendto(socketDescriptor, message.c_str(), message.size(), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress)) < 0) {
        std::cerr << "Failed to send message" << std::endl;
        return;
    }


    auto output = "\nDie Nachricht: " + message + " wurde erfolgreich an Port: " + std::to_string(ntohs(clientAddress.sin_port)) + " gesendet\n";
    printColoredText(output,44);
    return;
}

// Diese Funktion updated das Topic und schickt dementsprechende Nachrichten an alle Subscriber
void updateTopic(connections allConnections, Topic& topic){
    std::string response;
    topic.updateTime();

    response += "\n-------\nTopic \"" + topic.getName() + "\" wurde geupdated!\n";
    auto lastUpdated = topic.getLastUpdated();
    response += "\n";
    response += "Zuletzt geupdated: " + timeToString(lastUpdated, "%Y-%m-%d %H:%M:%S");
    response += "\nNeue Beschreibung: " + topic.getDescription() + "\n------\n";
    for (auto subscriber: topic.getSubscribers()){
        // Wir müssen auch noch die korrekten Sockets rausfiltern
        for (int i=0; i< allConnections.connections.size(); i++){
            if (compareSockaddr(allConnections.connections[i], subscriber)) {
                int socketDescriptor = allConnections.socket_connections[i];
                sendMessage(socketDescriptor, response, subscriber);
            }
        }

    }
    return;
}

// Dieser Thread implementiert den Heart-Beat Mechanismus der Topics. Alle 20 Sekunden wird geschaut ob 
// das letzte Update von allen Topics mind. 20 Sekunden her ist, wenn ja, wird es nochmal geupdated
// und dahingehend auch alle User informiert
DWORD WINAPI UpdateTopics(LPVOID lpParam) {
    // Cast des lpParam zurück zu den ursprünglichen Parametern
    auto params = static_cast<std::pair<connections*, std::vector<Topic>*>*>(lpParam);
    connections* allConnections = params->first;
    std::vector<Topic>& topics = *(params->second);
    
    // Code, der in einem separaten Thread ausgeführt werden soll
    while (true) {
        // gehe alle topics durch und schaue ob sie geupdated werden müssen (Heart-Beat 20 Sekunden!)
        auto currentTime = std::chrono::system_clock::now();
        for (Topic& current_topic : topics) {
            // Zeit die 10 Sekunden zurückliegt
            auto lastUpdate = std::chrono::system_clock::from_time_t(current_topic.getLastUpdated());
            auto timediff = currentTime - lastUpdate;
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(timediff).count();
            // Sind seit dem letzten Update 10 Sekunden vergangen?
            if (seconds >= 20) {
                updateTopic(*allConnections, current_topic);
            }
        }
        
        Sleep(10000); // Warte 20 Sekunden
    }

    return 0;
}

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



// Funktion welche die Topics in einer Liste (Topic Name als String) zurückgibt
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
    // Vergleiche sockaddr_in structs byte für byte
    return std::memcmp(&a, &b, sizeof(sockaddr_in)) == 0;
}




// ----------------------------------------------------
// SECTION START SERVER
// ----------------------------------------------------
// In dieser Funktion wird sämtliche Serverlogik implementiert.
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


    // Binden des Sockets an eine IP-Adresse und Port
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(serverAddressInput.c_str());
    serverAddress.sin_port = htons(static_cast<u_short>(std::stoi(portInput)));

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
        printColoredText("Server gestartet an Adresse: ", 32);
        printColoredText(serverAddressInput + ":" + portInput, 32);
    }
    printColoredText("Server gestartet. Warte auf Verbindungen...", 32);

    // Parameter für den Thread
    std::pair<connections*, std::vector<Topic>*> params(&allConnections, &topics);

    // Erstellen des Thread Handlers für Heart-Beat
    HANDLE threadHandle = CreateThread(
        NULL,                                   // Standard-Sicherheitsattribute
        0,                                      // Standard-Stackgröße
        UpdateTopics,                           // Funktion, die im Thread ausgeführt wird
        &params, // Parameter für die Funktion
        0,                                      // Flags zur Steuerung des Thread-Verhaltens (hier 0 für sofortige Ausführung)
        NULL                                    // Zeiger auf eine Variable, die die Thread-ID zurückgibt (hier nicht verwendet)
    );

        if (threadHandle == NULL) {
        printColoredText("Fehler beim erstellen des Heart-Beat Threads",31);
        }


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
            continue;
        }



        // Überprüfe, ob der Server-Socket Daten zum Lesen bereit hat (neue Verbindung)
        if (FD_ISSET(serverSocket, &readSet)) {
            // Akzeptieren einer eingehenden Verbindung
            sockaddr_in clientAddress;
            socklen_t clientAddressSize = sizeof(clientAddress);
            int clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientAddressSize);
            if (clientSocket < 0) {
                printColoredText("Fehler beim Akzeptieren der Verbindung", 31);
                continue;
            }

            // Verbindung erfolgreich akzeptiert
            allConnections.connections.push_back(clientAddress);
            allConnections.socket_connections.push_back(clientSocket);
            int port = ntohs(clientAddress.sin_port);
            auto output = "\n---------\nPort Nummer: " + std::to_string(port) + " ist verbunden und bereit.\n" + "Socket Nummer: " + std::to_string(clientSocket) + "\n--------\n";
            printColoredText(output,32);
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
                printColoredText("Fehler bei select()", 31);
                continue;
            }
            if(FD_ISSET(current_socket, &readSet)){
                bytesRead = recv(current_socket, buffer, sizeof(buffer), 0);
                if (bytesRead == 0 ||bytesRead < 0) {
                    auto output = "Verbindung von Port: " + std::to_string(ntohs(allConnections.connections[index].sin_port)) + " wurde geschlossen.";
                    printColoredText(output, 32);
                    // User von allen Topics Unsubscriben
                    for (Topic& deleteUserTopic : topics) {
                        for (auto iterator = topics.begin(); iterator != topics.end(); ) {
                            auto subs = deleteUserTopic.getSubscribers();
                            auto it = std::find_if(subs.begin(), subs.end(), [&](const sockaddr_in& subscriber) {
                                return isEqualSockaddrIn(subscriber, allConnections.connections[index]);
                            });
                            if (it != subs.end()) {
                                deleteUserTopic.unsubscribeTopic(allConnections.connections[index]);
                                topics.erase(iterator);
                            } else {
                                iterator++;
                            }
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
                    auto output = "\n---------\nEmpfangene Nachricht: " + message + "\n------\n";
                    printColoredText(output,36);
                    // Moechte der User Nachrichten publisen
                    // Neue Nachricht auf Topic publishen (Funktion aufrufen)
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
                                for (Topic& topic: topics) {
                                    if(topic.getName() == topicName) {
                                        topicFound = true;
                                        auto returnMsg = topic.publishTopic(topicDescription);
                                        sendMessage(current_socket, returnMsg, allConnections.connections[index]);
                                        if(returnMsg == "Topic erfolgreich aktualisiert"){
                                            // Wenn Topic erfolgreich aktualisiert wurde, dann schicken wir Meldungen an alle unsere
                                            // Subscriber
                                            updateTopic(allConnections, topic);
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
                        // Letztes Topic nach dem letzten semicolon
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
                    // wir haben den identifier für listTopics() gefunden
                    if (found != std::string::npos) {
                        auto topics = listTopics();
                        std::string response = "\n----------\nListe von Topics:\n";
                        for (const auto& topic : topics) {
                            response += "- " + topic + "\n";
                        }
                        response += "----------\n";
                        sendMessage(current_socket, response, allConnections.connections[index]);
                    }

                    // Moechte der User den Topic Status haben?
                    found = message.find("$s");
                    // wir haben den identifier für Status gefunden
                    if (found != std::string::npos) {
                        std::string response = "\n--------TopicStatus \n";

                        auto topicName = message.substr(found+3);
                        for (Topic& topic: topics) {
                            if(topic.getName() == topicName) {
                                auto subs = topic.getSubscribers();
                                auto lastUpdated = topic.getLastUpdated();
                                response += "Liste von Subscribern ";
                                for (auto curSub: subs){
                                    response += " - ";
                                    response += std::to_string(ntohs(curSub.sin_port));
                                }
                                response += "\n";
                                response += "LastUpdated - ";
                                response += timeToString(lastUpdated, "%Y-%m-%d %H:%M:%S") + "\n--------\n";
                            }
                        }
                        sendMessage(current_socket, response, allConnections.connections[index]);
                    }


                    // Moechte der User Topics deabbonieren?
                    startPos = message.find("$u");
                    // Filtere Topics heraus welche unsubscribed werden sollen.
                    std::vector<std::string> topic_array;
                    if (startPos != std::string::npos) {
                        startPos+= 3; // Leerzeichen nach "$u" skippen
                        size_t endPos = message.find(";", startPos);
                        while (endPos != std::string::npos) {
                            std::string topic = message.substr(startPos, endPos - startPos);
                            topic_array.push_back(topic);
                            startPos = endPos + 1;
                            endPos = message.find(";", startPos);
                        }
                        // Letztes Topic nach dem letzten semicolon
                        std::string lastTopic = message.substr(startPos);
                        topic_array.push_back(lastTopic);
                        for (auto filteredTopics : topic_array) {
                            for (auto it = topics.begin(); it != topics.end(); ) {
                                if (filteredTopics == it->getName()) {
                                    // Wenn Topic gefunden wurde, setze Client weg von Subscriber Liste
                                    it->unsubscribeTopic(allConnections.connections[index]);
                                    sendMessage(current_socket, "Erfolgreich von Topic abgemeldet", allConnections.connections[index]);
                                    if (it->getSubscribers().empty()) {
                                        it = topics.erase(it);  // Lösche Topic wenn keine Subscriber mehr da sind
                                    } else {
                                        ++it;
                                    }
                                } else {
                                    ++it;
                                }
                            }
                        }                  
                    }
                    // Moechte der User Topics abbonieren?
                    startPos = message.find("$t");
                    // Filtere Topics heraus welche subscribed werden sollen.
                    topic_array.clear(); // leere die Liste die wir vorhin schon benutzt haben könnten
                    if (startPos != std::string::npos) {
                        startPos+=3; // Skippe Leerzeichen nach "$t"
                        size_t endPos = message.find(";", startPos);
                        while (endPos != std::string::npos) {
                            std::string topic = message.substr(startPos, endPos - startPos);
                            topic_array.push_back(topic);
                            startPos = endPos + 1;
                            endPos = message.find(";", startPos);
                        }
                        // Letztes Topic nach dem letzten semicolon
                        std::string lastTopic = message.substr(startPos);
                        topic_array.push_back(lastTopic);
                        std::string sendMsg;
                        for (auto filteredTopics: topic_array){
                                        // Wenn noch kein Topic existiert, dann erstelle ein Topic mit leerer Beschreibung
                                        if(topics.empty()){
                                            topics.emplace_back(filteredTopics, "");
                                            Topic& createdTopic = topics.back();
                                            createdTopic.subscribeTopic(allConnections.connections[index]);
                                            sendMsg += "\n-------\nTopic nicht vorhanden, das Topic \"" + filteredTopics + "\" wurde neu erstellt.\n";
                                        } else {
                                            bool topicExists = false;
                                            for (Topic& allTopics : topics) {
                                                if (filteredTopics == allTopics.getName()) {
                                                    // Wenn das Topic gefunden wurde, setze den Client auf die Subscriber-Liste
                                                    allTopics.subscribeTopic(allConnections.connections[index]);
                                                    updateTopic(allConnections, allTopics);
                                                    topicExists = true;
                                                    break; // Breche die Schleife ab, da das passende Topic gefunden wurde
                                                }
                                            }

                                            if (!topicExists) {
                                                // Wenn kein passendes Topic gefunden wurde, erstelle ein neues (leeres Topic)
                                                topics.emplace_back(filteredTopics, "");
                                                Topic& createdTopic = topics.back();
                                                createdTopic.subscribeTopic(allConnections.connections[index]);
                                                sendMsg += "\n-------\nTopic nicht vorhanden, das Topic \"" + filteredTopics + "\" wurde neu erstellt.\n";
                                            }
                                        }
                        } 
                        sendMessage(current_socket, sendMsg, allConnections.connections[index]);
               
                    }
                }
            }
            index++;
        }
}
CloseHandle(threadHandle);
printColoredText("\n-----\nSchliessen des Sockets...\n------\n", 32);
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
