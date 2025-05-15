#ifndef ANALYZER_H
#define ANALYZER_H

#include "common/protocol.h"
#include <vector>
#include <string>
#include <mutex>
#include <future>
#include <unordered_map>
#include <thread>
#include <condition_variable>

class LogAnalyzer {
public:
    LogAnalyzer(const AnalysisRequest& request);
    ~LogAnalyzer();
    
    // Main analysis method
    AnalysisResult analyze(const std::vector<std::string>& logFiles);
    
private:
    // Analyze a single file
    std::unordered_map<std::string, int> analyzeFile(const std::string& filename);
    
    // Extract key based on analysis type
    std::string getKeyForEntry(const LogEntry& entry);
    
    // Thread pool management
    void startThreadPool(unsigned int numThreads);
    void stopThreadPool();
    
    // Worker thread function
    void workerThread();
    
    // Task queue management
    void addTask(std::function<void()> task);
    std::function<void()> getTask();
    
    // Member variables
    AnalysisRequest request_;
    AnalysisResult result_;
    std::mutex resultMutex_;
    
    // Thread pool variables
    std::vector<std::thread> threads_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
    std::vector<std::function<void()>> tasks_;
    bool stop_;
};

#endif // ANALYZER_H