//
//  ConvertMid.cpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#include "ConvertMid.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <libgeneral/macros.h>
#include <sys/mman.h>
#include <sys/stat.h>

ConvertMid::ConvertMid(std::string inpath, Region region, Edat_type edattype)
: _fd(-1),_mem(NULL),_memSize(0), _region(region)
{
    struct stat st = {};
    assure((_fd = open(inpath.c_str(), O_RDONLY)) != -1);
    assure(!fstat(_fd, &st));
    _memSize = st.st_size;
    assure((_mem = (uint8_t*)mmap(NULL, _memSize, PROT_READ, MAP_PRIVATE, _fd, 0)) != (uint8_t*)-1);
}

ConvertMid::~ConvertMid(){
    //
}
