//
//  main.cpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#include <iostream>
#include "ConvertMogg.hpp"

int main(int argc, const char * argv[]) {
    printf("start\n");

    ConvertMogg cm("/Users/tihmstar/Desktop/rb3/unpacked/songs/rhythmoflove1xx/rhythmoflove1xx.mogg");

    printf("got cryptversion=0x%X\n",cm.cryptversion());
    
    
    printf("done\n");
    return 0;
}
