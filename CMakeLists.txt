cmake_minimum_required(VERSION 3.26)
project(osm)
set(CMAKE_CXX_STANDARD 23)

find_package(ZLIB REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(protobuf REQUIRED CONFIG)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${SQLITE3_INCLUDE_DIRS} protobuf::libprotobuf)

add_executable(
        osm src/main.cpp
        src/utils.cpp src/utils.h
        src/generated/fileformat.pb.cc src/generated/fileformat.pb.h
        src/generated/osmformat.pb.cc src/generated/osmformat.pb.h
        src/parser.cpp src/parser.h
)
target_link_libraries(osm ${ZLIB_LIBRARIES} ${SQLITE3_LIBRARIES} protobuf::libprotobuf)
