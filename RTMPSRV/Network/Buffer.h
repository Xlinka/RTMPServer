#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <cstddef> // For std::size_t
#include <mutex>   // For std::mutex

class Buffer {
public:
    static void store_data(const char* data, std::size_t length);
    static const std::vector<char>& get_data();
    static void clear();

private:
    static std::vector<char> buffer_;    // Internal storage for incoming data
    static std::mutex buffer_mutex_;     // Mutex for thread-safe access
};

#endif // BUFFER_H
