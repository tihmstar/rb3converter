//
//  STFS.hpp
//  rb3converter
//
//  Created by tihmstar on 30.12.20.
//

#ifndef STFS_hpp
#define STFS_hpp

#include <iostream>
#include "FileLoader.hpp"
#include <stdint.h>

#define ATTRIBUTE_PACKED __attribute__ ((packed))

class STFS : public FileLoader{
    //See: https://github.com/Free60Project/wiki/blob/master/docs/STFS.md
    enum Certificate_Owner_Console_Type_t : uint8_t {
        Certificate_Owner_Console_Type_devkit = 1,
        Certificate_Owner_Console_Type_retail = 2
    };
    enum Platform_t : uint8_t{
        Platform_Xbox_360 = 2,
        Platform_PC = 4
    };
    
    enum Descriptor_type_t : int32_t{
        Descriptor_type_STFS = 0,
        Descriptor_type_SVOD = 1
    };
    
    struct ATTRIBUTE_PACKED CON_header{
        uint16_t Public_Key_Certificate_Size;
        uint8_t Certificate_Owner_Console_ID[0x5];
        char Certificate_Owner_Console_Part_Number[0x14];
        Certificate_Owner_Console_Type_t Certificate_Owner_Console_Type;
        char Certificate_Date_of_Generation[0x8];
        uint32_t Public_Exponent;
        uint8_t Public_Modulus[0x80];
        uint8_t Certificate_Signature[0x100];
        uint8_t Signature[0x80];
    };
    struct ATTRIBUTE_PACKED LIVE_PIRS_header{
        uint8_t Package_Signature[0x100];
        uint8_t Padding[0x128];
    };
    struct ATTRIBUTE_PACKED header{
        char magic[4];
        union{
            CON_header con_header;
            LIVE_PIRS_header live_pirs_header;
        };
    };
    
    struct ATTRIBUTE_PACKED license_entry{
        uint64_t License_ID;
        uint32_t License_Bits;
        uint32_t License_Flags;
    };
    
    struct ATTRIBUTE_PACKED Volume_Descriptor_STFS{
        uint8_t Volume_descriptor_size; //0x24
        uint8_t Reserved;
        uint8_t Block_Seperation;
        uint16_t File_Table_Block_Count;
        uint8_t File_Table_Block_Number[3];
        uint8_t Top_Hash_Table_Hash[0x14];
        int32_t Total_Allocated_Block_Count;
        int32_t Total_Unallocated_Block_Count;
    };
    
    struct ATTRIBUTE_PACKED Volume_Descriptor_SVOD{
        uint8_t Volume_descriptor_size; //0x24
        uint8_t Block_Cache_Element_Count;
        uint8_t Worker_Thread_Processor;
        uint8_t Worker_Thread_Priority;
        uint8_t Hash[0x14];
        uint8_t Device_features;
        uint8_t Data_block_count[3];
        uint8_t Data_block_offset[3];
        uint8_t Padding[5];
    };
    
    struct ATTRIBUTE_PACKED Volume_Descriptor{
        union{
            Volume_Descriptor_STFS STFS;
            Volume_Descriptor_SVOD SVOD;
        };
    };
    
    struct ATTRIBUTE_PACKED metadata_v1{
        uint8_t Padding[0x4C];
        uint8_t Device_ID[0x14];
        uint8_t Display_Name[0x900];
        uint8_t Display_Description[0x900];
        unsigned char Publisher_Name[0x80];
        unsigned char Title_Name[0x80];
        uint8_t Transfer_Flags;
        int32_t Thumbnail_Image_Size;
        int32_t Title_Thumbnail_Image_Size;
        uint8_t Thumbnail_Image[0x4000];
        uint8_t Title_Thumbnail_Image[0x4000];
    };

    struct ATTRIBUTE_PACKED metadata_v2{
        uint8_t Series_ID[0x10];
        uint8_t Season_ID[0x10];
        int16_t Season_Number;
        int16_t Episode_Number;
        uint8_t Padding[0x28];
        uint8_t Thumbnail_Image[0x3D00];
        uint8_t Additional_Display_Names[0x300];
        uint8_t Title_Thumbnail_Image[0x3D00];
        uint8_t Additional_Display_Descriptions[0x300];
    };

    struct ATTRIBUTE_PACKED metadata{
        license_entry Licensing_Data[0x10];
        uint8_t Header_SHA1_Hash[0x14];
        uint32_t HeaderSize;
        int32_t Content_Type;
        int32_t Metadata_Version;
        int64_t Content_Size;
        uint32_t Media_ID;
        int32_t Version;
        int32_t Base_Version;
        uint32_t Title_ID;
        Platform_t Platform;
        uint8_t Executable_Type;
        uint8_t Disc_Number;
        uint8_t Disc_In_Set;
        uint32_t Save_Game_ID;
        uint8_t Console_ID[5];
        uint8_t Profile_ID[8];
        Volume_Descriptor Volume_Descriptor;
        int32_t Data_File_Count;
        int64_t Data_File_Combined_Size;
        Descriptor_type_t Descriptor_type;
        int32_t Reserved;
        union {
            metadata_v1 v1;
            metadata_v2 v2;
        };
    };
    
private:
    const header *_head;
    const metadata *_meta;

    uint32_t _block_number;
    uint16_t _block_count;
public:
    STFS(std::string inpath);
    ~STFS();
};

#endif /* STFS_hpp */
