#include "ParseUtils.h"
#include <iostream>

void Parse::write_amf_string(const std::string& str, std::vector<char>& buffer) {
    buffer.push_back(0x02); // AMF0 String marker
    buffer.push_back((str.length() >> 8) & 0xFF);
    buffer.push_back(str.length() & 0xFF);
    buffer.insert(buffer.end(), str.begin(), str.end());
}

void Parse::write_amf_number(double value, std::vector<char>& buffer) {
    buffer.push_back(0x00); // AMF0 Number marker
    char value_bytes[8];
    std::memcpy(value_bytes, &value, 8);

    // AMF uses big-endian format, so we need to add the bytes in reverse order
    for (int i = 7; i >= 0; --i) {
        buffer.push_back(value_bytes[i]);
    }
}
void Parse::dump_hex(const char* data, std::size_t length) {
    std::cout << "Hex dump (" << length << " bytes):" << std::endl;
    for (std::size_t i = 0; i < length; ++i) {
        printf("%02X ", (unsigned char)data[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n");
}
