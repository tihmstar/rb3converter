//
//  ConvertMid.hpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#ifndef NPD_hpp
#define NPD_hpp

#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <string>

struct NPD {
public:
    uint8_t magic[4];
    uint32_t version;
    uint32_t license;
    uint32_t type;
    uint8_t content_id[48];
    uint8_t digest[16];
    uint8_t titleHash[16];
    uint8_t devHash[16];
    uint8_t unknown[16];
    static NPD createNPD(uint8_t* bytes);
    static void createNPDHash1(std::string filename, uint8_t* npd_bytes, uint8_t* hash);
    static void createNPDHash2(uint8_t *klicensee, uint8_t* npd_bytes, uint8_t* hash);
    static NPD writeValidNPD(std::string filename, uint8_t* devKLic, uint8_t* rawnpd, uint8_t* contentID, uint8_t flags);
    bool validate();
};


#endif /* NPD_hpp */
