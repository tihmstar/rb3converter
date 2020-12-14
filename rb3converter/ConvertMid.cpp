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
: FileLoader(inpath), _region(region)
{
    //
}

ConvertMid::~ConvertMid(){
    //
}
