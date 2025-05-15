#include "server/analyzer.h"
#include "common/log_parser.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <memory> // For std::shared_ptr

LogAnalyzer::LogAnalyzer(const AnalysisRequest& request)
    : request_(request), stop_(false) {
    // Initialize the thread pool with hardware concurrency
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4; // Default to 4 if can't detect
    
    // Initialize result
    result_.type = request_.type;
    result_.totalEntries = 0;
    
    // Start thread pool
    startThreadPool(numThreads);
}

LogAnalyzer::~LogAnalyzer() {
    stopThreadPool();
}

void LogAnalyzer::startThreadPool(unsigned int numThreads) {
    for (unsigned int i = 0; i < numThreads; ++i) {
        threads_.emplace_back(&LogAnalyzer::workerThread, this);
    }
}

void LogAnalyzer::stopThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stop_ = true;
    }
    condition_.notify_all();
    
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void LogAnalyzer::workerThread() {
    while (true) {
        std::function<void()> task = getTask();
        if (!task) {
            break; // Exit if no more tasks or stop is requested
        }
        
        task(); // Execute the task
    }
}

void LogAnalyzer::addTask(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        tasks_.push_back(std::move(task));
    }
    condition_.notify_one();
}

std::function<void()> LogAnalyzer::getTask() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    
    condition_.wait(lock, [this]() {
        return stop_ || !tasks_.empty();
    });
    
    if (stop_ && tasks_.empty()) {
        return nullptr;
    }
    
    auto task = std::move(tasks_.front());
    tasks_.erase(tasks_.begin());
    return task;
}

std::string LogAnalyzer::getKeyForEntry(const LogEntry& entry) {
    switch (request_.type) {
        case AnalysisType::USER:
            return entry.user;
        case AnalysisType::IP:
            return entry.ip;
        case AnalysisType::LOG_LEVEL:
            return entry.level;
        default:
            return entry.user; // Default to user
    }
}

AnalysisResult LogAnalyzer::analyze(const std::vector<std::string>& logFiles) {
    // Create a vector to store futures for each file analysis
    std::vector<std::future<std::unordered_map<std::string, int>>> futures;
    
    // Process each file in a separate task
    for (const auto& filename : logFiles) {
        // Create a packaged task with shared_ptr to make it copy-constructible
        auto taskPtr = std::make_shared<std::packaged_task<std::unordered_map<std::string, int>()>>(
            [this, filename]() { return this->analyzeFile(filename); }
        );
        
        // Get future from task
        std::future<std::unordered_map<std::string, int>> future = taskPtr->get_future();
        futures.push_back(std::move(future));
        
        // Add task to queue using shared_ptr to make it copy-constructible
        addTask([taskPtr]() { (*taskPtr)(); });
    }
    
    // Collect results
    for (auto& future : futures) {
        auto fileCounts = future.get();
        
        // Merge file results with overall results
        std::lock_guard<std::mutex> lock(resultMutex_);
        for (const auto& pair : fileCounts) {
            result_.counts[pair.first] += pair.second;
        }
    }
    
    return result_;
}

std::unordered_map<std::string, int> LogAnalyzer::analyzeFile(const std::string& filename) {
    std::unordered_map<std::string, int> counts;
    int entriesProcessed = 0;
    
    try {
        // Read file content
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return counts;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        // Create appropriate parser based on file extension
        auto parser = LogParser::createParser(filename);
        
        // Parse log entries
        auto entries = parser->parse(content);
        
        // Count based on analysis type
        for (const auto& entry : entries) {
            // Check if entry is in the specified date range
            if (isDateInRange(entry.timestamp, request_.startDate, request_.endDate)) {
                std::string key = getKeyForEntry(entry);
                counts[key]++;
                entriesProcessed++;
            }
        }
        
        // Update total entries
        std::lock_guard<std::mutex> lock(resultMutex_);
        result_.totalEntries += entriesProcessed;
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing file " << filename << ": " << e.what() << std::endl;
    }
    
    return counts;
}