//
//  ConvertMid.cpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#include "ConvertMid.hpp"
#include "NPD.hpp"
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
#include <filesystem>

#define NPD_SIZE (128)
#define EDAT_BLOCKSIZE (16384)
#define FLAG_SDAT (16777216)

static const uint8_t SDATKEY[17] = "\x0d\x65\x5e\xf8\xe6\x74\xa9\x8a\xb8\x50\x5c\xfa\x7d\x01\x29\x33";

struct EDATData {
    uint32_t flags;
    uint32_t blockSize;
    uint64_t fileLen;
    static EDATData createEDATData(uint8_t* data, size_t len) {
        // TODO: test if byteswap is necessary
        assure(len >= 16);
        EDATData edatData = {
                ((uint32_t*)data)[0],
                ((uint32_t*)data)[1],
                ((uint64_t *)data)[1],
        };
        return edatData;

    }
};

uint64_t swapByteOrder64(uint64_t in) {
#if __has_builtin(__builtin_bswap64)
    return __builtin_bswap64(in);
#else
    return ((((in) & 0xff00000000000000ull) >> 56)                                      \
      | (((in) & 0x00ff000000000000ull) >> 40)                                      \
      | (((in) & 0x0000ff0000000000ull) >> 24)                                      \
      | (((in) & 0x000000ff00000000ull) >> 8)                                      \
      | (((in) & 0x00000000ff000000ull) << 8)                                      \
      | (((in) & 0x0000000000ff0000ull) << 24)                                      \
      | (((in) & 0x000000000000ff00ull) << 40)                                      \
      | (((in) & 0x00000000000000ffull) << 56))
#endif
}

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
    std::getline(_is, _contentId);
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

void ConvertMid::createEdat1(std::string outpath) {
    uint8_t *npdBuffer = NULL;
    uint8_t *encryptedData = NULL;
    uint8_t *renameMe = NULL;
    uint8_t edatBlock[EDAT_BLOCKSIZE];
    cleanup([&]{
        safeFree(npdBuffer);
        safeFree(encryptedData);
        safeFree(renameMe);
    });

    size_t blockCount = (_memSize + EDAT_BLOCKSIZE - 1) / EDAT_BLOCKSIZE;
    npdBuffer = (uint8_t*)malloc(NPD_SIZE); // this is dirty but a valid NPD seems to have a fixed size
    encryptedData = (uint8_t*)malloc(blockCount * EDAT_BLOCKSIZE);
    renameMe = (uint8_t*)malloc(_memSize + 15);
    std::ofstream outfile (outpath, std::ofstream::binary);
    NPD dummyNPD = NPD::writeValidNPD(std::filesystem::path(outpath).filename(), _key, npdBuffer, (uint8_t*)_contentId.c_str(), 0x00);
    assure(dummyNPD.validate());
    outfile.write((char*)npdBuffer, NPD_SIZE);
    outfile.write("\x00\x00\x00\x00", 4);
    outfile.write("\x00\x00\x40\x00", 4);
    uint64_t bswappedFileSize = swapByteOrder64(_memSize);
    outfile.write((char*)&bswappedFileSize, 8);
    outfile.seekp(256); // the data seems to be padded to 256 bytes
    EDATData edatData = {0, EDAT_BLOCKSIZE, _memSize};

    // derive encryption key
    uint8_t encryptionKey[16] = {0};
    if(edatData.flags & FLAG_SDAT) {
        for (int i = 0; i < 16; ++i) {
            encryptionKey[i] = dummyNPD.devHash[i] ^ SDATKEY[i];
        }
    } else if(dummyNPD.license == 3) {
        memcpy(encryptionKey, _key, 16);
    } else if(dummyNPD.license == 2) {
        memcpy(encryptionKey, _rapKey, 16);
    } else {
        // there is no valid encryption key available
        assure(0);
    }

    // this is where the real encryption is performed (compare with EDAT.cs -> encryptData(...))
    for (int i = 0; i < blockCount; ++i) {
        size_t block_offset = i * EDAT_BLOCKSIZE;
        size_t plaintextBytesForThisBlock = (i != (blockCount-1)) ? EDAT_BLOCKSIZE : _memSize % EDAT_BLOCKSIZE;
        size_t readSize = (plaintextBytesForThisBlock + 15) & 0xfffffffffffffff0;
    }

    outfile.close();
}
