#include "log_parser.h"
#include "protocol.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <iostream>
#include <filesystem>

// Helper function to trim whitespace
static inline std::string trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char c) {
        return std::isspace(c);
    });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) {
        return std::isspace(c);
    }).base();
    return (start < end) ? std::string(start, end) : std::string();
}

LogFormat LogParser::detectFormat(const std::string& filename) {
    // Detect format based on file extension
    std::filesystem::path path(filename);
    std::string extension = path.extension().string();
    
    std::transform(extension.begin(), extension.end(), extension.begin(),
                  [](unsigned char c){ return std::tolower(c); });
    
    if (extension == ".json") return LogFormat::JSON;
    if (extension == ".xml") return LogFormat::XML;
    if (extension == ".txt") return LogFormat::TXT;
    
    return LogFormat::UNKNOWN;
}

std::unique_ptr<LogParser> LogParser::createParser(const std::string& filename) {
    LogFormat format = detectFormat(filename);
    
    switch (format) {
        case LogFormat::JSON:
            return std::make_unique<JsonLogParser>();
        case LogFormat::XML:
            return std::make_unique<XmlLogParser>();
        case LogFormat::TXT:
            return std::make_unique<TxtLogParser>();
        default:
            // Default to TXT parser
            return std::make_unique<TxtLogParser>();
    }
}

// Simple JSON parser for log entries
std::vector<LogEntry> JsonLogParser::parse(const std::string& content) {
    std::vector<LogEntry> entries;
    
    // Simple JSON parsing - in a production system, use a proper JSON library
    std::istringstream iss(content);
    std::string line;
    
    // Regex patterns for extracting JSON fields
    std::regex timestampRegex("\"timestamp\"\\s*:\\s*\"([^\"]+)\"");
    std::regex userRegex("\"user\"\\s*:\\s*\"([^\"]+)\"");
    std::regex ipRegex("\"ip\"\\s*:\\s*\"([^\"]+)\"");
    std::regex levelRegex("\"level\"\\s*:\\s*\"([^\"]+)\"");
    std::regex messageRegex("\"message\"\\s*:\\s*\"([^\"]*)\"");
    
    LogEntry currentEntry;
    bool inEntry = false;
    
    while (std::getline(iss, line)) {
        line = trim(line);
        
        if (line == "{") {
            inEntry = true;
            currentEntry = LogEntry();
        }
        else if (line == "}" || line == "},") {
            if (inEntry) {
                entries.push_back(currentEntry);
                inEntry = false;
            }
        }
        else if (inEntry) {
            std::smatch match;
            
            if (std::regex_search(line, match, timestampRegex)) {
                currentEntry.timestamp = match[1];
            }
            else if (std::regex_search(line, match, userRegex)) {
                currentEntry.user = match[1];
            }
            else if (std::regex_search(line, match, ipRegex)) {
                currentEntry.ip = match[1];
            }
            else if (std::regex_search(line, match, levelRegex)) {
                currentEntry.level = match[1];
            }
            else if (std::regex_search(line, match, messageRegex)) {
                currentEntry.message = match[1];
            }
        }
    }
    
    return entries;
}

// Simple XML parser for log entries
std::vector<LogEntry> XmlLogParser::parse(const std::string& content) {
    std::vector<LogEntry> entries;
    
    // Simple XML parsing - in a production system, use a proper XML library
    std::istringstream iss(content);
    std::string line;
    
    // Regex patterns for extracting XML fields
    std::regex entryStartRegex("<entry>");
    std::regex entryEndRegex("</entry>");
    std::regex timestampRegex("<timestamp>([^<]+)</timestamp>");
    std::regex userRegex("<user>([^<]+)</user>");
    std::regex ipRegex("<ip>([^<]+)</ip>");
    std::regex levelRegex("<level>([^<]+)</level>");
    std::regex messageRegex("<message>([^<]*)</message>");
    
    LogEntry currentEntry;
    bool inEntry = false;
    
    while (std::getline(iss, line)) {
        line = trim(line);
        
        if (std::regex_search(line, entryStartRegex)) {
            inEntry = true;
            currentEntry = LogEntry();
        }
        else if (std::regex_search(line, entryEndRegex)) {
            if (inEntry) {
                entries.push_back(currentEntry);
                inEntry = false;
            }
        }
        else if (inEntry) {
            std::smatch match;
            
            if (std::regex_search(line, match, timestampRegex)) {
                currentEntry.timestamp = match[1];
            }
            else if (std::regex_search(line, match, userRegex)) {
                currentEntry.user = match[1];
            }
            else if (std::regex_search(line, match, ipRegex)) {
                currentEntry.ip = match[1];
            }
            else if (std::regex_search(line, match, levelRegex)) {
                currentEntry.level = match[1];
            }
            else if (std::regex_search(line, match, messageRegex)) {
                currentEntry.message = match[1];
            }
        }
    }
    
    return entries;
}

// Simple TXT parser for log entries - assumes a specific format
std::vector<LogEntry> TxtLogParser::parse(const std::string& content) {
    std::vector<LogEntry> entries;
    
    // Assume tab or pipe delimited format: timestamp|user|ip|level|message
    std::istringstream iss(content);
    std::string line;
    
    while (std::getline(iss, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;  // Skip empty lines or comments
        }
        
        LogEntry entry;
        std::istringstream lineStream(line);
        std::string token;
        
        // Try to parse as pipe-delimited
        if (line.find('|') != std::string::npos) {
            std::getline(lineStream, entry.timestamp, '|');
            std::getline(lineStream, entry.user, '|');
            std::getline(lineStream, entry.ip, '|');
            std::getline(lineStream, entry.level, '|');
            std::getline(lineStream, entry.message);
        }
        // Try to parse as tab-delimited
        else if (line.find('\t') != std::string::npos) {
            std::getline(lineStream, entry.timestamp, '\t');
            std::getline(lineStream, entry.user, '\t');
            std::getline(lineStream, entry.ip, '\t');
            std::getline(lineStream, entry.level, '\t');
            std::getline(lineStream, entry.message);
        }
        // Try to parse as space-delimited (assuming message can contain spaces)
        else {
            std::istringstream spaceStream(line);
            spaceStream >> entry.timestamp >> entry.user >> entry.ip >> entry.level;
            
            // Rest of the line is the message
            std::getline(spaceStream, entry.message);
            entry.message = trim(entry.message);
        }
        
        // Trim all fields
        entry.timestamp = trim(entry.timestamp);
        entry.user = trim(entry.user);
        entry.ip = trim(entry.ip);
        entry.level = trim(entry.level);
        entry.message = trim(entry.message);
        
        // Add valid entries only
        if (!entry.timestamp.empty() && !entry.user.empty() && 
            !entry.ip.empty() && !entry.level.empty()) {
            entries.push_back(entry);
        }
    }
    
    return entries;
}