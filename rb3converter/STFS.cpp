//
//  STFS.cpp
//  rb3converter
//
//  Created by tihmstar on 30.12.20.
//

#include "STFS.hpp"
#include <libgeneral/macros.h>
#include <sys/stat.h>

#define BLOCKSIZE 0x1000

STFS::STFS(std::string inpath)
: FileLoader(inpath),
    _head(NULL),_meta(NULL), _block_number(0), _block_count(0)
{

    assure(_memSize >= sizeof(header));
    _head = (const header*)_mem;
    
    retassure(strncmp(_head->magic, "CON ", 4) == 0
           || strncmp(_head->magic, "PIRS", 4) == 0
           || strncmp(_head->magic, "LIVE", 4) == 0,"Invalid header magic");

    assure(_memSize >= sizeof(header)+sizeof(metadata));
    
    if (strncmp(_head->magic, "CON ", 4) != 0) {
        reterror("Only CON implemented");
    }
    
    _meta = (const metadata*)(_head+1);
    
    if (_meta->Descriptor_type == Descriptor_type_STFS) {
        const uint8_t *bn = _meta->Volume_Descriptor.STFS.File_Table_Block_Number;
        
        _block_number = 0;
        for (int i=0; i<sizeof(_meta->Volume_Descriptor.STFS.File_Table_Block_Number); i++) {
            _block_number <<=8; _block_number |= bn[i];
        }
        _block_count = _meta->Volume_Descriptor.STFS.File_Table_Block_Count;
    }else{
        reterror("not implemented");
    }
    
    parseFiletable(_block_number, _block_count);
}

STFS::~STFS(){
    //
}

const uint8_t *STFS::getblock(uint16_t blocknum){
    uint32_t headerSize = htonl(_meta->HeaderSize);
    uint32_t blockOffset = (headerSize + 0xfff) & 0xF000;
    uint32_t hashblocks = (blocknum/170);
    uint32_t hashhashblocks = (hashblocks/170)+1;
    
    if (hashblocks) hashblocks++;
    
    uint32_t fixed_blocknum = blocknum+hashblocks+hashhashblocks;
    
    uint64_t finalOffset = blockOffset + fixed_blocknum*BLOCKSIZE;
    assure(finalOffset < _memSize);
    return _mem+finalOffset;
}

const STFS::hashTableEntry *STFS::gethashtable(uint16_t blocknum){
    uint32_t headerSize = htonl(_meta->HeaderSize);
    uint32_t blockOffset = (headerSize + 0xfff) & 0xF000;
    uint32_t hashblocks = (blocknum/170);
    uint32_t hashhashblocks = (hashblocks/170);

    uint32_t record = blocknum % 170;
    
    uint32_t fixed_blocknum = hashhashblocks+170*hashblocks+hashblocks;
    if (fixed_blocknum) fixed_blocknum++;

    uint64_t finalOffset = +blockOffset + fixed_blocknum*BLOCKSIZE;
    assure(finalOffset < _memSize);

    hashTableEntry *entry = (hashTableEntry *)(_mem+finalOffset);
    return &entry[record];
}


void STFS::parseFiletable(uint32_t startBlock, uint32_t numBlocks){
    for (uint16_t i=0; i<numBlocks; i++) {
        const uint8_t *block = getblock(startBlock);
        const hashTableEntry *hashtable = gethashtable(startBlock);
        cleanup([&]{
            //prepare for next iteration
            uint32_t nextBlock = (hashtable->nextBlock[0] << 16) | (hashtable->nextBlock[1] << 8) | (hashtable->nextBlock[2] << 0);
            startBlock = nextBlock;
        });

        for (uint32_t z=0; z<BLOCKSIZE; z+=sizeof(fileRecord)) {
            const fileRecord *file = (const fileRecord *)&block[z];
            if (!file->type) continue;
            
            std::string filename;
            int16_t pathindex = htons(file->pathindex);
            if (pathindex != -1 && pathindex < _files.size()) {
                const fileRecord *otherfile = (const fileRecord *)&block[sizeof(fileRecord)*pathindex];
                for (auto f : _files) {
                    if (f.second == otherfile) {
                        filename += f.first;
                        goto found_filename;
                    }
                }
                reterror("Internal error constructing file path");
            }
        found_filename:
            filename += file->filename;
            if (file->type & type_directory) filename += '/';
            
            _files[filename] = file;
        }
    }
}

std::vector<uint8_t> STFS::getFile(const fileRecord *file){
    std::vector<uint8_t> buf;
    uint32_t fileSize = htonl(file->size);
    uint32_t firstBlock = (file->firstblock[2] << 16) | (file->firstblock[1] << 8) | (file->firstblock[0] << 0);
    uint32_t numBlocks = (file->numblocks[2] << 16) | (file->numblocks[1] << 8) | (file->numblocks[0] << 0);
    
    buf.resize(fileSize);
    
    for (uint32_t i=0; i<numBlocks; i++) {
        const hashTableEntry *hashtable = gethashtable(firstBlock);
        cleanup([&]{
            //prepare for next iteration
            uint32_t nextBlock = (hashtable->nextBlock[0] << 16) | (hashtable->nextBlock[1] << 8) | (hashtable->nextBlock[2] << 0);
            firstBlock = nextBlock;
        });
        assure(hashtable->status == HashTableStatus_used);
        const uint8_t *block = getblock(firstBlock);
        uint32_t copySize = fileSize;
        if (copySize > BLOCKSIZE) {
            copySize = BLOCKSIZE;
        }
        fileSize -= copySize;
        memcpy(&buf.data()[i*BLOCKSIZE], block, copySize);
    }
    
    return buf;
}

void STFS::extract_all(std::string outputPath){
    if (outputPath.back() != '/') outputPath += '/';
    
    for (auto &file : _files) {
        std::string filepath = outputPath + file.first;
        if (filepath.back() == '/') {
            printf("creating directory=%s\n",filepath.c_str());
            retassure(!mkdir(filepath.c_str(), 0755) || errno == EEXIST, "Failed to create out directory '%s'",filepath.c_str());
        }
    }
    
    for (auto &file : _files) {
        std::string filepath = outputPath + file.first;
        if (filepath.back() == '/') continue;
        FILE *f = NULL;
        cleanup([&]{
            safeFreeCustom(f, fclose);
        });
        printf("writing file=%s\n",filepath.c_str());
        auto data = getFile(file.second);
        
        retassure(f = fopen(filepath.c_str(), "wb"), "Failed to open target file");
        retassure(fwrite(data.data(), 1, data.size(), f) == data.size(),"Failed to write target file");
    }
}
