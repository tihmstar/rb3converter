//
//  FileLoader.cpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#include "FileLoader.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <libgeneral/macros.h>
#include <sys/mman.h>
#include <sys/stat.h>

FileLoader::FileLoader(std::string inpath)
: _fd(-1), _mem(NULL), _memSize(0)
{
    struct stat st = {};
    assure((_fd = open(inpath.c_str(), O_RDONLY)) != -1);
    assure(!fstat(_fd, &st));
    _memSize = st.st_size;
    if (_memSize) {
        assure((_mem = (const uint8_t*)mmap(NULL, _memSize, PROT_READ, MAP_PRIVATE, _fd, 0)) != (uint8_t*)-1);
    }else{
        _memSize = 1;
        assure((_mem = (const uint8_t*)mmap(NULL, _memSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) != (uint8_t*)-1);
        ((char*)_mem)[0] = '\0';
    }
}

FileLoader::~FileLoader(){
    if (_mem) {
        munmap((void*)_mem, _memSize);
    }
    if (_fd != -1){
        close(_fd);
    }
}

const uint8_t *FileLoader::mem(){
    return _mem;
}

size_t FileLoader::size(){
    return _memSize;
}
