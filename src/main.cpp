#include <iostream>
#include <fstream>
#include <filesystem>
#include <zlib.h>
#include "parser.h"


namespace fs = std::filesystem;


int main() {
    std::cout << "OSM parser" << std::endl;
    fs::path currentPath = fs::current_path();
    std::ifstream file("/Users/janpaul/code/janpaul/osm/netherlands-latest.osm.pbf", std::ios::binary);
    if (file) {
        parse(file);
    }
    std::cout << "Ok" << std::endl;

    file.close();
    return 0;
}
