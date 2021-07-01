//
//  ConvertMogg.cpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#include "ConvertMogg.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <libgeneral/macros.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <vector>
#include <string.h>

static uint8_t enc_ps3_header[72] = {
    0x00, 0x00, 0x00, 0x00, 0x34, 0x96, 0xc1, 0x1f, 0x05, 0x09, 0x12, 0x66,
    0x9e, 0x20, 0x09, 0xef, 0x30, 0x8b, 0xa2, 0x21, 0x00, 0x00, 0x00, 0x00,
    0x4d, 0x12, 0x6a, 0x47, 0x00, 0x00, 0x00, 0x00, 0xac, 0x05, 0x7d, 0xf0,
    0x8b, 0x29, 0x1d, 0x90, 0x2c, 0x58, 0x7a, 0x0c, 0x10, 0xc3, 0x53, 0xeb,
    0x4e, 0x64, 0xea, 0xda, 0x41, 0xc8, 0xa5, 0xec, 0x42, 0x26, 0x7f, 0x00,
    0x04, 0xf0, 0x55, 0x27, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint8_t k[16] = {
    0x31, 0x98, 0xe0, 0x6d, 0x16, 0xb4, 0x80, 0xd, 0xb1, 0xca, 0xf9, 0x2c, 0xd8, 0xd5, 0x16, 0x82
};

ConvertMogg::ConvertMogg(std::string inpath)
: _loader(nullptr)
, _mem(NULL), _memSize(NULL)
, _ctx(NULL)
{
    _loader = new FileLoader(inpath);
    _mem = _loader->mem();
    _memSize = _loader->size();

    assure(_ctx = EVP_CIPHER_CTX_new());
    assure(EVP_EncryptInit_ex(_ctx, EVP_aes_128_ecb(), NULL, k, NULL) == 1);

    for (int i=0; i<16; i++) {
        _streamP.push_back(enc_ps3_header[i]);
    }
}

ConvertMogg::ConvertMogg(const void *mem, size_t memSize)
: _loader(nullptr)
, _mem((const uint8_t*)mem), _memSize(memSize)
, _ctx(NULL)
{
    assure(_ctx = EVP_CIPHER_CTX_new());
    assure(EVP_EncryptInit_ex(_ctx, EVP_aes_128_ecb(), NULL, k, NULL) == 1);

    for (int i=0; i<16; i++) {
        _streamP.push_back(enc_ps3_header[i]);
    }
}

ConvertMogg::~ConvertMogg(){
    safeFreeCustom(_ctx, EVP_CIPHER_CTX_free);
    safeDelete(_loader);
}

void ConvertMogg::encryptBodyBlock(const void *dst, const void *src){
    int outl = 0;
    uint32_t *encnum = (uint32_t*)_streamP.data();
    assure(EVP_EncryptUpdate(_ctx, (unsigned char*)dst, &outl, (unsigned char*)_streamP.data(), 16) == 1);
    (*encnum)++;
    if (((uint64_t)src & 0xf) == 0 && ((uint64_t)dst & 0xf) == 0){
        uint64_t *srcp = (uint64_t*)src;
        uint64_t *dstp = (uint64_t*)dst;
        dstp[0] ^= srcp[0];
        dstp[1] ^= srcp[1];
    } else if (((uint64_t)src & 0x3) == 0 && ((uint64_t)dst & 0x3) == 0){
        uint32_t *srcp = (uint32_t*)src;
        uint32_t *dstp = (uint32_t*)dst;
        dstp[0] ^= srcp[0];
        dstp[1] ^= srcp[1];
        dstp[2] ^= srcp[2];
        dstp[3] ^= srcp[3];
    } else {
        uint8_t *srcp = (uint8_t*)src;
        uint8_t *dstp = (uint8_t*)dst;
        dstp[0] ^= srcp[0];
        dstp[1] ^= srcp[1];
        dstp[2] ^= srcp[2];
        dstp[3] ^= srcp[3];
        dstp[4] ^= srcp[4];
        dstp[5] ^= srcp[5];
        dstp[6] ^= srcp[6];
        dstp[7] ^= srcp[7];
        dstp[8] ^= srcp[8];
        dstp[9] ^= srcp[9];
        dstp[10] ^= srcp[10];
        dstp[11] ^= srcp[11];
        dstp[12] ^= srcp[12];
        dstp[13] ^= srcp[13];
        dstp[14] ^= srcp[14];
        dstp[15] ^= srcp[15];
    }
}


const ConvertMogg::CryptVersion ConvertMogg::cryptversion() const{
    return (ConvertMogg::CryptVersion)_mem[0];
}

const uint32_t ConvertMogg::bodySize() const{
    return ((uint32_t*)_mem)[1];
}


const uint32_t ConvertMogg::itemPairCnt() const{
    return ((uint32_t*)_mem)[4];
}

const ConvertMogg::ItemPair *ConvertMogg::getItemPairs() const{
    return (ItemPair*)&_mem[0x14];
}

void ConvertMogg::convertToPS3(std::string outpath){
    int nfd = -1;
    uint8_t *newmem = NULL;
    size_t newmemsize = _memSize+sizeof(enc_ps3_header);
    cleanup([&]{
        if (newmem){
            munmap(newmem, newmemsize);
            newmem = NULL;
        }
        if (nfd != -1) {
            close(nfd);
        }
    });
    const uint8_t *base = NULL;
    size_t baseSize = 0;
    size_t headerSizeMem = 0;
    size_t headerSizeNew = 0;

    assure((nfd = open(outpath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0655)) != -1);
    assure(lseek(nfd, newmemsize-1, SEEK_SET) == newmemsize-1);
    {
        uint8_t nullbyte = 0;
        assure(write(nfd, &nullbyte, 1) == 1);
    }
    assure(lseek(nfd, 0, SEEK_SET) == 0);
    assure((newmem = (uint8_t*)mmap(NULL, newmemsize, PROT_READ | PROT_WRITE, MAP_SHARED, nfd, 0)) != (uint8_t*)-1);
    
    base = (const uint8_t *)(getItemPairs()+itemPairCnt());
    if (cryptversion() != ConvertMogg::CryptVersion::x0A) {
        base += (cryptversion() == ConvertMogg::CryptVersion::x0B) ? 16 : 72;
    }
    
    headerSizeNew = headerSizeMem = (base-_mem);
    baseSize = _memSize - headerSizeMem;
    assure(baseSize && baseSize < _memSize);//overflow check
    
    memcpy(newmem, _mem, headerSizeMem);

    //fixup encryption type
    ((uint32_t*)newmem)[0] = CryptVersion::x0D;

    //fixup body size
    ((uint32_t*)newmem)[1] +=sizeof(enc_ps3_header);
    
    //copy over enc_ps3_header
    memcpy(&newmem[headerSizeNew], enc_ps3_header, sizeof(enc_ps3_header));
    headerSizeNew +=sizeof(enc_ps3_header);
    
    //copy over first 4 blocks
    memcpy(&newmem[headerSizeNew], &_mem[headerSizeMem], 32);

    
    {
        //something obfuscated!?
        uint32_t num1 = htonl(*(uint32_t*)&_mem[headerSizeMem + 12]);
        uint32_t num2 = htonl(*(uint32_t*)&_mem[headerSizeMem + 20]);
        uint32_t num3 = *(uint32_t*)&enc_ps3_header[24];

        uint32_t num4 = (num3 ^ 0x36363636) * 0x19660d + 0x3c6ef35f;
        
        uint32_t num5 = *(uint32_t*)&enc_ps3_header[16];
        uint32_t num6 = (num5 ^ 0x5C5C5C5C) * 0x19660d + 0x3c6ef35f;

        num6 = num6 * 0x19660d + 0x3c6ef35f;
        num6 ^= num1;
        num4 ^= num2;

        *(uint32_t*)&newmem[headerSizeNew + 0] = 'AXMH';
        *(uint32_t*)&newmem[headerSizeNew + 12] = htonl(num6);
        *(uint32_t*)&newmem[headerSizeNew + 20] = htonl(num4);
    }

    {
        size_t newBodySize = newmemsize - headerSizeNew;
        int len = 0;
        for (; len < newBodySize && len < 32; len+=16) {
            uint8_t tmp[16];
            encryptBodyBlock(tmp, &newmem[headerSizeNew+len]);
            memcpy(&newmem[headerSizeNew+len], tmp, 16);
        }
        for (; len < newBodySize; len+=16) {
            ssize_t remaining = _memSize-(headerSizeMem+len);
            if (remaining < 16) {
                char buf_plain[16] = {};
                char buf_enc[16] = {};
                memcpy(buf_plain, &_mem[headerSizeMem+len], remaining);
                encryptBodyBlock(buf_enc, buf_plain);
                memcpy(&newmem[headerSizeNew+len], buf_enc, remaining);
            }else{
                encryptBodyBlock(&newmem[headerSizeNew+len], &_mem[headerSizeMem+len]);
            }
        }
    }
}
