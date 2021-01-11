//
//  dtaParser.cpp
//  rb3converter
//
//  Created by tihmstar on 11.01.21.
//

#include "dtaParser.hpp"
#include <libgeneral/macros.h>

dtaParser::dtaParser(std::string inpath)
: FileLoader(inpath)
{
    const char *buf = (const char *)_mem;
    size_t bufSize = _memSize;
    while (bufSize>0) {
        dtaObject obj;
        try {
            obj = parseElement(buf, bufSize);
        } catch (...) {
            //check if only invisible characters are at the end of file. If so, ignore the error
            for (size_t i = 0; i<bufSize; i++) {
                char c = buf[i];
                if (c == '\r' ||
                    c == '\n' ||
                    c == ' ') {
                    continue;
                }
                throw;
            }
            break;
        }
        buf += obj.usedSize;
        bufSize -= obj.usedSize;
        _roots.push_back(obj);
        assure(bufSize < _memSize); //new size can't be bigger
    }
}

dtaParser::~dtaParser(){
    //
}

dtaObject dtaParser::parseElement(const char *buf, size_t size){
    dtaObject ret;
    bool didOpen = false;
    size_t origSize = size;
    bool isNeg = false;
    
    while (size-- > 0) {
        char c = *buf++;
        if (c == ' ' ||
            c == '\n' ||
            c == '\r' ) {
            continue;
        }
        
        if (!didOpen) {
            assure(c = '(');
            didOpen = true;
            continue;
        }else if (c == '(') {
            ret.type = dtaObject::type_children;

            dtaObject child = parseElement(buf-1, size+1);
            buf+=child.usedSize-1;
            size-=child.usedSize-1;
            ret.children.push_back(child);
            continue;
        }
        
        if (c == '\'') {
            if (ret.keys.size()){
                assure(ret.type == dtaObject::type_undefined || ret.type == dtaObject::type_keys);
                ret.type = dtaObject::type_keys;
            }
            std::string key;
            while (true) {
                assure(size-- > 0);
                char k = *buf++;
                assure(k != '\0');
                if (k == '\''){
                    break;
                }
                key += k;
            }
            ret.keys.push_back(key);
            continue;
        }else if (c == '\"'){
            assure(ret.strValue.size() == 0);
            ret.type = dtaObject::type_str;
            while (true) {
                assure(size-- > 0);
                char k = *buf++;
                assure(k != '\0');
                if (k == '\"'){
                    break;
                }
                ret.strValue += k;
            }
            continue;
        }else if (c == ')'){
            goto end;
        }else if (c == '-'){
            isNeg = true;
            assure(size--);c = *buf++;
            assure(isdigit(c)); //next char has to be a digit!
        }
        
        if (isdigit(c)) {
            assure(ret.type == dtaObject::type_undefined);
            int intNum = 0;
            float floatNum = 0;
            int decimalPoints = 0;
            while (true) {
                if (isdigit(c)){
                    if (ret.type == dtaObject::type_floats) {
                        if (decimalPoints == 0) {
                            floatNum *= 10;
                            floatNum += c -'0';
                        }else{
                            float digit = c - '0';
                            digit /= (10*decimalPoints);
                            floatNum += digit;
                        }
                    }else{
                        //first number gets handled here
                        intNum *=10;
                        intNum += c-'0';
                    }
                }else if (c == '.'){
                    //this is a float
                    assure(ret.type == dtaObject::type_floats || ret.type == dtaObject::type_undefined);
                    ret.type = dtaObject::type_floats;
                    floatNum = intNum;
                    decimalPoints = 1;
                }else if (c == ' ' || c == ')'){
                    if (ret.type == dtaObject::type_undefined) {
                        ret.type = dtaObject::type_ints;
                    }
                    if (ret.type == dtaObject::type_floats) {
                        if (isNeg) floatNum *= -1;
                        ret.floatValues.push_back(floatNum);
                        floatNum = 0;
                        decimalPoints = 0;
                    }else if (ret.type == dtaObject::type_ints){
                        if (isNeg) intNum *= -1;
                        ret.intValues.push_back(intNum);
                        intNum = 0;
                    }
                    isNeg = false;
                }else if (c == '-'){
                    isNeg = true;
                }else{
                    reterror("parser error");
                }
                
                if (c == ')') goto end;
                
                assure(size-- > 0);
                c = *buf++;
                assure(c != '\0');
            }
        }else if (isalpha(c)){
            std::string keyword;
            while (c != ' ' && c != ')'){
                keyword += c;
                assure(size-- > 0);
                c = *buf++;
                assure(c != '\0');
            }
            ret.keywords.push_back(keyword);
            if (c == ')') goto end;
            continue;
        }else if (c == ';'){
            //comment
            dtaObject comment;
            comment.type = dtaObject::type_comment;
            comment.usedSize = 0;
            while (c != '\r' && c != '\n'){
                comment.comment += c;
                assure(size-- > 0);
                c = *buf++;
                assure(c != '\0');
            }
            ret.children.push_back(comment);
            continue;
        }
    }
    
end:
    if (ret.type == dtaObject::type_undefined && ret.keywords.size()) {
        ret.type = dtaObject::type_keywords;
    }
    ret.usedSize = origSize-size;
    retassure(didOpen, "invalid object");
    return ret;
}


size_t dtaParser::getSongsCnt(){
    return _roots.size();
}

std::string dtaParser::getSongIDForSong(uint32_t songnum){
    dtaObject &song = _roots.at(songnum);
    assure(song.type == dtaObject::type_children);

    for (auto &child : song.children) {
        if (child.keys.size()) {
            std::string &key = child.keys.at(0);
            if (key == "song_id") {
                if (child.intValues.size()) {
                    return std::to_string(child.intValues.at(0));
                }else if (child.strValue.size()){
                    return child.strValue;
                }else{
                    reterror("error reading song_id");
                }
            }
        }
    }
    
    reterror("failed to get song_id");
}
