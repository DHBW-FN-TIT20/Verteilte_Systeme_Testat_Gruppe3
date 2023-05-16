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



// ----------------------------------------------------
// SECTION SUBSCRIBE TOPIC
// ----------------------------------------------------
enum class SubscribeResult {
    SUCCESS,
    INVALID_PARAMETERS,
    INTERNAL_ERROR
};

SubscribeResult subscribeTopic(const std::string& topicName) {
    // Überprüfe die Gültigkeit der Parameter
    if (topicName.empty()) {
        return SubscribeResult::INVALID_PARAMETERS;
    }

    // Hier erfolgt die Logik zur Registrierung des Subscribers auf das Topic
    // Überprüfung, ob das Topic bereits existiert oder erstellt werden muss
    // Registrierung des Subscribers auf das Topic

    // Beispiel: Annahme, dass die Registrierung erfolgreich ist
    return SubscribeResult::SUCCESS;
}

// ----------------------------------------------------
// SECTION UNSUBSCRIBE TOPIC
// ----------------------------------------------------
std::string unsubscribeTopic(const std::string& name) {
    // Implementiere hier die Logik zur Abmeldung des Subscribers vom Topic
    // Überprüfe die Parameter, prüfe die Existenz des Topics und entferne die Registrierung
    // Gib entsprechende Rückgabewerte basierend auf dem Ergebnis zurück

    // Beispielcode:
    if (name.empty()) {
        return "Ungültige Parameter";
    }

    // Überprüfe, ob das Topic existiert und der Subscriber registriert ist
    //bool topicExists = /* Prüfe, ob das Topic existiert */;
    //bool registrationExists = /* Prüfe, ob der Subscriber registriert ist */;

    //if (!topicExists || !registrationExists) {
    //    return "Topic/Registrierung existiert nicht";
    //}

    // Führe die Abmeldung durch
    // ...

    return "Erfolgreich von Topic abgemeldet";
}

// ----------------------------------------------------
// SECTION PUBLISH TOPIC
// ----------------------------------------------------
// Funktion zum Publizieren einer Nachricht auf dem Topic
bool publishTopic(const std::string& topicName, const std::string& message) {
    // Überprüfen, ob das Topic existiert
    //if (!topicExists(topicName)) {
    //    std::cout << "Topic existiert nicht" << std::endl;
    //    return false;
    //}

    // Überprüfen der Parameter
    if (topicName.empty() || message.empty()) {
        std::cout << "Ungültige Parameter" << std::endl;
        return false;
    }

    // Weitere interne Logik zur Aktualisierung des Topics mit der neuen Nachricht
    // ...

    std::cout << "Topic erfolgreich aktualisiert" << std::endl;
    return true;
}

// ----------------------------------------------------
// SECTION LIST TOPICS
// ----------------------------------------------------
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

struct TopicStatus {
    std::time_t lastUpdated;
    std::vector<std::string> subscribers;
};

// ----------------------------------------------------
// SECTION GET TOPIC STATUS
// ----------------------------------------------------
// Funktion, um den aktuellen Status eines Topics abzurufen
TopicStatus getTopicStatus(const std::string& topicName) {
    TopicStatus status;

    // Überprüfen, ob das Topic existiert
    std::vector<std::string> topics = listTopics(); // Annahme: Die Funktion listTopics() gibt eine Liste der verfügbaren Topics zurück

    bool topicExists = false;
    for (const auto& topic : topics) {
        if (topic == topicName) {
            topicExists = true;
            break;
        }
    }

    if (!topicExists) {
        // Topic existiert nicht
        std::cout << "Topic existiert nicht" << std::endl;
        return status;
    }

    // Hier weiter mit der Implementierung, um den Status des Topics abzurufen
    // status.lastUpdated = ... // Setze den Zeitstempel der Aktualisierung
    // status.subscribers = ... // Setze die Liste der Subscriber

    return status;
}

// Struktur, um Informationen zu einem Topic zu speichern
struct Topic {
    std::string name;
    std::string message;
    std::string timestamp;
    std::vector<std::string> subscribers;
};

// Globale Map zum Speichern der Topics
std::map<std::string, Topic> topics;

// Funktion zum Übermitteln des Topic-Inhalts an alle Subscriber
void updateTopic(const std::string& name, const std::string& message, const std::string& timestamp) {
    // Überprüfen, ob das Topic existiert
    auto it = topics.find(name);
    if (it == topics.end()) {
        std::cout << "Topic '" << name << "' existiert nicht" << std::endl;
        return;
    }

    // Aktualisieren der Topic-Nachricht und des Zeitstempels
    it->second.message = message;
    it->second.timestamp = timestamp;

    // Übermitteln der Nachricht an alle Subscriber
    std::cout << "Update für Topic '" << name << "' wird an alle Subscriber gesendet" << std::endl;
    for (const std::string& subscriber : it->second.subscribers) {
        std::cout << "Nachricht an Subscriber '" << subscriber << "': " << message << std::endl;
    }
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
