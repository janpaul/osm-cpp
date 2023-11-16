#pragma once

#include <string>

uint8_t getBlobHeaderSize(const char *buffer);

std::string presentBool(bool value);

std::string uncompress_zlib(const std::string &compressed);
