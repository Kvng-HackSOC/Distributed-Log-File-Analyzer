#include "client/client.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <cstring>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <algorithm>

namespace fs = std::filesystem;

LogClient::LogClient() : socket_(INVALID_SOCKET_VALUE), connected_(false) {
}

LogClient::~LogClient() {
    disconnect();
}

bool LogClient::connect(const std::string& serverIP, int port) {
    if (connected_) {
        std::cerr << "Already connected to server" << std::endl;
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
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        cleanupWinsock();
#else
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
#endif
        return false;
    }
    
    // Prepare server address
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid server address: " << serverIP << std::endl;
#ifdef _WIN32
        closesocket(socket_);
        cleanupWinsock();
#else
        close(socket_);
#endif
        socket_ = INVALID_SOCKET_VALUE;
        return false;
    }
    
    // Connect to server
    if (::connect(socket_, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
#ifdef _WIN32
        std::cerr << "Error connecting to server: " << WSAGetLastError() << std::endl;
        closesocket(socket_);
        cleanupWinsock();
#else
        std::cerr << "Error connecting to server: " << strerror(errno) << std::endl;
        close(socket_);
#endif
        socket_ = INVALID_SOCKET_VALUE;
        return false;
    }
    
    connected_ = true;
    std::cout << "Connected to server at " << serverIP << ":" << port << std::endl;
    
    return true;
}

void LogClient::disconnect() {
    if (connected_ && socket_ != INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        closesocket(socket_);
        cleanupWinsock();
#else
        close(socket_);
#endif
        socket_ = INVALID_SOCKET_VALUE;
        connected_ = false;
        std::cout << "Disconnected from server" << std::endl;
    }
}

bool LogClient::sendRequest(const AnalysisRequest& request) {
    if (!connected_) {
        std::cerr << "Not connected to server" << std::endl;
        return false;
    }
    
    std::string requestStr = serializeRequest(request);
    return sendMessage(socket_, MSG_REQUEST, requestStr);
}

bool LogClient::sendLogFiles(const std::string& directory) {
    if (!connected_) {
        std::cerr << "Not connected to server" << std::endl;
        return false;
    }
    
    // Check if directory exists
    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        std::cerr << "Directory not found: " << directory << std::endl;
        return false;
    }
    
    // Get list of files in directory
    std::vector<std::string> files = listFilesInDirectory(directory);
    if (files.empty()) {
        std::cerr << "No files found in directory: " << directory << std::endl;
        return false;
    }
    
    // Send each file
    for (const auto& file : files) {
        if (!sendFile(file)) {
            std::cerr << "Failed to send file: " << file << std::endl;
            return false;
        }
    }
    
    // Signal end of file transfer
    if (!sendMessage(socket_, MSG_FILE_END, "")) {
        std::cerr << "Failed to signal end of file transfer" << std::endl;
        return false;
    }
    
    return true;
}

bool LogClient::sendFile(const std::string& filepath) {
    if (!connected_) {
        std::cerr << "Not connected to server" << std::endl;
        return false;
    }
    
    // Check if file exists
    if (!fs::exists(filepath) || !fs::is_regular_file(filepath)) {
        std::cerr << "File not found: " << filepath << std::endl;
        return false;
    }
    
    // Get file name (without path)
    std::string filename = fs::path(filepath).filename().string();
    
    // Signal start of file transfer
    if (!sendMessage(socket_, MSG_FILE_START, filename)) {
        std::cerr << "Failed to signal start of file transfer" << std::endl;
        return false;
    }
    
    // Open file
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << filepath << std::endl;
        return false;
    }
    
    // Send file in chunks
    char buffer[BUFFER_SIZE];
    while (!file.eof()) {
        file.read(buffer, BUFFER_SIZE);
        std::streamsize bytesRead = file.gcount();
        
        if (bytesRead > 0) {
            std::string chunk(buffer, bytesRead);
            if (!sendMessage(socket_, MSG_FILE_CHUNK, chunk)) {
                std::cerr << "Error sending file chunk" << std::endl;
                return false;
            }
        }
    }
    
    // Signal end of file
    if (!sendMessage(socket_, MSG_FILE_END, "")) {
        std::cerr << "Failed to signal end of file" << std::endl;
        return false;
    }
    
    // Wait for acknowledgment
    char type;
    std::string message;
    if (!receiveMessage(socket_, type, message) || type != MSG_ACK) {
        std::cerr << "Did not receive acknowledgment for file transfer" << std::endl;
        return false;
    }
    
    std::cout << "Sent file: " << filename << std::endl;
    return true;
}

