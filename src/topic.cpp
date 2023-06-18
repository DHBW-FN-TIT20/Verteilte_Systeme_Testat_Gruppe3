#include "topic.h"


// Subscribed einen User auf das Topic / setzt ihn auf die Liste (Vector)
std::string Topic::subscribeTopic(sockaddr_in subscriber) {
    subscribers.push_back(subscriber);
    std::cout<<"\n-----\nClient mit Port: " << ntohs(subscriber.sin_port) << " wurde erfolgreich zur sublist hinzugefuegt\n-----\n";
    return "Erfolgreich subscribed!";
}

// Unsubscribed einen User vom Topic / streicht ihn von der Liste (Vector)
void Topic::unsubscribeTopic(const sockaddr_in& subscriber) {
    // Verwendung von std::remove_if und std::erase, um das Element zu entfernen
    subscribers.erase(std::remove_if(subscribers.begin(), subscribers.end(),
                                                 [&subscriber](const sockaddr_in& elem) {
                                                     return memcmp(&elem, &subscriber, sizeof(sockaddr_in)) == 0;
                                                 }),
                                  subscribers.end());
}


// Funktion zum Publizieren einer Nachricht auf dem Topic
std::string Topic::publishTopic(const std::string& message) {

    std::string returnString;
    // Überprüfen der Parameter
    if (message.empty()) {
        returnString = "Ungültige Parameter";
        return returnString;
    }

    this->description = message;
    this->lastUpdated = getCurrentTime();
    
    returnString = "Topic erfolgreich aktualisiert";
    return returnString;
}

