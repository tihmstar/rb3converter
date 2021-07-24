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
, _mem(NULL), _memSize(0)
, _ctx(NULL)
{
    _loader = new FileLoader(inpath);
    _mem = _loader->mem();
    _memSize = _loader->size();

    assure(_ctx = EVP_CIPHER_CTX_new());
    setupPS3EncryptKey();
}

ConvertMogg::ConvertMogg(const void *mem, size_t memSize)
: _loader(nullptr)
, _mem((const uint8_t*)mem), _memSize(memSize)
, _ctx(NULL)
{
    assure(_ctx = EVP_CIPHER_CTX_new());
    setupPS3EncryptKey();
}

ConvertMogg::~ConvertMogg(){
    safeFreeCustom(_ctx, EVP_CIPHER_CTX_free);
    safeDelete(_loader);
}

void ConvertMogg::setupPS3EncryptKey(){
    assure(EVP_EncryptInit_ex(_ctx, EVP_aes_128_ecb(), NULL, k, NULL) == 1);
    _streamP.clear();
    for (int i=0; i<16; i++) {
        _streamP.push_back(enc_ps3_header[i]);
    }
}

void ConvertMogg::encryptBodyBlock(void *dst, const void *src){
    uint8_t copy[0x10];
    int outl = 0;
    uint32_t *encnum = (uint32_t*)_streamP.data();
    {
        int64_t diff = (uint8_t*)dst - (uint8_t*)src;
        if (abs(diff) < 0x10) {
            memcpy(copy, src, 0x10);
            src = copy;
        }
    }
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
    size_t newmemsize_mapped = newmemsize;
    cleanup([&]{
        if (newmem){
            munmap(newmem, newmemsize_mapped);
            newmem = NULL;
        }
        if (nfd != -1 && newmemsize < newmemsize_mapped) {
            ftruncate(nfd, newmemsize);
        }
        safeClose(nfd);
    });
    const uint8_t *base = NULL;
    size_t baseSize = 0;
    size_t headerSizeMem = 0;
    size_t headerSizeNew = 0;
    bool isEncrypted = false;
    
    const uint8_t *srcmem = _mem;
    size_t srcmemSize = _memSize;


    assure((nfd = open(outpath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644)) != -1);
    assure(lseek(nfd, newmemsize-1, SEEK_SET) == newmemsize_mapped-1);
    {
        uint8_t nullbyte = 0;
        assure(write(nfd, &nullbyte, 1) == 1);
    }
    assure(lseek(nfd, 0, SEEK_SET) == 0);
    assure((newmem = (uint8_t*)mmap(NULL, newmemsize_mapped, PROT_READ | PROT_WRITE, MAP_SHARED, nfd, 0)) != (uint8_t*)-1);
    
    base = (const uint8_t *)(getItemPairs()+itemPairCnt());
    if (cryptversion() != ConvertMogg::CryptVersion::x0A) {
        uint32_t count = (cryptversion() == ConvertMogg::CryptVersion::x0B) ? 16 : 72;
        uint32_t num = *(uint32_t*)&srcmem[0x10];
        std::vector<uint8_t> key{&srcmem[0x14 + num*8],&srcmem[0x18 + num*8]+count};
        std::vector<uint8_t> kkk;
        
        if (cryptversion() == CryptVersion::x0C) {
            char hard_key[] = "\x2d\x68\x64\x6c\x94\x73\x17\x47\x97\x3d\x64\xdf\x89\x17\x63\x8b";
            kkk = {hard_key,hard_key+sizeof(hard_key)-1};
        }else if (cryptversion() == CryptVersion::x0D){
            char hard_key[] = "\xc0\x87\x69\x00\xe2\x7c\x73\xeb\xcc\xd4\x21\x3d\x70\x2a\x4f\xed";
            kkk = {hard_key,hard_key+sizeof(hard_key)-1};
        }else{
            reterror("unknown encryption 0x%x",cryptversion());
        }

        assure(EVP_EncryptInit_ex(_ctx, EVP_aes_128_ecb(), NULL, kkk.data(), NULL) == 1);
        _streamP = key;
        _streamP.resize(16);
        
        isEncrypted = true;
    }
    
    headerSizeNew = headerSizeMem = (base-srcmem);
    baseSize = srcmemSize - headerSizeMem;
    assure(baseSize && baseSize < srcmemSize);//overflow check
    
    memcpy(newmem, srcmem, headerSizeMem);

    //fixup encryption type
    ((uint32_t*)newmem)[0] = CryptVersion::x0D;

    if (!isEncrypted) {
        //fixup body size
        ((uint32_t*)newmem)[1] += sizeof(enc_ps3_header);
    }else{
        newmemsize -= sizeof(enc_ps3_header);;
    }
    
    //copy over enc_ps3_header
    memcpy(&newmem[headerSizeNew], enc_ps3_header, sizeof(enc_ps3_header));
    headerSizeNew +=sizeof(enc_ps3_header);
    if (isEncrypted) {
        headerSizeMem +=sizeof(enc_ps3_header);
    }

    //copy over first 4 blocks
    memcpy(&newmem[headerSizeNew], &srcmem[headerSizeMem], 32);
    
    if (isEncrypted) {
        uint8_t *dst = &newmem[headerSizeNew];
        const uint8_t *src = &srcmem[headerSizeMem];
        
        while (src+0x10 <= &srcmem[srcmemSize]) {
            encryptBodyBlock(dst,src);
            dst += 0x10;
            src += 0x10;
        }
        if (src < &srcmem[srcmemSize]) {
            size_t remainder = &srcmem[srcmemSize] - src;
            uint8_t block[0x10] = {};
            memcpy(block, src, remainder);
            encryptBodyBlock(block,block);
            memcpy(dst, block, remainder);
        }
        
        srcmem = newmem;
        srcmemSize = newmemsize;
        
        if (cryptversion() == CryptVersion::x0D) {
            retassure(*(uint32_t*)&newmem[headerSizeMem] == 'AXMH', "AXMH signature not found");
            //something obfuscated!?
            uint32_t num1 = htonl(*(uint32_t*)&srcmem[headerSizeMem + 12]);
            uint32_t num2 = htonl(*(uint32_t*)&srcmem[headerSizeMem + 20]);
            uint32_t num3 = *(uint32_t*)&_streamP.data()[24];

            uint32_t num4 = (num3 ^ 0x36363636) * 0x19660d + 0x3c6ef35f;

            uint32_t num5 = *(uint32_t*)&_streamP.data()[16];
            uint32_t num6 = (num5 ^ 0x5C5C5C5C) * 0x19660d + 0x3c6ef35f;

            num6 = num6 * 0x19660d + 0x3c6ef35f;
            num6 ^= num1;
            num4 ^= num2;

            *(uint32_t*)&newmem[headerSizeNew + 0] = 'SggO';
            *(uint32_t*)&newmem[headerSizeNew + 12/*0xc*/] = htonl(num6);
            *(uint32_t*)&newmem[headerSizeNew + 20/*0x14*/] = htonl(num4);
        }
        setupPS3EncryptKey();
    }

    retassure(*(uint32_t*)&newmem[headerSizeNew] == 'SggO', "OggS signature not found");
    
    {
        //something obfuscated!?
        uint32_t num1 = htonl(*(uint32_t*)&srcmem[headerSizeMem + 12]);
        uint32_t num2 = htonl(*(uint32_t*)&srcmem[headerSizeMem + 20]);
        uint32_t num3 = *(uint32_t*)&enc_ps3_header[24];

        uint32_t num4 = (num3 ^ 0x36363636) * 0x19660d + 0x3c6ef35f;

        uint32_t num5 = *(uint32_t*)&enc_ps3_header[16];
        uint32_t num6 = (num5 ^ 0x5C5C5C5C) * 0x19660d + 0x3c6ef35f;

        num6 = num6 * 0x19660d + 0x3c6ef35f;
        num6 ^= num1;
        num4 ^= num2;

        *(uint32_t*)&newmem[headerSizeNew + 0] = 'AXMH';
        *(uint32_t*)&newmem[headerSizeNew + 12/*0xc*/] = htonl(num6);
        *(uint32_t*)&newmem[headerSizeNew + 20/*0x14*/] = htonl(num4);
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
            ssize_t remaining = srcmemSize-(headerSizeMem+len);
            if (remaining < 16) {
                char buf_plain[16] = {};
                char buf_enc[16] = {};
                memcpy(buf_plain, &srcmem[headerSizeMem+len], remaining);
                encryptBodyBlock(buf_enc, buf_plain);
                memcpy(&newmem[headerSizeNew+len], buf_enc, remaining);
            }else{
                encryptBodyBlock(&newmem[headerSizeNew+len], &srcmem[headerSizeMem+len]);
            }
        }
    }
}
