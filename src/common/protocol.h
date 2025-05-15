#ifndef PROTOCOL_H
#define PROTOCOL_H

// Platform-specific includes
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <ctime>

enum class AnalysisType {
    USER,
    IP,
    LOG_LEVEL
};

struct LogEntry {
    std::string timestamp;
    std::string user;
    std::string ip;
    std::string level;
    std::string message;
};

struct AnalysisRequest {
    AnalysisType type;
    std::optional<std::string> startDate;
    std::optional<std::string> endDate;
};

struct AnalysisResult {
    AnalysisType type;
    std::unordered_map<std::string, int> counts;
    int totalEntries = 0;
};

// Protocol specific constants
constexpr int DEFAULT_PORT = 8080;
constexpr int BUFFER_SIZE = 4096;

// Message types
constexpr char MSG_REQUEST = 'R';
constexpr char MSG_FILE_START = 'F';
constexpr char MSG_FILE_CHUNK = 'C';
constexpr char MSG_FILE_END = 'E';
constexpr char MSG_RESULT = 'S';
constexpr char MSG_ERROR = 'X';
constexpr char MSG_ACK = 'A';

// Cross-platform socket type
#ifdef _WIN32
typedef SOCKET socket_t;
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#define SOCKET_ERROR_VALUE SOCKET_ERROR
#define close_socket closesocket
#else
typedef int socket_t;
#define INVALID_SOCKET_VALUE -1
#define SOCKET_ERROR_VALUE -1
#define close_socket close
#endif

// Protocol functions
bool sendMessage(socket_t socket, char type, const std::string& message);
bool receiveMessage(socket_t socket, char& type, std::string& message);

// Utility functions
std::string serializeRequest(const AnalysisRequest& request);
AnalysisRequest deserializeRequest(const std::string& data);

std::string serializeResult(const AnalysisResult& result);
AnalysisResult deserializeResult(const std::string& data);

// Date utility functions
bool isDateInRange(const std::string& date,
                   const std::optional<std::string>& startDate,
                   const std::optional<std::string>& endDate);

// String conversion utilities
AnalysisType stringToAnalysisType(const std::string& typeStr);
std::string analysisTypeToString(AnalysisType type);

// Network initialization/cleanup (for Windows)
#ifdef _WIN32
bool initializeWinsock();
void cleanupWinsock();
#endif

#endif // PROTOCOL_H