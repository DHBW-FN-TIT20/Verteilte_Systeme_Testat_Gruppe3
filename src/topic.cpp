#include "topic.h"



std::string Topic::subscribeTopic(sockaddr_in subscriber) {
    topicStatus.subscribers.push_back(subscriber);
    std::cout<<"Client mit Port: " << ntohs(subscriber.sin_port) << " wurde erfolgreich zur sublist hinzugefuegt\n-----\n";
    std::cout<<"Liste von Subs: ";
    for(auto subs: topicStatus.subscribers){

        std::cout<< " - " << ntohs(subs.sin_port);
    }
    return "Erfolgreich subscribed!";
}


void Topic::unsubscribeTopic(const sockaddr_in& subscriber) {
    // Verwendung von std::remove_if und std::erase, um das Element zu entfernen
    topicStatus.subscribers.erase(std::remove_if(topicStatus.subscribers.begin(), topicStatus.subscribers.end(),
                                                 [&subscriber](const sockaddr_in& elem) {
                                                     return memcmp(&elem, &subscriber, sizeof(sockaddr_in)) == 0;
                                                 }),
                                  topicStatus.subscribers.end());
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
    this->topicStatus.lastUpdated = getCurrentTime();
    
    returnString = "Topic erfolgreich aktualisiert";
    return returnString;
}
