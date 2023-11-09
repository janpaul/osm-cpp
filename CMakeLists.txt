cmake_minimum_required(VERSION 3.26)
project(osm)

find_package(ZLIB REQUIRED)

set(CMAKE_CXX_STANDARD 23)

add_executable(osm main.cpp)
target_link_libraries(osm ZLIB::ZLIB)
