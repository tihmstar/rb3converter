//
//  ConvertMogg.hpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#ifndef ConvertMogg_hpp
#define ConvertMogg_hpp

#include <iostream>
#include <unistd.h>
#include <stdint.h>

class ConvertMogg {
public:
    enum CryptVersion{
        unknown = 0,
        x0A = 0x0A, //No encryption
        x0B = 0x0B, //RB1, RB1 DLC
        x0C = 0x0C, //RB2, AC/DC Live, some RB2 DLC
        x0D = 0x0D, // TODO: what?
        x0E = 0x0E, //Lego, Green Day, most RB2 DLC
        x0F = 0x0F, //RBN
        x10 = 0x10, //RB3, RB3 DLC
    };
    
private:
    int _fd;
    uint8_t *_mem;
    size_t _memSize;
public:
    ConvertMogg(std::string inpath);
    ~ConvertMogg();
    
    const CryptVersion cryptversion();
    
    
};

#endif /* ConvertMogg_hpp */