bool LogClient::receiveResult(AnalysisResult& result) {
    if (!connected_) {
        std::cerr << "Not connected to server" << std::endl;
        return false;
    }
    
    char type;
    std::string message;
    
    if (!receiveMessage(socket_, type, message)) {
        std::cerr << "Error receiving message from server" << std::endl;
        return false;
    }
    
    if (type == MSG_RESULT) {
        result = deserializeResult(message);
        return true;
    }
    else if (type == MSG_ERROR) {
        std::cerr << "Server error: " << message << std::endl;
        return false;
    }
    else {
        std::cerr << "Unexpected message type: " << type << std::endl;
        return false;
    }
}

void LogClient::printResult(const AnalysisResult& result) {
    std::cout << "\n===== Analysis Results =====\n";
    std::cout << "Analysis type: " << analysisTypeToString(result.type) << "\n";
    std::cout << "Total log entries: " << result.totalEntries << "\n\n";
    
    std::cout << "Counts:\n";
    std::cout << std::setw(30) << std::left << "Key" << "Count\n";
    std::cout << std::string(40, '-') << "\n";
    
    // Find the maximum count for scaling (if we want to add a visual indicator)
    int maxCount = 0;
    for (const auto& pair : result.counts) {
        if (pair.second > maxCount) {
            maxCount = pair.second;
        }
    }
    
    // Create a vector of pairs for sorting
    std::vector<std::pair<std::string, int>> sortedCounts(result.counts.begin(), result.counts.end());
    
    // Sort by count (descending)
    std::sort(sortedCounts.begin(), sortedCounts.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Print sorted results
    for (const auto& pair : sortedCounts) {
        std::cout << std::setw(30) << std::left << pair.first << pair.second;
        
        // Add a simple visual indicator (20 chars max)
        int barLength = (pair.second * 20) / (maxCount > 0 ? maxCount : 1);
        std::cout << "  " << std::string(barLength, '#');
        
        std::cout << "\n";
    }
    
    std::cout << "===========================\n";
}

bool LogClient::saveResult(const AnalysisResult& result, const std::string& filename) {
    std::ofstream file(filename);
    if (!file) {
        std::cerr << "Error creating file: " << filename << std::endl;
        return false;
    }
    
    // Get current time for report header
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    file << "Log Analysis Report\n";
    file << "===================\n";
    file << "Generated: " << std::ctime(&time);
    file << "Analysis Type: " << analysisTypeToString(result.type) << "\n";
    file << "Total Log Entries: " << result.totalEntries << "\n\n";
    
    file << "Counts:\n";
    file << std::setw(30) << std::left << "Key" << "Count\n";
    file << std::string(40, '-') << "\n";
    
    // Create a vector of pairs for sorting
    std::vector<std::pair<std::string, int>> sortedCounts(result.counts.begin(), result.counts.end());
    
    // Sort by count (descending)
    std::sort(sortedCounts.begin(), sortedCounts.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Print sorted results
    for (const auto& pair : sortedCounts) {
        file << std::setw(30) << std::left << pair.first << pair.second << "\n";
    }
    
    file << "\nEnd of Report\n";
    
    std::cout << "Results saved to: " << filename << std::endl;
    return true;
}

std::string getRandomClientFolder(const std::string& basePath) {
    // Create a list of potential client folders
    std::vector<std::string> clientFolders;
    
    // Check if the base path exists and is a directory
    if (fs::exists(basePath) && fs::is_directory(basePath)) {
        for (const auto& entry : fs::directory_iterator(basePath)) {
            if (fs::is_directory(entry) && 
                entry.path().filename().string().find("client") != std::string::npos) {
                clientFolders.push_back(entry.path().string());
            }
        }
    }
    
    // If no client folders found, return empty string
    if (clientFolders.empty()) {
        return "";
    }
    
    // Select a random client folder
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, clientFolders.size() - 1);
    
    return clientFolders[distrib(gen)];
}

std::vector<std::string> listFilesInDirectory(const std::string& directory) {
    std::vector<std::string> files;
    
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (fs::is_regular_file(entry)) {
            // Only include files with expected extensions
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            if (ext == ".json" || ext == ".xml" || ext == ".txt") {
                files.push_back(entry.path().string());
            }
        }
    }
    
    return files;
}