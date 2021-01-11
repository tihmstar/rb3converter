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
#include "dtaParser.hpp"

int main(int argc, const char * argv[]) {
    printf("start\n");

//    ConvertMid cm("/home/malte/projects/rockband_file_format/360unpack/songs/rhythmoflove1xx/rhythmoflove1xx.mid", "/home/malte/projects/rockband_file_format/klic.txt", "/home/malte/projects/rockband_file_format/raps/ntsc.rap", ConvertMid::Region_NTSC, ConvertMid::Edat_type_1);

    dtaParser songsdta("/Users/tihmstar/Desktop/songs.dta");
    
    songsdta.writeToFile("songs.dta");
    
    for (int i=0; i<songsdta.getSongsCnt(); i++) {
        auto id = songsdta.getSongIDForSong(i);
        printf("id=%s\n",id.c_str());
    }
    
//    STFS xboxfs("/Users/tihmstar/Desktop/rb3/rhythmoflove1xx.con");
//
//    xboxfs.extract_all(".");
    
    
//    ConvertMid cm("/Users/tihmstar/Desktop/rb3/unpacked/songs/rhythmoflove1xx/rhythmoflove1xx.mid", "/Users/tihmstar/Desktop/C3CONToolsv401/bin/klic.txt", "/Users/tihmstar/Desktop/C3CONToolsv401/bin/raps/ntsc.rap", ConvertMid::Region_NTSC, ConvertMid::Edat_type_1);

    printf("done\n");
    return 0;
}
