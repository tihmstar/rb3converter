//
//  ConvertPNG.cpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#include "ConvertPNG.hpp"
#include <sys/mman.h>
#include <fcntl.h>
#include <libgeneral/macros.h>
#include <string.h>
#include <arpa/inet.h>

ConvertPNG::ConvertPNG(std::string inpath)
: _loader(nullptr)
, _mem(NULL), _memSize(0)
{
    _loader = new FileLoader(inpath);
    _mem = _loader->mem();
    _memSize = _loader->size();
}

ConvertPNG::ConvertPNG(const void *mem, size_t memSize)
: _loader(nullptr)
, _mem((const uint8_t*)mem), _memSize(memSize)
{
    //
}

ConvertPNG::~ConvertPNG(){
    safeDelete(_loader);
}

void ConvertPNG::convertToPS3(std::string outpath){
    int nfd = -1;
    uint8_t *newmem = NULL;
    cleanup([&]{
        if (newmem){
            munmap(newmem, _memSize);
            newmem = NULL;
        }
        if (nfd != -1) {
            close(nfd);
        }
    });
    size_t newmemsize = _memSize;
    if (newmemsize % 8) {
        newmemsize += 8 - (newmemsize % 8);
    }
    
    assure((nfd = open(outpath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644)) != -1);
    assure(lseek(nfd, newmemsize-1, SEEK_SET) == newmemsize-1);
    {
        uint8_t nullbyte = 0;
        assure(write(nfd, &nullbyte, 1) == 1);
    }
    assure(lseek(nfd, 0, SEEK_SET) == 0);
    assure((newmem = (uint8_t*)mmap(NULL, newmemsize, PROT_READ | PROT_WRITE, MAP_SHARED, nfd, 0)) != (uint8_t*)-1);
    
    assure(newmemsize >= 32);
    
    memcpy(newmem, _mem, 32);
    
    for (size_t i=32; i<_memSize; i+=2) {
        uint16_t *src = (uint16_t*)&_mem[i];
        uint16_t *dst = (uint16_t*)&newmem[i];
        *dst = htons(*src);
    }
    memset(&newmem[_memSize], newmem[_memSize-1], newmemsize-_memSize);
}
