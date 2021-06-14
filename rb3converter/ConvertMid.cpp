//
//  ConvertMid.cpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#include "ConvertMid.hpp"
#include "NPD.hpp"
#include <libgeneral/macros.h>
#include <sys/mman.h>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <filesystem>
#include <utility>
#include <openssl/cmac.h>

#define NPD_SIZE (128)
#define EDAT_BLOCKSIZE (16384)
#define FLAG_COMPRESSED (1u)
#define FLAG_SDAT (16777216u)
#define EDAT_DATA_START (256)
#define HEADER_MAX_BLOCKSIZE (15360)

static const uint8_t SDATKEY[17] = "\x0d\x65\x5e\xf8\xe6\x74\xa9\x8a\xb8\x50\x5c\xfa\x7d\x01\x29\x33";

struct EDATData {
    uint32_t flags;
    uint32_t blockSize;
    uint64_t fileLen;
    /*
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
     */
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
        bytes[i] = (hexNibbleToInt(upper)<<4u) | hexNibbleToInt(lower);
    }
}

void findAndReplaceAll(std::string & data, std::string toSearch, std::string replaceStr){
    size_t pos = data.find(toSearch);
    while( pos != std::string::npos){
        data.replace(pos, toSearch.size(), replaceStr);
        pos = data.find(toSearch, pos + replaceStr.size());
    }
}

void apploaderReverse(uint8_t *input_block, uint8_t *output_block, int input_block_len, uint8_t* key, uint8_t *iv, uint8_t *hashKey, uint8_t *hashOutput) {
    EVP_CIPHER_CTX *ctx = NULL;
    cleanup([&]{
        safeFreeCustom(ctx, EVP_CIPHER_CTX_free);
    });
    int ciphertext_len = 0;
    int len = 0;

    // encryption using aes-128-cbc
    assure((ctx = EVP_CIPHER_CTX_new()) != nullptr );
    assure(EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, key, iv) == 1);
    assure(EVP_CIPHER_CTX_set_padding(ctx, 0) == 1);
    assure(EVP_EncryptUpdate(ctx, output_block, &len, input_block, input_block_len) == 1);
    ciphertext_len = len;
    assure(EVP_EncryptFinal_ex(ctx, output_block + len, &len) == 1);
    ciphertext_len += len;
    assure(ciphertext_len == input_block_len);

    // hash using CMAC of length 16
    {
        CMAC_CTX *cmac_ctx = NULL;
        cleanup([&]{
            safeFreeCustom(cmac_ctx, CMAC_CTX_free);
        });
        size_t mactlen = 0;
        
        assure(cmac_ctx = CMAC_CTX_new());
        assure(CMAC_Init(cmac_ctx, hashKey, 16, EVP_aes_128_cbc(), nullptr) == 1);
        assure(CMAC_Update(cmac_ctx, output_block, ciphertext_len) == 1);
        assure(CMAC_Final(cmac_ctx, hashOutput, &mactlen) == 1);
        assure(mactlen == 16);
    }
}

void apploaderUnencryptedInit(CMAC_CTX *cmac_ctx, uint8_t* hashKey) {
    assure(CMAC_Init(cmac_ctx, hashKey, 16, EVP_aes_128_cbc(), nullptr) == 1);
}

void apploaderUnencryptedUpdate(CMAC_CTX *cmac_ctx, const uint8_t *data, size_t len) {
    assure(CMAC_Update(cmac_ctx, data, len) == 1);
}

void apploaderUnencryptedFinal(CMAC_CTX *cmac_ctx, uint8_t* hashOutput) {
    size_t mactlen = 0;
    assure(CMAC_Final(cmac_ctx, hashOutput, &mactlen) == 1);
    assure(mactlen == 16);
}

void apploaderUnencrypted(uint8_t* data, size_t len, uint8_t* hashKey, uint8_t* hashOutput) {
    CMAC_CTX *cmac_ctx = NULL;
    cleanup([&]{
        safeFreeCustom(cmac_ctx, CMAC_CTX_free);
    });
    assure(cmac_ctx = CMAC_CTX_new());
    apploaderUnencryptedInit(cmac_ctx, hashKey);
    apploaderUnencryptedUpdate(cmac_ctx, data, len);
    apploaderUnencryptedFinal(cmac_ctx, hashOutput);
}

