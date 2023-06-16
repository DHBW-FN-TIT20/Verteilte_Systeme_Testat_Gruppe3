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
    std::time_t lastUpdated;
    std::vector<sockaddr_in> subscribers;
};

class Topic {
public:
    Topic(const std::string& name, const std::string& description)
        : name(name), description(description), topicStatus({getCurrentTime(), {}}) {}
    std::string subscribeTopic(sockaddr_in subscriber);
    void unsubscribeTopic(const sockaddr_in& subscriber);
    std::string publishTopic(const std::string& message);
    std::string getName() const{return name;}
    std::string getDescription() const{return description;}
    TopicStatus getTopicStatus() const{return topicStatus;}

private:
    std::string name;
    std::string description;
    TopicStatus topicStatus;
    std::time_t getCurrentTime() {
        auto currentTime = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(currentTime);
        return time;
    }
};

#endif  // TOPIC_H
