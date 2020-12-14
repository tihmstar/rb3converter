//
//  ConvertMid.hpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#ifndef ConvertMid_hpp
#define ConvertMid_hpp

#include <iostream>
#include <unistd.h>
#include <stdint.h>

class ConvertMid {
public:
    enum Region{
        Region_undefind = 0,
        Region_PAL,
        Region_NTSC
    };
    enum Edat_type{
        Edat_type_undefined = 0,
        Edat_type_1,
        Edat_type_2,
        Edat_type_3
    };
private:
    int _fd;
    uint8_t *_mem;
    size_t _memSize;
    Region _region;
public:
    ConvertMid(std::string inpath, Region region = Region::Region_PAL, Edat_type edattype = Edat_type::Edat_type_1);
    ~ConvertMid();
    
};
#endif /* ConvertMid_hpp */
