//
//  dtaParser.hpp
//  rb3converter
//
//  Created by tihmstar on 11.01.21.
//

#ifndef dtaParser_hpp
#define dtaParser_hpp

#include "FileLoader.hpp"
#include <vector>

class dtaObject{
public:
    std::vector<std::string> keys; //might be non-existant
    std::vector<std::string> keywords;
    enum{
        type_undefined,
        type_ints,
        type_floats,
        type_str,
        type_keys,
        type_keywords,
        type_children,
        type_empty,
        type_comment
    } type = type_undefined;
    std::vector<int> intValues;
    std::vector<float> floatValues;
    std::string strValue;
    std::vector<dtaObject> children;
    std::string comment;
    size_t usedSize = 0;
};

class dtaParser{
    std::vector<dtaObject> _roots;
    uint32_t _nextSongID;
    
    dtaObject parseElement(const char *buf, size_t size);
    
    std::string getWriteDataIntended(const void *buf, size_t bufSize, int intendlevel, bool noending);
    std::string getWriteObjData(const dtaObject &obj, int intendlevel, bool noending = false);
        
public:
    dtaParser(std::string inpath);
    dtaParser(const void *mem, size_t memSize);

    ~dtaParser();
    
    size_t getSongsCnt();
    
    std::string getSongIDForSong(uint32_t songnum);
    
    dtaParser& operator+=(const dtaParser &add);
    
    void verifyAndFixSongIDs();
    
    void writeSongToFile(std::string outfile, uint32_t songnum);
    
    void writeToFile(std::string outfile);
};

#endif /* dtaParser_hpp */
