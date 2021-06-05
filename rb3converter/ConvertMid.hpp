//
//  ConvertMid.hpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#ifndef ConvertMid_hpp
#define ConvertMid_hpp

#include "FileLoader.hpp"
#include "NPD.hpp"

class ConvertMid {
public:
    enum Region{
        Region_undefined = 0,
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
    FileLoader *_loader;
    const uint8_t *_mem; //don't free this!
    size_t _memSize;
    Region _region;
    std::string _contentId;
    uint8_t _key[16] = {0};
    uint8_t _rapKey[16] = {0};
    void readKLIC(std::string inpath);
    void readRapKey(std::string inpath);
    void createEdat1(const std::string& outpath);
    static void calculateBlockKeyEdat1(int blk, NPD npd, uint8_t *keyBuffer);

public:
    ConvertMid(std::string inpath, std::string klicpath, std::string rappath, Region region = Region::Region_PAL, Edat_type edattype = Edat_type::Edat_type_1);
    ConvertMid(const void *mem, size_t memSize, std::string klicpath, std::string rappath, Region region = Region::Region_PAL, Edat_type edattype = Edat_type::Edat_type_1);
    ~ConvertMid();

    void convertToPS3(std::string outpath);
    
};
#endif /* ConvertMid_hpp */
