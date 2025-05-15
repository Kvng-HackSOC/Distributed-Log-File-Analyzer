#!/bin/bash

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build (without parallel flag for Windows compatibility)
cmake --build .

# Create test log directories if they don't exist
mkdir -p test_logs/client1
mkdir -p test_logs/client2
mkdir -p test_logs/client3
mkdir -p test_logs/client4
mkdir -p test_logs/client5

# Copy sample logs to test directories if they exist
if [ -d "../test_logs" ]; then
    cp -r ../test_logs/* test_logs/
fi

echo "Build completed successfully."
echo "Run the server with: ./server"
echo "Run the client with: ./client <server_ip> <analysis_type> [start_date] [end_date]"
echo "Example: ./client 127.0.0.1 user 2023-01-01 2023-12-31"