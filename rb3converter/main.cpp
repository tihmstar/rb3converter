//
//  main.cpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#include <iostream>
#include "ConvertMogg.hpp"
#include "ConvertPNG.hpp"

int main(int argc, const char * argv[]) {
    printf("start\n");

    ConvertPNG cm("/Users/tihmstar/Desktop/rb3/unpacked/songs/rhythmoflove1xx/gen/rhythmoflove1xx_keep.png_xbox");

    cm.convertToPS3("/tmp/kk/ps3.png");
    
    printf("done\n");
    return 0;
}