ConvertMid::ConvertMid(std::string inpath, std::string klicpath, std::string rappath, Region region, Edat_type edattype)
: _loader(nullptr)
, _mem(NULL), _memSize(NULL)
, _region(region)
{
    _loader = new FileLoader(inpath);
    _mem = _loader->mem();
    _memSize = _loader->size();

    readKLIC(klicpath);
    readRapKey(rappath);
}

ConvertMid::ConvertMid(const void *mem, size_t memSize, std::string klicpath, std::string rappath, Region region, Edat_type edattype)
: _loader(nullptr)
, _mem((const uint8_t*)mem), _memSize(memSize)
, _region(region)
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
    findAndReplaceAll(_contentId, "\r", "");
    findAndReplaceAll(_contentId, "\n", "");
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

    uint8_t encryptedRapKey[16] = {};

    {
        std::ifstream infile;
        infile.open(inpath, std::ios::binary | std::ios::in);
        infile.read((char*)encryptedRapKey, 16);
        infile.close();
    }
    
    {
        EVP_CIPHER_CTX *ctx = NULL;
        cleanup([&]{
            safeFreeCustom(ctx, EVP_CIPHER_CTX_free);
        });
        int plaintext_len = 0;
        int len = 0;
        assure((ctx = EVP_CIPHER_CTX_new()) != nullptr);
        assure(EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), nullptr, rapKey, nullptr) == 1);
        assure(EVP_CIPHER_CTX_set_padding(ctx, 0) == 1);
        assure(EVP_DecryptUpdate(ctx, _rapKey, &len, encryptedRapKey, 16) == 1);
        plaintext_len = len;
        assure(EVP_DecryptFinal_ex(ctx, _rapKey + len, &len) == 1);
        plaintext_len += len;
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
}

