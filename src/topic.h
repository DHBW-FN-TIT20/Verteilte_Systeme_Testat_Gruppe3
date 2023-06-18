#ifndef TOPIC_H
#define TOPIC_H

#include <string>
#include <iostream>
#include <vector>
#include <ctime>
#include <winsock2.h> // wir speichern eine Liste von Socket-Addressen
#include <algorithm>
#include <iterator>
#include <chrono> // Zeit
#include <ctime> // Zeit in C



struct TopicStatus {

};

// Klasse zum managen von Topics
class Topic {
public:
    Topic(const std::string& name, const std::string& description)
        : name(name), description(description) {
            lastUpdated = getCurrentTime();
        }
    std::string subscribeTopic(sockaddr_in subscriber);
    void unsubscribeTopic(const sockaddr_in& subscriber);
    std::string publishTopic(const std::string& message);
    std::string getName() const{return name;}
    std::string getDescription() const{return description;}
    std::vector<sockaddr_in> getSubscribers() const{return subscribers;}
    std::time_t getLastUpdated() const{return lastUpdated;}
    void updateTime() {    this->lastUpdated = getCurrentTime();}

private:
    std::string name;
    std::string description;
    std::time_t lastUpdated;
    std::vector<sockaddr_in> subscribers;
    std::time_t getCurrentTime() {
        auto currentTime = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(currentTime);
        return time;
    }
};

#endif  // TOPIC_H
