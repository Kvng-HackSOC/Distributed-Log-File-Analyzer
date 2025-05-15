#include "server/server.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>

// Global server instance for signal handler
LogServer* gServer = nullptr;

// Signal handler
void signalHandler(int signal) {
    std::cout << "Received signal " << signal << std::endl;
    if (gServer) {
        gServer->stop();
    }
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [port]" << std::endl;
    std::cout << "  port - Optional server port (default: " << DEFAULT_PORT << ")" << std::endl;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    int port = DEFAULT_PORT;
    
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
            if (port <= 0 || port > 65535) {
                std::cerr << "Error: Port must be between 1 and 65535" << std::endl;
                printUsage(argv[0]);
                return 1;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error parsing port: " << e.what() << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Set up signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Create and start server
    LogServer server(port);
    gServer = &server;
    
    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    std::cout << "Server started on port " << port << std::endl;
    std::cout << "Press Ctrl+C to stop the server" << std::endl;
    
    // Wait for server to stop
    while (server.isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    gServer = nullptr;
    return 0;
}