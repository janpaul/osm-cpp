#include <iostream>
#include <fstream>
#include <vector>
#include "generated/fileformat.pb.h"
#include "generated/osmformat.pb.h"
#include "parser.h"
#include "utils.h"

const int blobHeaderLength = 4;
const int32_t defaultGranularity = 100;
const int64_t defaultLatOffset = 0;
const int64_t defaultLonOffset = 0;
//const int32_t defaultDateGranularity = 1000;

using kv_vector = std::vector<int32_t>;
using string_vector = std::vector<std::string>;

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


BlobHeader getBlobHeader(std::ifstream &file) {
    // Read the first 4 bytes of the file, this value is the size of the blob header
    char bHeaderSize[blobHeaderLength];
    file.read(bHeaderSize, blobHeaderLength);
    int blobHeaderSize = getBlobHeaderSize(bHeaderSize);

    return getProtoBlockFromStream<BlobHeader>(file, blobHeaderSize);
}

string_vector stringTableContents(const StringTable &stringTable) {
    std::vector<std::string> contents;
    for (auto const &s: stringTable.s()) {
        contents.push_back(s);
    }
    return contents;
}

kv_vector keysValsAsVector(const DenseNodes &dense) {
    std::vector<int32_t> keysVals;
    for (auto const &kv: dense.keys_vals()) {
        keysVals.push_back(kv);
    }
    return keysVals;
}

std::vector<kv_vector> splitKeysVals(kv_vector in) {
    std::vector<kv_vector> result;
    kv_vector current;
    for (const auto &kv: in) {
        if (kv == 0) {
            result.push_back(current);
            current.clear();
        } else {
            current.push_back(kv);
        }
    }
    return result;
}

void parseNode(const Node &node) {

}

void parse(std::ifstream &file) {
    bool done = false;
    size_t primitiveCount = 0;

    while (!done) {
        auto const &blobHeader = getBlobHeader(file);
        auto const &blobSize = blobHeader.datasize();
//        std::cout << "blob header type: " << blobHeader.type() << "\n";
        //    std::cout << "blob header datasize: " << blobSize << "\n";

        auto const &blob = getProtoBlockFromStream<Blob>(file, blobSize);
//    std::cout << "blob raw size: " << blob.raw_size() << "\n";
//    std::cout << "has raw?: " << presentBool(blob.has_raw()) << "\n";
//    std::cout << "has zlib?: " << presentBool(blob.has_zlib_data()) << "\n";

        auto data = blob.has_zlib_data() ? uncompress_zlib(blob.zlib_data()) : blob.raw();
//    std::cout << "data size: " << data.size() << "\n";

        if (blob.raw_size() != data.size()) {
            throw (std::runtime_error("Blob raw size does not match data size."));
        }

        if (blobHeader.type() == "OSMHeader") {
            std::cout << "************************************* OSM HEADER *************************************"
                      << "\n";
            auto const &headerBlock = getProtoBlockFromString<HeaderBlock>(data);
            std::cout << "* writing program: " << headerBlock.writingprogram() << "\n";
            auto const &bbox = headerBlock.bbox();
            std::cout << "* bbox: " << bbox.left() << ", " << bbox.right() << ", " << bbox.top() << ", "
                      << bbox.bottom()
                      << "\n";
            for (auto const &feature: headerBlock.required_features()) {
                std::cout << "* required feature: " << feature << "\n";
            }
            for (auto const &feature: headerBlock.optional_features()) {
                std::cout << "* optional feature: " << feature << "\n";
            }
            std::cout << "**************************************************************************************"
                      << "\n";
        } else { // blobHeader.type() == "OSMData"
            auto const &block = getProtoBlockFromString<PrimitiveBlock>(data);
            primitiveCount++;
//            std::cout << "granularity: " << primitiveBlock.date_granularity() << "\n";
//            std::cout << "lat_offset: " << primitiveBlock.lat_offset() << "\n";
//            std::cout << "lon_offset: " << primitiveBlock.lon_offset() << "\n";
//            std::cout << "has stringtable: " << primitiveBlock.has_stringtable() << "\n";

            auto const granularity = block.has_granularity() ? block.granularity() : defaultGranularity;
            auto const latOffset = block.has_lat_offset() ? block.lat_offset() : defaultLatOffset;
            auto const lonOffset = block.has_lon_offset() ? block.lon_offset() : defaultLonOffset;
//            auto const dateGranularity = block.has_date_granularity() ? block.date_granularity()
//                                                                      : defaultDateGranularity;
            auto const &strings = stringTableContents(block.has_stringtable() ? block.stringtable() : StringTable());

            auto latitude = [latOffset, granularity](int64_t lat) {
                return .000000001 * (double) (latOffset + (granularity * lat));
            };
            auto longitude = [lonOffset, granularity](int64_t lon) {
                return .000000001 * (double) (lonOffset + (granularity * lon));
            };
            auto addNode = [strings](uint64_t id, double lat, double lon, kv_vector keysvals) {
//                std::cout << "adding node: " << id << ", " << lat << ", " << lon << "\n";

                size_t i = 0;
                while (i < keysvals.size()) {
                    auto key = strings.at(keysvals.at(i));
                    auto val = strings.at(keysvals.at(i + 1));
                    std::cout << "key: " << key << ", val: " << val << "\n";
                    i += 2;
                }
            };
            auto parseDenseNodes = [latitude, longitude, addNode](const DenseNodes &dense) {
                //    std::cout << "parsing dense nodes" << "\n";
                auto const &allKeysVals = keysValsAsVector(dense);
                auto const &ids = dense.id();
                auto const &lats = dense.lat();
                auto const &lons = dense.lon();
                auto const &keysvals = splitKeysVals(allKeysVals);
                std::cout << "keysvals: " << keysvals.size() << "\n";
                int64_t id = 0, lat = 0, lon = 0;
                if (ids.size() != lats.size() || ids.size() != lons.size()) {
                    throw (std::runtime_error("amount of ids, lats and lons do not match."));
                }

                for (int i = 0; i < ids.size(); i++) {
                    id += ids[i];
                    lat += lats[i];
                    lon += lons[i];
//                    std::cout << "id: " << id << ", lat: " << lat << ", lon: " << lon << "\n";
                    addNode(id, latitude(lat), longitude(lon), keysvals.at(i));
                }
            };
//            std::cout << "strings: " << strings.size() << "\n";
//            std::cout << "primitive groups in block: " << block.primitivegroup_size() << "\n";

            for (auto const &group: block.primitivegroup()) {
                auto const &nodes = group.nodes();

//                for (auto const &node: nodes) {
//                    addNode(node.id(), latitude(node.lat()), longitude(node.lon()));
//                }

                if (group.has_dense()) { // probably always true
                    parseDenseNodes(group.dense());
                }
            }

//            std::cout << "primitive blocks read: " << primitiveCount << "\n";
        }

        std::cout << std::endl; // flush stdout
    }
}
