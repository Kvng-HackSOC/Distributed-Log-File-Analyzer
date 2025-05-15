cmake_minimum_required(VERSION 3.10)
project(distributed_log_analyzer VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Add common library
add_library(common
    src/common/log_parser.cpp
    src/common/protocol.cpp
)

# Add client executable
add_executable(client
    src/client/main.cpp
    src/client/client.cpp
)
target_link_libraries(client common)

# Add server executable
add_executable(server
    src/server/main.cpp
    src/server/server.cpp
    src/server/analyzer.cpp
)
target_link_libraries(server common)

# Windows-specific: Link with winsock2
if(WIN32)
    target_link_libraries(client ws2_32)
    target_link_libraries(server ws2_32)
endif()

# Create test log directories
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_logs)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_logs/client1)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_logs/client2)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_logs/client3)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_logs/client4)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_logs/client5)