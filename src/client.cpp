#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <tclap/CmdLine.h>

const int BUFFER_SIZE = 1024;
const int PORT = 12345;

int main(int argc, char** argv) {

    TCLAP::CmdLine cmd("Command description message", ' ', "0.9");
    TCLAP::SwitchArg listArg ("l", "list", "List all topics", false);
    TCLAP::ValueArg<std::string> portArg("", "port", "Port of the server: ", false, "", "string");
    TCLAP::ValueArg<std::string> topicArg("", "topic", "Topic to subscribe to", false, "", "string");
    TCLAP::MultiArg<std::string> publishArg("p", "publish", "Publish a message", false, "string");


    cmd.add(portArg);
    cmd.add(topicArg);
    cmd.add(publishArg);
    cmd.add(listArg);
    cmd.parse(argc, argv);

    //use port for every connection
    if (topicArg.isSet())
    {
        // subscribe to topic
        // t: Topic1,Topic2 etc.
    } else if (publishArg.isSet())
    {
        // publish message
        // t: Topic1 m: Message
    } else if (listArg.isSet())
    {
        // list all topics
        // Message to server: list all
    } else
    {
        std::cout << "No command specified" << std::endl;
    }
    exit(0);
    



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

    // Server-Adresse und Port festlegen
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");  // IP-Adresse des Servers hier eintragen
    serverAddress.sin_port = htons(PORT);

    // Verbindung zum Server herstellen
    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Fehler beim Herstellen der Verbindung: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Daten senden
    const char* message = "Hello, Server!";
    if (send(clientSocket, message, strlen(message), 0) == SOCKET_ERROR) {
        std::cerr << "Fehler beim Senden der Daten: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Antwort des Servers empfangen
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
    if (bytesRead == SOCKET_ERROR) {
        std::cerr << "Fehler beim Empfangen der Daten: " << WSAGetLastError() << std::endl;
    } else {
        std::cout << "Antwort vom Server: " << buffer << std::endl;
    }

    // Socket schließen und Winsock aufräumen
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}

