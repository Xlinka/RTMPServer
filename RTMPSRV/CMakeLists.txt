cmake_minimum_required(VERSION 3.10)

# Project name and version
project(RTMPServer VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add source files
set(SOURCES
    Main.cpp
    Network/Client.cpp
    Network/Buffer.cpp
    Network/Parse.cpp
    Network/ParseAMF.cpp  # New file added
    Network/ParseControl.cpp  # New file added
    Network/ParseUtils.cpp  # New file added
)

# Add include directories
include_directories(${PROJECT_SOURCE_DIR}/Network)

# Create executable
add_executable(RTMPServer ${SOURCES})

# Link Winsock library on Windows
if (WIN32)
    target_link_libraries(RTMPServer ws2_32)
endif()


