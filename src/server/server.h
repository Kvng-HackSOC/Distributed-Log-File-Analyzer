#ifndef SERVER_H
#define SERVER_H

#include "common/protocol.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

class LogServer {
public:
    LogServer(int port = DEFAULT_PORT);
    ~LogServer();
    
    // Start the server
    bool start();
    
    // Stop the server
    void stop();
    
    // Check if server is running
    bool isRunning() const;
    
private:
    // Main server thread function
    void serverThread();
    
    // Client handler thread function
    void clientHandler(socket_t clientSocket);
    
    // Handle client request
    bool handleRequest(socket_t clientSocket, const AnalysisRequest& request);
    
    // Handle file transfer
    bool handleFileTransfer(socket_t clientSocket, const std::string& tempDir);
    
    // Process received log files
    AnalysisResult processLogFiles(const AnalysisRequest& request, const std::string& tempDir);
    
    // Server state
    int port_;
    socket_t serverSocket_;
    std::atomic<bool> running_;
    std::thread serverThreadHandle_;
    
    // Active client connections
    std::vector<std::thread> clientThreads_;
    std::mutex clientThreadsMutex_;
};

#endif // SERVER_H