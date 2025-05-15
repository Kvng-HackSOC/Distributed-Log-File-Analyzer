#ifndef LOG_PARSER_H
#define LOG_PARSER_H

#include "common/protocol.h"
#include <string>
#include <vector>
#include <memory>

enum class LogFormat {
    JSON,
    XML,
    TXT,
    UNKNOWN
};

class LogParser {
public:
    LogParser() = default;
    virtual ~LogParser() = default;
    
    // Factory method to create the right parser
    static std::unique_ptr<LogParser> createParser(const std::string& filename);
    static LogFormat detectFormat(const std::string& filename);
    
    // Interface for parsing logs
    virtual std::vector<LogEntry> parse(const std::string& content) = 0;
};

class JsonLogParser : public LogParser {
public:
    std::vector<LogEntry> parse(const std::string& content) override;
};

class XmlLogParser : public LogParser {
public:
    std::vector<LogEntry> parse(const std::string& content) override;
};

class TxtLogParser : public LogParser {
public:
    std::vector<LogEntry> parse(const std::string& content) override;
};

#endif // LOG_PARSER_H