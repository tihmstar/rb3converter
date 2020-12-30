//
//  main.cpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#include <iostream>
#include "ConvertMogg.hpp"
#include "ConvertPNG.hpp"
#include "ConvertMid.hpp"
#include "STFS.hpp"

int main(int argc, const char * argv[]) {
    printf("start\n");

//    ConvertMid cm("/home/malte/projects/rockband_file_format/360unpack/songs/rhythmoflove1xx/rhythmoflove1xx.mid", "/home/malte/projects/rockband_file_format/klic.txt", "/home/malte/projects/rockband_file_format/raps/ntsc.rap", ConvertMid::Region_NTSC, ConvertMid::Edat_type_1);

    STFS xboxfs("/Users/tihmstar/Desktop/rb3/rhythmoflove1xx.con");
    
    
    ConvertMid cm("/Users/tihmstar/Desktop/rb3/unpacked/songs/rhythmoflove1xx/rhythmoflove1xx.mid", "/Users/tihmstar/Desktop/C3CONToolsv401/bin/klic.txt", "/Users/tihmstar/Desktop/C3CONToolsv401/bin/raps/ntsc.rap", ConvertMid::Region_NTSC, ConvertMid::Edat_type_1);

    printf("done\n");
    return 0;
}
