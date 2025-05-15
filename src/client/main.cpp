#include "client/client.h"
#include <iostream>
#include <string>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <server_ip> <analysis_type> [log_directory] [start_date] [end_date] [output_file]\n";
    std::cout << "  server_ip     - IP address of the log analysis server\n";
    std::cout << "  analysis_type - Type of analysis to perform (user|ip|log_level)\n";
    std::cout << "  log_directory - Optional directory containing log files (default: auto-select a client folder)\n";
    std::cout << "  start_date    - Optional start date for analysis (YYYY-MM-DD)\n";
    std::cout << "  end_date      - Optional end date for analysis (YYYY-MM-DD)\n";
    std::cout << "  output_file   - Optional file to save results (default: do not save)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " 127.0.0.1 user\n";
    std::cout << "  " << programName << " 127.0.0.1 ip test_logs/client1 2023-01-01 2023-12-31\n";
    std::cout << "  " << programName << " 127.0.0.1 log_level test_logs/client2 \"\" \"\" results.txt\n";
}

AnalysisType parseAnalysisType(const std::string& typeStr) {
    std::string typeLower = typeStr;
    std::transform(typeLower.begin(), typeLower.end(), typeLower.begin(), ::tolower);
    
    if (typeLower == "user") return AnalysisType::USER;
    if (typeLower == "ip") return AnalysisType::IP;
    if (typeLower == "log_level") return AnalysisType::LOG_LEVEL;
    
    throw std::runtime_error("Invalid analysis type: " + typeStr);
}

int main(int argc, char* argv[]) {
    // Check minimum required arguments
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }
    
    try {
        // Parse command line arguments
        std::string serverIP = argv[1];
        AnalysisType analysisType = parseAnalysisType(argv[2]);
        
        // Optional parameters
        std::string logDirectory;
        std::optional<std::string> startDate;
        std::optional<std::string> endDate;
        std::optional<std::string> outputFile;
        
        // Determine log directory (auto-select or user-specified)
        if (argc > 3 && argv[3][0] != '\0') {
            logDirectory = argv[3];
        } else {
            // Try to auto-select a client folder from test_logs
            logDirectory = getRandomClientFolder("test_logs");
            if (logDirectory.empty()) {
                std::cerr << "Error: Could not find any client folders in test_logs\n";
                std::cerr << "Please specify a log directory explicitly\n";
                return 1;
            }
        }
        
        // Parse date range if provided
        if (argc > 4 && argv[4][0] != '\0') {
            startDate = argv[4];
        }
        
        if (argc > 5 && argv[5][0] != '\0') {
            endDate = argv[5];
        }
        
        // Parse output file if provided
        if (argc > 6 && argv[6][0] != '\0') {
            outputFile = argv[6];
        }
        
        // Check if log directory exists
        if (!fs::exists(logDirectory) || !fs::is_directory(logDirectory)) {
            std::cerr << "Error: Log directory not found: " << logDirectory << std::endl;
            return 1;
        }
        
        // Create analysis request
        AnalysisRequest request;
        request.type = analysisType;
        request.startDate = startDate;
        request.endDate = endDate;
        
        // Create client and connect to server
        LogClient client;
        if (!client.connect(serverIP)) {
            return 1;
        }
        
        // Send analysis request
        std::cout << "Sending analysis request: " << analysisTypeToString(analysisType) << std::endl;
        if (!client.sendRequest(request)) {
            return 1;
        }
        
        // Send log files
        std::cout << "Sending log files from directory: " << logDirectory << std::endl;
        if (!client.sendLogFiles(logDirectory)) {
            return 1;
        }
        
        // Receive analysis result
        AnalysisResult result;
        std::cout << "Waiting for analysis results..." << std::endl;
        if (!client.receiveResult(result)) {
            return 1;
        }
        
        // Print result
        client.printResult(result);
        
        // Save result to file if specified
        if (outputFile.has_value()) {
            client.saveResult(result, outputFile.value());
        }
        
        // Disconnect from server
        client.disconnect();
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}