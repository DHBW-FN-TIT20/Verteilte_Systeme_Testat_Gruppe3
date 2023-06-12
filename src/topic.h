#ifndef TOPIC_H
#define TOPIC_H

#include <string>
#include <iostream>
#include <vector>

enum class SubscribeResult {
    SUCCESS,
    INVALID_PARAMETERS,
    INTERNAL_ERROR
};

struct TopicStatus {
    std::time_t lastUpdated;
    std::vector<std::string> subscribers;
};

class Topic {
public:
    Topic(const std::string& name, const std::string& description);

    SubscribeResult subscribeTopic(const std::string& topicName);
    std::string unsubscribeTopic(const std::string& name);
    bool publishTopic(const std::string& message);
    TopicStatus getTopicStatus(const std::string& topicName);
    void updateTopic(const std::string& message, const std::time_t& timestamp);

private:
    std::string name;
    std::string description;
    std::time_t lastUpdated;
    std::vector<std::string> subscribers;
    // Add any additional private data members or helper functions here
};

#endif  // TOPIC_H
