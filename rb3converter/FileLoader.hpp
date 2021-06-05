//
//  FileLoader.hpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#ifndef FileLoader_hpp
#define FileLoader_hpp

#include <iostream>
#include <unistd.h>
#include <stdint.h>

class FileLoader{
    int _fd;
protected:
    const uint8_t *_mem;
    size_t _memSize;
public:
    FileLoader(std::string inpath);
    ~FileLoader();
    
    const uint8_t *mem();
    size_t size();
};

#endif /* FileLoader_hpp */
