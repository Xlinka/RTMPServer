#ifndef PARSEUTILS_H
#define PARSEUTILS_H

#include <vector>
#include <string>
#include <cstddef> // For std::size_t

class Parse {
public:
    static void write_amf_string(const std::string& str, std::vector<char>& buffer);
    static void write_amf_number(double value, std::vector<char>& buffer);
    static void dump_hex(const char* data, std::size_t length);
};

#endif // PARSEUTILS_H
