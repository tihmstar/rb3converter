//
//  STFS.cpp
//  rb3converter
//
//  Created by tihmstar on 30.12.20.
//

#include "STFS.hpp"
#include <libgeneral/macros.h>

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
}

STFS::~STFS(){
    //
}
