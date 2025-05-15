#include "server/server.h"
#include "server/analyzer.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <filesystem>

#ifdef _WIN32
#include <direct.h>
#define mkdir(dir, mode) _mkdir(dir)
#endif

namespace fs = std::filesystem;

LogServer::LogServer(int port)
    : port_(port), serverSocket_(INVALID_SOCKET_VALUE), running_(false) {
}

LogServer::~LogServer() {
    stop();
}

bool LogServer::start() {
    if (running_) {
        std::cerr << "Server is already running" << std::endl;
        return false;
    }
    
#ifdef _WIN32
    // Initialize Winsock
    if (!initializeWinsock()) {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return false;
    }
#endif
    
    // Create socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket_ == INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        cleanupWinsock();
#else
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
#endif
        return false;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
#ifdef _WIN32
        std::cerr << "Error setting socket options: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket_);
        cleanupWinsock();
#else
        std::cerr << "Error setting socket options: " << strerror(errno) << std::endl;
        close(serverSocket_);
#endif
        return false;
    }
    
    // Bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr *)&address, sizeof(address)) < 0) {
#ifdef _WIN32
        std::cerr << "Error binding socket: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket_);
        cleanupWinsock();
#else
        std::cerr << "Error binding socket: " << strerror(errno) << std::endl;
        close(serverSocket_);
#endif
        return false;
    }
    
    // Listen for connections
    if (listen(serverSocket_, 5) < 0) {
#ifdef _WIN32
        std::cerr << "Error listening on socket: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket_);
        cleanupWinsock();
#else
        std::cerr << "Error listening on socket: " << strerror(errno) << std::endl;
        close(serverSocket_);
#endif
        return false;
    }
    
    // Start server thread
    running_ = true;
    serverThreadHandle_ = std::thread(&LogServer::serverThread, this);
    
    std::cout << "Server started on port " << port_ << std::endl;
    return true;
}

