#include <iostream>
#include <cstring>
#include <winsock2.h>

const int BUFFER_SIZE = 1024;
const int PORT = 12345;

int main() {
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
