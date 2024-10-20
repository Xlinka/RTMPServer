#include "Buffer.h"
#include <iostream>
#include <vector>
#include <mutex>

std::vector<char> Buffer::buffer_;
std::mutex Buffer::buffer_mutex_;

void Buffer::store_data(const char* data, std::size_t length) {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    buffer_.insert(buffer_.end(), data, data + length);
    std::cout << "Buffer: Stored " << length << " bytes of data." << std::endl;
}

const std::vector<char>& Buffer::get_data() {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    return buffer_;
}

void Buffer::clear() {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    buffer_.clear();
    std::cout << "Buffer: Cleared." << std::endl;
}
