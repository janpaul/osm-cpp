#include <zlib.h>
#include "utils.h"

uint8_t getBlobHeaderSize(const char *buffer) {
    return (buffer[3] & 0xFF) | ((buffer[2] & 0xFF) << 8) | ((buffer[1] & 0xFF) << 16) | ((buffer[0] & 0xFF) << 24);
}

std::string presentBool(bool value) {
    return value ? "YES" : "NO";
}

const size_t BUFFER_SIZE = 32768;

std::string uncompress_zlib(const std::string &compressed) {
    z_stream zs;                        // z_stream is zlib's control structure
    memset(&zs, 0, sizeof(zs));
    if (inflateInit(&zs) != Z_OK)
        throw (std::runtime_error("inflateInit failed while decompressing."));

    zs.next_in = (Bytef *) compressed.data();
    zs.avail_in = compressed.size();
    int ret;
    char outbuffer[BUFFER_SIZE];
    std::string outstring;

    do {
        zs.next_out = reinterpret_cast<Bytef *>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = inflate(&zs, 0);

        if (outstring.size() < zs.total_out) {
            outstring.append(outbuffer, zs.total_out - outstring.size());
        }
    } while (ret == Z_OK);

    inflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        throw (std::runtime_error("Exception during zlib decompression: (" + std::to_string(ret) + ") " + zs.msg));
    }

    return outstring;
}