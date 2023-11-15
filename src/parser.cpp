#include <iostream>
#include <fstream>
#include "generated/fileformat.pb.h"
#include "generated/osmformat.pb.h"
#include "parser.h"
#include "utils.h"

const int blobHeaderLength = 4;

template<typename T>
T getProtoBlock(std::ifstream&file, std::streamsize size) {
    char buffer[size];
    file.read(buffer, size);
    T data;
    data.ParseFromArray(buffer, size);
    return data;
}


BlobHeader getBlobHeader(std::ifstream&file) {
    // Read the first 4 bytes of the file, this value is the size of the blob header
    char bHeaderSize[blobHeaderLength];
    file.read(bHeaderSize, blobHeaderLength);
    int blobHeaderSize = getBlobHeaderSize(bHeaderSize);

    return getProtoBlock<BlobHeader>(file, blobHeaderSize);
}


void parse(std::ifstream&file) {
    auto blobHeader = getBlobHeader(file);
    auto blobSize = blobHeader.datasize();
    //    std::cout << "blob header type: " << blobHeader.type() << std::endl;
    //    std::cout << "blob header datasize: " << blobSize << std::endl;


    auto blob = getProtoBlock<Blob>(file, blobSize);
    std::cout << "blob raw size: " << blob.raw_size() << std::endl;
    std::cout << "has raw?: " << presentBool(blob.has_raw()) << std::endl;
    std::cout << "has zlib?: " << presentBool(blob.has_zlib_data()) << std::endl;
}
