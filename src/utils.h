#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <ios>

uint8_t getBlobHeaderSize(const char *buffer);

std::string presentBool(bool value);

std::string uncompress_zlib(const std::string &compressed);

template<typename T>
T getProtoBlockFromStream(std::ifstream &file, std::streamsize size) {
    char buffer[size];
    file.read(buffer, size);
    T data;
    data.ParseFromArray(buffer, size);
    return data;
}

template<typename T>
T getProtoBlockFromString(std::string &str) {
    auto size = str.size();
    T data;
    data.ParseFromString(str.substr(0, size));
    return data;
}
