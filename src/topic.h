#ifndef TOPIC_H
#define TOPIC_H

#include <string>
#include <iostream>
#include <vector>
#include <ctime>

enum class SubscribeResult {
    SUCCESS,
    INVALID_PARAMETERS,
    INTERNAL_ERROR
};

struct TopicStatus {
    std::time_t lastUpdated;
    std::vector<int> subscribers;
};

class Topic {
public:
    Topic(const std::string& name, const std::string& description) : name(name), description(description) {}

    SubscribeResult subscribeTopic(const std::string& topicName);
    std::string unsubscribeTopic(const std::string& name);
    void publishTopic(const std::string& message);
    TopicStatus getTopicStatus(const std::string& topicName);
    void updateTopic(const std::string& message, const std::time_t& timestamp);
    std::string getName() const{return name;}
    std::string getDescription() const{return description;}
    TopicStatus getTopicStatus() const{return topicStatus;}

private:
    std::string name;
    std::string description;
    TopicStatus topicStatus;
    // Add any additional private data members or helper functions here
};

#endif  // TOPIC_H
