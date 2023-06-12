#include "topic.h"

SubscribeResult Topic::subscribeTopic(const std::string& topicName) {
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

Topic::Topic(const std::string& name, const std::string& description){

}

std::string Topic::unsubscribeTopic(const std::string& name) {
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

// Funktion zum Publizieren einer Nachricht auf dem Topic
bool publishTopic(const std::string& message) {
    // Überprüfen, ob das Topic existiert
    //if (!topicExists(topicName)) {
    //    std::cout << "Topic existiert nicht" << std::endl;
    //    return false;
    //}

    // Überprüfen der Parameter
    if (message.empty()) {
        std::cout << "Ungültige Parameter" << std::endl;
        return false;
    }
    // Weitere interne Logik zur Aktualisierung des Topics mit der neuen Nachricht
    // ...
    std::cout << "Topic erfolgreich aktualisiert" << std::endl;
    return true;
}

// Funktion zum Publizieren einer Nachricht auf dem Topic
bool Topic::publishTopic(const std::string& message) {
    // Überprüfen, ob das Topic existiert
    //if (!topicExists(topicName)) {
    //    std::cout << "Topic existiert nicht" << std::endl;
    //    return false;
    //}

    // Überprüfen der Parameter
    if (message.empty()) {
        std::cout << "Ungültige Parameter" << std::endl;
        return false;
    }
    // Weitere interne Logik zur Aktualisierung des Topics mit der neuen Nachricht
    // ...
    std::cout << "Topic erfolgreich aktualisiert" << std::endl;
    return true;
}

// Funktion, um den aktuellen Status eines Topics abzurufen
TopicStatus Topic::getTopicStatus(const std::string& topicName) {
    TopicStatus status;

    // Hier weiter mit der Implementierung, um den Status des Topics abzurufen
    // status.lastUpdated = ... // Setze den Zeitstempel der Aktualisierung
    // status.subscribers = ... // Setze die Liste der Subscriber

    return status;
}

// Funktion zum Übermitteln des Topic-Inhalts an alle Subscriber
void Topic::updateTopic(const std::string& message, const std::time_t& timestamp) {

    // Aktualisieren der Topic-Nachricht und des Zeitstempels
    this->description = message;
    this->lastUpdated = timestamp;

    // Übermitteln der Nachricht an alle Subscriber
    std::cout << "Update für Topic '" << "' wird an alle Subscriber gesendet" << std::endl;
    for (const std::string& subscriber : this->subscribers) {
        std::cout << "Nachricht an Subscriber '" << subscriber << "': " << message << std::endl;
    }
}