void LogServer::stop() {
    if (!running_) {
        return;
    }
    
    // Set running flag to false
    running_ = false;
    
    // Close server socket to interrupt accept()
    if (serverSocket_ != INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        closesocket(serverSocket_);
#else
        close(serverSocket_);
#endif
        serverSocket_ = INVALID_SOCKET_VALUE;
    }
    
    // Wait for server thread to finish
    if (serverThreadHandle_.joinable()) {
        serverThreadHandle_.join();
    }
    
    // Wait for client threads to finish
    {
        std::lock_guard<std::mutex> lock(clientThreadsMutex_);
        for (auto& thread : clientThreads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        clientThreads_.clear();
    }
    
#ifdef _WIN32
    cleanupWinsock();
#endif
    
    std::cout << "Server stopped" << std::endl;
}

bool LogServer::isRunning() const {
    return running_;
}

void LogServer::serverThread() {
    std::cout << "Server thread started" << std::endl;
    
    while (running_) {
        // Accept client connection
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        socket_t clientSocket = accept(serverSocket_, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET_VALUE) {
            if (running_) {  // Only report error if still running
#ifdef _WIN32
                std::cerr << "Error accepting connection: " << WSAGetLastError() << std::endl;
#else
                std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
#endif
            }
            break;
        }
        
        // Get client information
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        std::cout << "New connection from " << clientIP << ":" << ntohs(clientAddr.sin_port) << std::endl;
        
        // Create a new thread to handle client
        {
            std::lock_guard<std::mutex> lock(clientThreadsMutex_);
            clientThreads_.emplace_back(&LogServer::clientHandler, this, clientSocket);
        }
    }
    
    std::cout << "Server thread finished" << std::endl;
}

void LogServer::clientHandler(socket_t clientSocket) {
    // Create a temporary directory for this client
    std::string tempDir = std::string("temp_") + std::to_string(clientSocket);
    fs::create_directory(tempDir);
    
    try {
        char type;
        std::string message;
        
        // Receive request
        if (receiveMessage(clientSocket, type, message) && type == MSG_REQUEST) {
            AnalysisRequest request = deserializeRequest(message);
            
            // Handle the request
            handleRequest(clientSocket, request);
        }
        else {
            std::cerr << "Invalid initial message from client" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error handling client: " << e.what() << std::endl;
        sendMessage(clientSocket, MSG_ERROR, "Server error: " + std::string(e.what()));
    }
    
    // Clean up temporary directory
    try {
        fs::remove_all(tempDir);
    }
    catch (const std::exception& e) {
        std::cerr << "Error cleaning up temp directory: " << e.what() << std::endl;
    }
    
    // Close client socket
#ifdef _WIN32
    closesocket(clientSocket);
#else
    close(clientSocket);
#endif
    std::cout << "Client disconnected" << std::endl;
}

bool LogServer::handleRequest(socket_t clientSocket, const AnalysisRequest& request) {
    std::cout << "Received analysis request: " << analysisTypeToString(request.type) << std::endl;
    
    // Create temporary directory for this request
    std::string tempDir = std::string("temp_") + std::to_string(clientSocket);
    if (!fs::exists(tempDir)) {
        fs::create_directory(tempDir);
    }
    
    // Handle file transfer
    if (!handleFileTransfer(clientSocket, tempDir)) {
        return false;
    }
    
    // Process log files
    std::cout << "Processing log files..." << std::endl;
    AnalysisResult result = processLogFiles(request, tempDir);
    
    // Send result back to client
    std::string resultStr = serializeResult(result);
    if (!sendMessage(clientSocket, MSG_RESULT, resultStr)) {
        std::cerr << "Failed to send result to client" << std::endl;
        return false;
    }
    
    std::cout << "Analysis completed and sent to client" << std::endl;
    return true;
}

bool LogServer::handleFileTransfer(socket_t clientSocket, const std::string& tempDir) {
    char type;
    std::string message;
    int fileCount = 0;
    
    // Receive files until MSG_FILE_END
    while (true) {
        if (!receiveMessage(clientSocket, type, message)) {
            std::cerr << "Error receiving message from client" << std::endl;
            return false;
        }
        
        if (type == MSG_FILE_END) {
            break; // End of file transfer
        }
        else if (type == MSG_FILE_START) {
            // Start of a new file
            std::string fileName = message;
            std::cout << "Receiving file: " << fileName << std::endl;
            
            // Create file path
            std::string filePath = tempDir + "/" + fileName;
            
            // Open file for writing
            std::ofstream file(filePath, std::ios::binary);
            if (!file) {
                std::cerr << "Error creating file: " << filePath << std::endl;
                sendMessage(clientSocket, MSG_ERROR, "Error creating file");
                return false;
            }
            
            // Receive file chunks
            while (true) {
                if (!receiveMessage(clientSocket, type, message)) {
                    std::cerr << "Error receiving file chunk" << std::endl;
                    return false;
                }
                
                if (type == MSG_FILE_CHUNK) {
                    // Write chunk to file
                    file.write(message.data(), message.size());
                }
                else if (type == MSG_FILE_END) {
                    // End of this file
                    file.close();
                    fileCount++;
                    sendMessage(clientSocket, MSG_ACK, "File received");
                    break;
                }
                else {
                    std::cerr << "Unexpected message type during file transfer: " << type << std::endl;
                    return false;
                }
            }
        }
        else {
            std::cerr << "Unexpected message type: " << type << std::endl;
            return false;
        }
    }
    
    std::cout << "Received " << fileCount << " files" << std::endl;
    return true;
}

AnalysisResult LogServer::processLogFiles(const AnalysisRequest& request, const std::string& tempDir) {
    // Get list of all files in temporary directory
    std::vector<std::string> logFiles;
    for (const auto& entry : fs::directory_iterator(tempDir)) {
        logFiles.push_back(entry.path().string());
    }
    
    // Create analyzer and process files
    LogAnalyzer analyzer(request);
    return analyzer.analyze(logFiles);
}