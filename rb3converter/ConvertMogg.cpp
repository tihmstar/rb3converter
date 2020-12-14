//
//  ConvertMogg.cpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#include "ConvertMogg.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <libgeneral/macros.h>
#include <sys/mman.h>
#include <sys/stat.h>

static uint8_t h[72]{
    0,
    0,
    0,
    0,
    52,
    150,
    193,
    31,
    5,
    9,
    18,
    102,
    158,
    32,
    9,
    239,
    48,
    139,
    162,
    33,
    0,
    0,
    0,
    0,
    77,
    18,
    106,
    71,
    0,
    0,
    0,
    0,
    172,
    5,
    125,
    240,
    139,
    41,
    29,
    144,
    44,
    88,
    122,
    12,
    16,
    195,
    83,
    235,
    78,
    100,
    234,
    218,
    65,
    200,
    165,
    236,
    66,
    38,
    127,
    0,
    4,
    240,
    85,
    39,
    6,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};

uint8_t k[16] {
    49,
    152,
    224,
    109,
    22,
    180,
    128,
    13,
    177,
    202,
    249,
    44,
    216,
    213,
    22,
    130
};

ConvertMogg::ConvertMogg(std::string inpath)
: _fd(-1), _mem(NULL), _memSize(0)
{
    struct stat st = {};
    assure((_fd = open(inpath.c_str(), O_RDONLY)) != -1);
    assure(!fstat(_fd, &st));
    _memSize = st.st_size;
    assure((_mem = (uint8_t*)mmap(NULL, _memSize, PROT_READ, MAP_PRIVATE, _fd, 0)) != (uint8_t*)-1);
}

ConvertMogg::~ConvertMogg(){
    
}

const ConvertMogg::CryptVersion ConvertMogg::cryptversion(){
    return (ConvertMogg::CryptVersion)_mem[0];
}
