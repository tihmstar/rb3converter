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
        type_comment
    } type = type_undefined;
    std::vector<int> intValues;
    std::vector<float> floatValues;
    std::string strValue;
    std::vector<dtaObject> children;
    std::string comment;
    size_t usedSize = 0;
};

class dtaParser : FileLoader{
    std::vector<dtaObject> _roots;
    
    dtaObject parseElement(const char *buf, size_t size);
    
public:
    dtaParser(std::string inpath);
    ~dtaParser();
    
    size_t getSongsCnt();
    
    std::string getSongIDForSong(uint32_t songnum);
};

#endif /* dtaParser_hpp */
