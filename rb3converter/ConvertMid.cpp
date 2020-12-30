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
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

uint8_t hexNibbleToInt(char nibble) {
    if (nibble >= '0' && nibble <= '9') {
        return nibble-0x30;
    } else if(nibble >= 'A' && nibble <= 'F') {
        return nibble-0x37;
    } else if(nibble >= 'a' && nibble <= 'f') {
        return nibble-0x57;
    } else {
        throw;
    }
}

void hexstringToBytes(const char* string, uint8_t* bytes, size_t numberofbytes) {
    assure(strlen(string) >= numberofbytes*2);

    for (int i = 0; i < numberofbytes; ++i) {
        char upper = string[i*2];
        char lower = string[i*2+1];
        bytes[i] = (hexNibbleToInt(upper)<<4) | hexNibbleToInt(lower);
    }
}

ConvertMid::ConvertMid(std::string inpath, std::string klicpath, std::string rappath, Region region, Edat_type edattype)
: FileLoader(inpath), _region(region)
{
    readKLIC(klicpath);
    readRapKey(rappath);
}

ConvertMid::~ConvertMid(){
    //
}

void ConvertMid::readKLIC(std::string inpath){
    std::string line;
    std::string contentid;
    std::string keyString;
    std::ifstream _is (inpath, std::ifstream::binary);
    std::getline(_is, line);
    std::getline(_is, line);
    std::getline(_is, line);
    std::getline(_is, line);
    if(_region == Region_PAL) {
        std::getline(_is, line);
        std::getline(_is, line);
        std::getline(_is, line);
        std::getline(_is, line);
    }
    std::getline(_is, contentid);
    std::getline(_is, line);
    std::getline(_is, keyString);
    hexstringToBytes(keyString.c_str(), _key, 16);
}

void ConvertMid::readRapKey(std::string inpath){
    static const uint8_t rapKey[16] = {
        134, 159, 119, 69, 193, 63, 216, 144, 204, 242, 145, 136, 227, 204, 62, 223
    };
    static const uint8_t indexTable[16] ={
        12, 3, 6, 4, 1, 11, 15, 8, 2, 7, 0, 5, 10, 14, 13, 9
    };

    static const uint8_t key1[16] {
        169, 62, 31, 214, 124, 85, 163, 41, 183, 95, 221, 166, 42, 149, 199, 165
    };

    static const uint8_t key2[16] = {
        103, 212, 93, 163, 41, 109, 0, 106, 78, 124, 83, 123, 245, 83, 140, 116
    };


    std::ifstream infile;
    infile.open(inpath, std::ios::binary | std::ios::in);
    uint8_t encryptedRapKey[16];
    infile.read((char*)encryptedRapKey, 16);
    infile.close();
    int plaintext_len = 0, len;

    EVP_CIPHER_CTX *ctx;
    assure((ctx = EVP_CIPHER_CTX_new()) != NULL);
    assure(EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, rapKey, NULL) == 1);
    assure(EVP_CIPHER_CTX_set_padding(ctx, NULL) == 1);
    assure(EVP_DecryptUpdate(ctx, _rapKey, &len, encryptedRapKey, 16) == 1);
    plaintext_len = len;
    assure(EVP_DecryptFinal_ex(ctx, _rapKey + len, &len) == 1);
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);
    assure(plaintext_len == 16);

    for(int i = 0; i < 5; i++) {
        for(int j = 0; j < 16; j++) {
            int num = indexTable[j];
            _rapKey[num] = _rapKey[num] ^ key1[num];
        }
        for (int num2 = 15; num2 > 0; num2--)
        {
            int num3 = indexTable[num2];
            int num4 = indexTable[num2 - 1];
            _rapKey[num3] = _rapKey[num3] ^ _rapKey[num4];
        }
        int num5 = 0;
        for (int k = 0; k < 16; k++)
        {
            int num6 = indexTable[k];
            uint8_t b = _rapKey[num6] = _rapKey[num6] - num5;
            if (num5 != 1 || b != 0xff)
            {
                int num7 = b;
                int num8 = key2[num6];
                num5 = (num7 < num8);
            }
            _rapKey[num6] = b - key2[num6];
        }
    }
}