void ConvertMid::createEdat1(const std::string& outpath) {
    uint8_t *npdBuffer = nullptr;
    uint8_t *encryptedBlockCMACs = nullptr;
    uint8_t *encryptedBlocks = nullptr;
    cleanup([&]{
        safeFree(npdBuffer);
        safeFree(encryptedBlockCMACs);
        safeFree(encryptedBlocks);
    });
    uint8_t encryptionKey[16] = {0};
    size_t blockCount = (_memSize + EDAT_BLOCKSIZE - 1) / EDAT_BLOCKSIZE;
    EDATData edatData = {0, EDAT_BLOCKSIZE, _memSize};

    npdBuffer = (uint8_t*)malloc(NPD_SIZE); // this is dirty but a valid NPD seems to have a fixed size
    memset(npdBuffer, 0, NPD_SIZE);

    encryptedBlockCMACs = (uint8_t*)malloc(blockCount * 16);
    encryptedBlocks = (uint8_t*)malloc(_memSize + 15);
    
    std::fstream outfile (outpath, std::ios::in | std::ios::out | std::ofstream::binary |  std::fstream::trunc);
    NPD dummyNPD = NPD::writeValidNPD(std::filesystem::path(outpath).filename(), _key, npdBuffer, _contentId, 0x00);
    
    assure(dummyNPD.validate());
    
    outfile.write((char*)npdBuffer, NPD_SIZE);
    outfile.write("\x00\x00\x00\x00", 4);
    outfile.write("\x00\x00\x40\x00", 4);
    {
        uint64_t bswappedFileSize = swapByteOrder64(_memSize);
        outfile.write((char*)&bswappedFileSize, 8);
    }
    outfile.seekp(EDAT_DATA_START); // the data seems to be padded to 256 bytes

    // derive encryption key
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
        reterror("there is no valid encryption key available");
    }

    // this is where the real encryption is performed (compare with EDAT.cs -> encryptData(...))
    for (int i = 0; i < blockCount; ++i) {
        uint8_t inputBlock[EDAT_BLOCKSIZE + 15] = {};
        uint8_t outputBlock[EDAT_BLOCKSIZE + 15] = {};
        uint8_t blockKey[20] = {};
        uint8_t encryptedBlockKey[16] = {};
        uint8_t blockCmac[16] = {};

        size_t block_offset = i * EDAT_BLOCKSIZE;
        size_t plaintextBytesForThisBlock = (i != (blockCount-1)) ? EDAT_BLOCKSIZE : _memSize % EDAT_BLOCKSIZE;
        size_t readSize = (plaintextBytesForThisBlock + 15) & ~0xf;
        int len = 0;

        memcpy(inputBlock, _mem + block_offset, plaintextBytesForThisBlock);
        calculateBlockKeyEdat1(i, dummyNPD, blockKey);

        {
            // encrypt the block key
            EVP_CIPHER_CTX *ctx = NULL;
            cleanup([&]{
                safeFreeCustom(ctx, EVP_CIPHER_CTX_free);
            });
            int ciphertext_len = 0;
            assure(ctx = EVP_CIPHER_CTX_new());
            assure(EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), nullptr, encryptionKey, nullptr) == 1);
            assure(EVP_CIPHER_CTX_set_padding(ctx, 0) == 1);
            assure(EVP_EncryptUpdate(ctx, encryptedBlockKey, &len, blockKey, 16) == 1);
            ciphertext_len = len;
            assure(EVP_EncryptFinal_ex(ctx, encryptedBlockKey + len, &len) == 1);
            ciphertext_len += len;
            assure(ciphertext_len == 16);
        }

        // encrypt the data itself
        apploaderReverse(inputBlock, outputBlock, (int)readSize, encryptedBlockKey, dummyNPD.digest, encryptedBlockKey, blockCmac);
        memcpy(encryptedBlocks+block_offset, outputBlock, readSize);
        memcpy(encryptedBlockCMACs+(i*16), blockCmac, 16);
    }
    outfile.write((char*)encryptedBlockCMACs, blockCount * 16);
    outfile.write((char*)encryptedBlocks, _memSize + 15);
        

    {
        // calculate CMAC of encrypted blocks and blockwise CMACS
        CMAC_CTX *cmac_ctx = NULL;
        cleanup([&]{
            safeFreeCustom(cmac_ctx, CMAC_CTX_free);
        });
        uint8_t dataCMAC[16] = {};
        uint8_t cmacDataBlock[HEADER_MAX_BLOCKSIZE] = {};
        size_t bytes_read = 0;
        size_t amount_of_blocks = (edatData.fileLen + edatData.blockSize - 11) / edatData.blockSize;
        size_t blocksize = edatData.flags & FLAG_COMPRESSED ? 32 : 16;
        size_t bytes_to_read = amount_of_blocks * blocksize;

        assure(cmac_ctx = CMAC_CTX_new());
        apploaderUnencryptedInit(cmac_ctx, encryptionKey);
        while (bytes_to_read > 0) {
            size_t bytes_to_read_in_this_iteration = bytes_to_read > HEADER_MAX_BLOCKSIZE ? HEADER_MAX_BLOCKSIZE : bytes_to_read;
            outfile.seekg(EDAT_DATA_START + bytes_read);
            outfile.read((char*)cmacDataBlock, bytes_to_read_in_this_iteration);
            apploaderUnencryptedUpdate(cmac_ctx, cmacDataBlock, bytes_to_read_in_this_iteration);
            bytes_read += bytes_to_read_in_this_iteration;
            bytes_to_read -= bytes_to_read_in_this_iteration;
        }
        apploaderUnencryptedFinal(cmac_ctx, dataCMAC);
        outfile.flush();
        outfile.seekp(144);
        outfile.write((char*)dataCMAC, 16);
    }

    outfile.flush();

    {
        // calculate CMAC of header
        uint8_t file_header[160] = {};
        uint8_t file_header_cmac[16] = {};
        outfile.seekg(0);
        outfile.read((char*)file_header, 160);
        apploaderUnencrypted(file_header, 160, encryptionKey, file_header_cmac);
        outfile.seekp(160);
        outfile.write((char*)file_header_cmac, 16);
    }

    outfile.close();
}

void ConvertMid::calculateBlockKeyEdat1(int blk, NPD npd, uint8_t *keyBuffer) {
    if (npd.version > 1) {
        memcpy(keyBuffer, npd.devHash, 12);
    } else {
        bzero(keyBuffer, 16);
    }
    ((uint32_t*)keyBuffer)[3] = htonl(blk);
}

void ConvertMid::convertToPS3(std::string outpath) {
    createEdat1(outpath);
}

