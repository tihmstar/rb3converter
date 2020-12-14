//
// Created by malte on 12/14/20.
//

#include "NPD.hpp"
#include <libgeneral/macros.h>
#include <openssl/cmac.h>

NPD NPD::createNPD(uint8_t* bytes) {
    NPD npd;
    memcpy(npd.magic, bytes, 4);
    npd.version = htonl(*((uint32_t*) (bytes + 4)));
    npd.license = htonl(*((uint32_t*) (bytes + 8)));
    npd.type = htonl(*((uint32_t*) (bytes + 12)));
    memcpy(npd.content_id, bytes+16, 48);
    memcpy(npd.digest, bytes+64, 16);
    memcpy(npd.titleHash, bytes+80, 16);
    memcpy(npd.devHash, bytes+96, 16);
    memcpy(npd.unknown, bytes+112, 16);
    return npd;
}

bool NPD::validate() {
    return magic[0] == 78 && magic[1] == 80 && magic[2] == 68 && magic[3] == 0;
}

void NPD::createNPDHash1(std::string filename, uint8_t* npd_bytes, uint8_t* hash) {
    static const uint8_t npdrm_omac_key3[16] = {
        155,
        81,
        95,
        234,
        207,
        117,
        6,
        73,
        129,
        170,
        96,
        77,
        145,
        165,
        78,
        151
    };

    size_t len_dataToHash = filename.length() + 48;
    size_t mactlen;
    uint8_t* dataToHash = (uint8_t*)malloc(len_dataToHash);
    memcpy(dataToHash, npd_bytes+16, 48);
    memcpy(dataToHash+48, filename.c_str(), filename.length());
    CMAC_CTX *ctx = CMAC_CTX_new();
    assure(CMAC_Init(ctx, npdrm_omac_key3, 16, EVP_aes_128_cbc(), NULL) == 1);
    assure(CMAC_Update(ctx, dataToHash, len_dataToHash) == 1);
    assure(CMAC_Final(ctx, hash, &mactlen) == 1);
    CMAC_CTX_free(ctx);
    free(dataToHash);
    assure(mactlen == 16);
}
void NPD::createNPDHash2(uint8_t *klicensee, uint8_t* npd_bytes, uint8_t* hash) {
    static const uint8_t npdrm_omac_key2[16] = {
        107,
        165,
        41,
        118,
        239,
        218,
        22,
        239,
        60,
        51,
        159,
        178,
        151,
        30,
        37,
        107
    };

    size_t len_dataToHash = 96;
    size_t mactlen = 0;

    uint8_t key[16];
    for (int i = 0; i < 16; ++i) {
        key[i] = klicensee[i] ^ npdrm_omac_key2[i];
    }

    CMAC_CTX *ctx = CMAC_CTX_new();
    assure(CMAC_Init(ctx, key, 16, EVP_aes_128_cbc(), NULL) == 1);
    assure(CMAC_Update(ctx, npd_bytes, len_dataToHash) == 1);
    assure(CMAC_Final(ctx, hash, &mactlen) == 1);
    CMAC_CTX_free(ctx);
    assure(mactlen == 16);
}