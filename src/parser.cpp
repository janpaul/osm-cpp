#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "generated/fileformat.pb.h"
#include "generated/osmformat.pb.h"
#include "parser.h"
#include "utils.h"

const int blobHeaderLength = 4;
const int32_t defaultGranularity = 100;
const int64_t defaultLatOffset = 0;
const int64_t defaultLonOffset = 0;

const std::array<std::string, 6> allowedKeys = {"name", "surface", "place", "population", "description", "elevation"};

using kv_vector = std::vector<int32_t>;
using string_vector = std::vector<std::string>;
using kv_map = std::unordered_map<std::string, std::string>;

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


std::vector<kv_vector> splitKeysVals(const kv_vector &in) {
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

kv_map generateKeysValsMap(const kv_vector &keysvals, const string_vector &strings) {
    kv_map map;
    size_t i = 0;
    while (i < keysvals.size()) {
        auto const &key = strings.at(keysvals.at(i));
        auto const &val = strings.at(keysvals.at(i + 1));
        if (std::includes(allowedKeys.begin(), allowedKeys.end(), &key, &key + 1)) {
            std::cout << key << "=" << val << "\n";
            map[key] = val;
        } else {
//            std::cout << "skipping key: " << key << "=" << val << "\n";
        }
        i += 2;
    }
    return map;
}

void parse(std::ifstream &file) {
    size_t primitiveCount = 0;

    while (!file.eof()) {
        auto const &blobHeader = getBlobHeader(file);
        auto const &blobSize = blobHeader.datasize();

        auto const &blob = getProtoBlockFromStream<Blob>(file, blobSize);
        auto data = blob.has_zlib_data() ? uncompress_zlib(blob.zlib_data()) : blob.raw();

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

            auto const granularity = block.has_granularity() ? block.granularity() : defaultGranularity;
            auto const latOffset = block.has_lat_offset() ? block.lat_offset() : defaultLatOffset;
            auto const lonOffset = block.has_lon_offset() ? block.lon_offset() : defaultLonOffset;

            auto const &strings = stringTableContents(block.has_stringtable() ? block.stringtable() : StringTable());

            auto latitude = [latOffset, granularity](int64_t lat) {
                return .000000001 * (double) (latOffset + (granularity * lat));
            };
            auto longitude = [lonOffset, granularity](int64_t lon) {
                return .000000001 * (double) (lonOffset + (granularity * lon));
            };
            auto addNode = [strings](uint64_t id, double lat, double lon, const kv_map &keysvals) {
            };

            for (auto const &group: block.primitivegroup()) {
                auto const &nodes = group.nodes();
                for (auto const &node: nodes) {
                    kv_vector keysvals;
                    for (auto i = 0; i < node.keys_size(); i++) {
                        keysvals.push_back((int32_t) node.keys(i));
                        keysvals.push_back((int32_t) node.vals(i + 1));
                    }
                    auto const kvMap = generateKeysValsMap(keysvals, strings);
                    addNode(node.id(), latitude(node.lat()), longitude(node.lon()), kvMap);
                }

                if (group.has_dense()) { // probably always true
                    auto const &dense = group.dense();
                    auto const &ids = dense.id();
                    auto const &lats = dense.lat();
                    auto const &lons = dense.lon();
                    auto const &keysvals = splitKeysVals(keysValsAsVector(dense));
                    std::cout << "keysvals: " << keysvals.size() << "\n";
                    int64_t id = 0, lat = 0, lon = 0;
                    assert(ids.size() == lats.size() || ids.size() == lons.size() || ids.size() == keysvals.size());

                    for (int i = 0; i < ids.size(); i++) {
                        id += ids[i];
                        lat += lats[i];
                        lon += lons[i];
                        auto const &kvMap = generateKeysValsMap(keysvals.at(i), strings);
                        addNode(id, latitude(lat), longitude(lon), kvMap);
                    }
                }
            }
        }
        std::cout << std::endl;
    }
}
