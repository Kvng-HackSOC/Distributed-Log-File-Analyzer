#ifndef CLIENT_H
#define CLIENT_H

#include "common/protocol.h"
#include <string>
#include <vector>
#include <filesystem>

class LogClient {
public:
    LogClient();
    ~LogClient();
    
    // Connect to server
    bool connect(const std::string& serverIP, int port = DEFAULT_PORT);
    
    // Disconnect from server
    void disconnect();
    
    // Send analysis request
    bool sendRequest(const AnalysisRequest& request);
    
    // Send log files from a directory
    bool sendLogFiles(const std::string& directory);
    
    // Get analysis result
    bool receiveResult(AnalysisResult& result);
    
    // Print analysis result
    void printResult(const AnalysisResult& result);
    
    // Save result to file
    bool saveResult(const AnalysisResult& result, const std::string& filename);
    
private:
    // Helper to send a single file
    bool sendFile(const std::string& filepath);
    
    // Socket for server connection
    socket_t socket_;
    
    // Connected state
    bool connected_;
};

// Utility function to get random client folder
std::string getRandomClientFolder(const std::string& basePath);

// Utility function to list all files in a directory
std::vector<std::string> listFilesInDirectory(const std::string& directory);

#endif // CLIENT_H