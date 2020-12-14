//
//  main.cpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#include <iostream>
#include "ConvertMogg.hpp"
#include "ConvertMid.hpp"

int main(int argc, const char * argv[]) {
    printf("start\n");

    ConvertMogg cm("/Users/tihmstar/Desktop/rb3/unpacked/songs/rhythmoflove1xx/rhythmoflove1xx.mogg");
    // ConvertMid cm("/home/malte/projects/rockband_file_format/360unpack/songs/rhythmoflove1xx/rhythmoflove1xx.mid", "/home/malte/projects/rockband_file_format/klic.txt", "/home/malte/projects/rockband_file_format/raps/ntsc.rap", ConvertMid::Region_NTSC, ConvertMid::Edat_type_1);

    printf("got cryptversion=0x%X\n",cm.cryptversion());
    
    cm.convertToPS3("/tmp/kk/myconvert.mogg");
    
    printf("done\n");
    return 0;
}
