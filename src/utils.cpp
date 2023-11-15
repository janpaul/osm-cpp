#include "utils.h"


uint8_t getBlobHeaderSize(const char *buffer) {
    return (buffer[3] & 0xFF) | ((buffer[2] & 0xFF) << 8) | ((buffer[1] & 0xFF) << 16) | ((buffer[0] & 0xFF) << 24);
}

std::string presentBool(bool value) {
    return value ? "YES" : "NO";
}
