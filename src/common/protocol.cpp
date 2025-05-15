#include "protocol.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <chrono>

#ifdef _WIN32
bool initializeWinsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }
    return true;
}

void cleanupWinsock() {
    WSACleanup();
}
#endif

bool sendMessage(socket_t socket, char type, const std::string& message) {
    // Format: [TYPE:1][LENGTH:8][MESSAGE:LENGTH]
    std::string header;
    header.push_back(type);
    
    // Add message length as 8 hex characters
    std::stringstream ss;
    ss << std::setw(8) << std::setfill('0') << std::hex << message.size();
    header += ss.str();
    
    // Send header
    int sent = send(socket, header.c_str(), 9, 0);
    if (sent != 9) {
        return false;
    }
    
    // Send message in chunks if it's not empty
    if (!message.empty()) {
        size_t totalSent = 0;
        while (totalSent < message.size()) {
            size_t remaining = message.size() - totalSent;
            size_t chunkSize = std::min(remaining, static_cast<size_t>(BUFFER_SIZE));
            
            sent = send(socket, message.c_str() + totalSent, static_cast<int>(chunkSize), 0);
            if (sent <= 0) {
                return false;
            }
            
            totalSent += sent;
        }
    }
    
    return true;
}

bool receiveMessage(socket_t socket, char& type, std::string& message) {
    // Receive header: [TYPE:1][LENGTH:8]
    char header[10] = {0};
    int received = 0;
    size_t totalReceived = 0;
    
    // Keep receiving until we have the full header
    while (totalReceived < 9) {
        received = recv(socket, header + totalReceived, 9 - static_cast<int>(totalReceived), 0);
        if (received <= 0) {
            return false;
        }
        totalReceived += received;
    }
    
    // Extract type and length
    type = header[0];
    header[9] = '\0';
    std::string lengthStr(header + 1, 8);
    unsigned long length = std::stoul(lengthStr, nullptr, 16);
    
    // Prepare message buffer
    message.clear();
    message.reserve(length);
    
    // Receive message in chunks
    char buffer[BUFFER_SIZE];
    totalReceived = 0;
    
    while (totalReceived < length) {
        size_t remaining = length - totalReceived;
        size_t chunkSize = std::min(remaining, static_cast<size_t>(BUFFER_SIZE - 1));
        
        received = recv(socket, buffer, static_cast<int>(chunkSize), 0);
        if (received <= 0) {
            return false;
        }
        
        buffer[received] = '\0';
        message.append(buffer, received);
        totalReceived += received;
    }
    
    return true;
}

std::string serializeRequest(const AnalysisRequest& request) {
    std::stringstream ss;
    ss << analysisTypeToString(request.type) << "|"
       << (request.startDate.has_value() ? request.startDate.value() : "NONE") << "|"
       << (request.endDate.has_value() ? request.endDate.value() : "NONE");
    return ss.str();
}

AnalysisRequest deserializeRequest(const std::string& data) {
    AnalysisRequest request;
    std::stringstream ss(data);
    std::string typeStr, startDate, endDate;
    
    std::getline(ss, typeStr, '|');
    std::getline(ss, startDate, '|');
    std::getline(ss, endDate, '|');
    
    request.type = stringToAnalysisType(typeStr);
    
    if (startDate != "NONE") {
        request.startDate = startDate;
    }
    
    if (endDate != "NONE") {
        request.endDate = endDate;
    }
    
    return request;
}

std::string serializeResult(const AnalysisResult& result) {
    std::stringstream ss;
    ss << analysisTypeToString(result.type) << "|"
       << result.totalEntries << "|"
       << result.counts.size();
    
    for (const auto& pair : result.counts) {
        ss << "|" << pair.first << "|" << pair.second;
    }
    
    return ss.str();
}

AnalysisResult deserializeResult(const std::string& data) {
    AnalysisResult result;
    std::stringstream ss(data);
    std::string typeStr, token;
    size_t countSize;
    
    std::getline(ss, typeStr, '|');
    result.type = stringToAnalysisType(typeStr);
    
    std::getline(ss, token, '|');
    result.totalEntries = std::stoi(token);
    
    std::getline(ss, token, '|');
    countSize = std::stoull(token);
    
    for (size_t i = 0; i < countSize; ++i) {
        std::string key, valueStr;
        std::getline(ss, key, '|');
        std::getline(ss, valueStr, '|');
        int value = std::stoi(valueStr);
        result.counts[key] = value;
    }
    
    return result;
}

bool isDateInRange(const std::string& date,
                  const std::optional<std::string>& startDate,
                  const std::optional<std::string>& endDate) {
    // If no date range specified, include all
    if (!startDate.has_value() && !endDate.has_value()) {
        return true;
    }
    
    // Simple string comparison (assumes YYYY-MM-DD format)
    if (startDate.has_value() && date < startDate.value()) {
        return false;
    }
    
    if (endDate.has_value() && date > endDate.value()) {
        return false;
    }
    
    return true;
}

AnalysisType stringToAnalysisType(const std::string& typeStr) {
    if (typeStr == "USER") return AnalysisType::USER;
    if (typeStr == "IP") return AnalysisType::IP;
    if (typeStr == "LOG_LEVEL") return AnalysisType::LOG_LEVEL;
    
    // Default
    return AnalysisType::USER;
}

std::string analysisTypeToString(AnalysisType type) {
    switch (type) {
        case AnalysisType::USER: return "USER";
        case AnalysisType::IP: return "IP";
        case AnalysisType::LOG_LEVEL: return "LOG_LEVEL";
        default: return "UNKNOWN";
    }
}
