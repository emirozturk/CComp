#include <iostream>
#include <string>
#include <chrono>
#include "compress.hpp"
#include "decompress.hpp"
#include "search.hpp"

int main(int argc, const char * argv[]) {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::string param = argv[1];
    std::string fileName = argv[2];
    unsigned char coreCount = atoi(argv[3]);
    
    if(param == "-c")
        Compress(fileName,coreCount,atoi(argv[4]));
    if(param =="-d")
        Decompress(fileName,coreCount);
    if(param =="-s")
        std::cout << Search(fileName,coreCount,argv[4]) << std::endl;
    
    auto finish = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count()/1000000 << " ms\n";
    return 0;
}
