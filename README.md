Distributed Log File Analyzer
A high-performance client-server application for distributed log file analysis, supporting concurrent processing of multiple log formats with real-time statistical analysis.
🚀 Features

Multi-threaded Server: Concurrent processing using thread pool architecture
Multiple Log Formats: Support for JSON, XML, and TXT log files
Analysis Types:

User-based analysis
IP address analysis
Log level analysis


Date Range Filtering: Optional date-based filtering for targeted analysis
Cross-Platform: Compatible with Windows and Linux environments
Network Protocol: Custom TCP/IP protocol for efficient data transmission
Result Export: Save analysis results to files for reporting

📋 Prerequisites

C++ compiler with C++17 support
CMake 3.10 or higher
POSIX-compliant system (Linux) or Windows with MinGW/WSL
Git for version control

🛠️ Installation

Clone the repository
bashgit clone https://github.com/Kvng-HackSOC/Distributed-Log-File-Analyzer.git
cd Distributed-Log-File-Analyzer

Build the project
bashchmod +x build_script.sh
./build_script.sh
This will:

Create a build directory
Configure the project with CMake
Compile the source code
Generate server and client executables



💻 Usage
Starting the Server
bashcd build
./server [port]
Default port is 8080 if not specified.
Running the Client
bash./client <server_ip> <analysis_type> [log_directory] [start_date] [end_date] [output_file]
Parameters:

server_ip: IP address of the server (e.g., 127.0.0.1)
analysis_type: Type of analysis (user | ip | log_level)
log_directory: Directory containing log files (optional, auto-selects if not specified)
start_date: Start date for filtering (YYYY-MM-DD format, optional)
end_date: End date for filtering (YYYY-MM-DD format, optional)
output_file: File to save results (optional)

Examples:
bash# Basic user analysis
./client 127.0.0.1 user

# IP analysis with specific directory
./client 127.0.0.1 ip test_logs/client1

# Log level analysis with date range
./client 127.0.0.1 log_level test_logs/client2 2023-01-01 2023-12-31

# Save results to file
./client 127.0.0.1 user test_logs/client3 "" "" results.txt
🧪 Testing
Run the comprehensive test suite:
bashchmod +x run_all_tests.sh
./run_all_tests.sh
This script runs various test scenarios including:

Different analysis types
Multiple client directories
Date range filtering
Output file generation

📁 Project Structure
distributed-log-analyzer/
├── src/
│   ├── common/          # Shared utilities
│   │   ├── protocol.h/cpp
│   │   └── log_parser.h/cpp
│   ├── client/          # Client implementation
│   │   ├── client.h/cpp
│   │   └── main.cpp
│   └── server/          # Server implementation
│       ├── server.h/cpp
│       ├── analyzer.h/cpp
│       └── main.cpp
├── test_logs/           # Sample log files
│   ├── client1/
│   ├── client2/
│   └── ...
├── build/               # Build output directory
├── CMakeLists.txt       # CMake configuration
├── build_script.sh      # Build automation script
└── README.md
🏗️ Architecture
Key Components

LogServer: Main server class handling client connections
LogClient: Client implementation for sending logs and receiving results
LogAnalyzer: Multi-threaded analysis engine with thread pool
LogParser: Extensible parser supporting multiple formats

Protocol Design
Custom message protocol with:

Message type identifiers
Length headers for reliable transmission
Pipe-delimited data serialization

🔧 Configuration
Server Configuration

Default port: 8080
Thread pool size: Auto-detected based on hardware

Client Configuration

Timeout settings configurable in code
Buffer size: 4096 bytes (adjustable)

🤝 Contributing
Contributions are welcome! Please:

Fork the repository
Create your feature branch (git checkout -b feature/AmazingFeature)
Commit your changes (git commit -m 'Add some AmazingFeature')
Push to the branch (git push origin feature/AmazingFeature)
Open a Pull Request

📊 Performance

Supports concurrent processing of multiple clients
Thread pool prevents thread creation overhead
Efficient memory management with streaming file processing
Scalable architecture suitable for large-scale deployments

🐛 Known Issues

Large files may require increased buffer sizes

📄 License
This project is licensed under the MIT License - see the LICENSE file for details.
👥 Authors
Kvng-HackSOC (Ezekiel Obanla)

🙏 Acknowledgments

Inspired by distributed systems principles
Built with modern C++ best practices


For more information, please refer to the project documentation in the docs/ directory.
