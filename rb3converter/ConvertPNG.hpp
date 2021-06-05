//
//  ConvertPNG.hpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#ifndef ConvertPNG_hpp
#define ConvertPNG_hpp

#include "FileLoader.hpp"
#include <stdint.h>
#include <stdlib.h>

class ConvertPNG{
    FileLoader *_loader;
    const uint8_t *_mem; //don't free this!
    size_t _memSize;
public:
    ConvertPNG(std::string inpath);
    ConvertPNG(const void *mem, size_t memSize);
    ~ConvertPNG();
    
    void convertToPS3(std::string outpath);
    
};

#endif /* ConvertPNG_hpp */
