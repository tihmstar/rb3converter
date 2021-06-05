//
//  dtaParser.cpp
//  rb3converter
//
//  Created by tihmstar on 11.01.21.
//

#include "dtaParser.hpp"
#include <libgeneral/macros.h>
#include <fcntl.h>
#include <set>

#define INTENDATIONCNT 2

dtaParser::dtaParser(std::string inpath)
: _loader(nullptr)
, _mem(NULL), _memSize(NULL)
, _nextSongID(1000000141)
{
    _loader = new FileLoader(inpath);
    _mem = _loader->mem();
    _memSize = _loader->size();

    const char *buf = (const char *)_mem;
    size_t bufSize = _memSize;
    while (bufSize>0) {
        dtaObject obj = {};
        try {
            obj = parseElement(buf, bufSize);
        } catch (tihmstar::exception &e) {
#ifdef DEBUG
            e.dump();
#endif
            
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
        if (obj.type != dtaObject::type_empty) {
            _roots.push_back(obj);
        }
        assure(bufSize < _memSize); //new size can't be bigger
    }
}

dtaParser::dtaParser(const void *mem, size_t memSize)
: _loader(nullptr)
, _mem((const uint8_t*)mem), _memSize(memSize)
, _nextSongID(1000000141)
{
    const char *buf = (const char *)_mem;
    size_t bufSize = _memSize;
    while (bufSize>0) {
        dtaObject obj = {};
        try {
            obj = parseElement(buf, bufSize);
        } catch (tihmstar::exception &e) {
#ifdef DEBUG
            e.dump();
#endif
            
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
        if (obj.type != dtaObject::type_empty) {
            _roots.push_back(obj);
        }
        assure(bufSize < _memSize); //new size can't be bigger
    }
}

dtaParser::~dtaParser(){
    safeDelete(_loader);
}

dtaObject dtaParser::parseElement(const char *buf, size_t size){
    dtaObject ret = {};
    bool didOpen = false;
    size_t origSize = size;
    const char *origBuf = buf;
    bool isNeg = false;
    
    if (origSize == 509) {
        printf("");
    }
    
    while (size > 0) {
        char c = *buf++;size--;
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
    if (ret.type == dtaObject::type_undefined &&
        ret.keys.size() == 0 &&
        ret.keywords.size() == 0 &&
        ret.intValues.size() == 0 &&
        ret.strValue.size() == 0 &&
        ret.children.size() == 0 &&
        ret.comment.size() == 0) {
        ret.type = dtaObject::type_empty;
    }
        
    assure(ret.type != dtaObject::type_undefined);
    ret.usedSize = buf-origBuf;
    retassure(didOpen || ret.type == dtaObject::type_empty, "invalid object");
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


std::string dtaParser::getWriteDataIntended(const void *buf, size_t bufSize, int intendlevel, bool noending){
    std::string ret;
    if (!noending) {
        for (int i = 0; i<intendlevel*INTENDATIONCNT; i++){
            ret += ' ';
        }
    }
    ret += std::string((unsigned char *)buf,(unsigned char *)buf+bufSize);
    if (!noending) {
        ret += "\r\n";
    }
    return ret;
}

std::string dtaParser::getWriteObjData (const dtaObject &obj, int intendlevel, bool noending){
    std::string ret;
    switch (obj.type) {
        case dtaObject::type_children:
        {
            if (obj.keywords.size() == 0) {
                ret += getWriteDataIntended("(", 1, intendlevel, noending);
                if (obj.keys.size() == 1) {
                    std::string w = '\'' + obj.keys.at(0) + '\'';
                    ret += getWriteDataIntended(w.data(), w.size(), intendlevel+1, noending);
                }else if (obj.keys.size() != 0){
                    reterror("todo");
                }
                for (auto child : obj.children){
                    ret += getWriteObjData(child, intendlevel+1);
                }
                ret += getWriteDataIntended(")", 1, intendlevel, noending);
            }else if (obj.keywords.size() == 1){
                std::string w;
                if (obj.children.size() == 1) {
                    w = getWriteDataIntended("(", 1, intendlevel, true);
                    w += obj.keywords.at(0);
                    w += ' ';
                    w += getWriteObjData(obj.children.at(0), intendlevel, true);
                }else if (obj.children.size() > 1){
                    std::string w = {'('};
                    w += obj.keywords.at(0);
                    for (auto o : obj.children) {
                        if (w.size() > 1) w += ' ';
                        w += getWriteObjData(o, intendlevel);
                    }
                }
                w += ')';
                ret += getWriteDataIntended(w.data(), w.size(), intendlevel, noending);
            }else{
                reterror("todo");
            }
        }
            break;
        case dtaObject::type_str:
        {
            ret += getWriteDataIntended("(", 1, intendlevel, noending);
            if (obj.keys.size() == 1) {
                std::string w = '\'' + obj.keys.at(0) + '\'';
                ret += getWriteDataIntended(w.data(), w.size(), intendlevel+1, noending);
            }else if (obj.keywords.size() == 1) {
                std::string w = obj.keywords.at(0);
                ret += getWriteDataIntended(w.data(), w.size(), intendlevel+1, noending);
            }else{
                reterror("todo");
            }
            {
                std::string w = '\"' + obj.strValue + '\"';
                ret += getWriteDataIntended(w.data(), w.size(), intendlevel+1, noending);
            }
            ret += getWriteDataIntended(")", 1, intendlevel, noending);
        }
            break;
        case dtaObject::type_ints:
        {
            std::string w{'('};
            if (obj.keys.size() == 1) {
                w += '\'' + obj.keys.at(0) + '\'';
            }else if (obj.keys.size() != 0){
                reterror("todo");
            }else if (obj.keywords.size()){
                retassure(obj.keywords.size() == 1, "todo");
                w += obj.keywords.at(0);
            }
                            
            for (auto v : obj.intValues) {
                if (w.size() > 1) w += ' ';
                w += std::to_string(v);
            }
            w += ')';
            ret += getWriteDataIntended(w.data(), w.size(), intendlevel, noending);
        }
            break;
        case dtaObject::type_floats:
        {
            std::string w{'('};
            if (obj.keys.size() == 1) {
                w += '\'' + obj.keys.at(0) + '\'';
            }else if (obj.keys.size() != 0){
                reterror("todo");
            }
            for (auto v : obj.floatValues) {
                if (w.size() > 1) w += ' ';
                char buf[0x100];
                snprintf(buf, sizeof(buf), "%.2f",v);
                w += buf;
            }
            w += ')';
            ret += getWriteDataIntended(w.data(), w.size(), intendlevel, noending);
        }
            break;
        case dtaObject::type_keys:
        {
            std::string w{'('};
            for (auto k : obj.keys) {
                if (w.size() > 1) w += ' ';
                w += '\'' + k + '\'';
            }
            w += ')';
            ret += getWriteDataIntended(w.data(), w.size(), intendlevel, noending);
        }
            break;
        case dtaObject::type_keywords:
        {
            std::string w{'('};
            for (auto k : obj.keywords) {
                if (w.size() > 1) w += ' ';
                w += k;
            }
            w += ')';
            ret += getWriteDataIntended(w.data(), w.size(), intendlevel, noending);
        }
            break;
        case dtaObject::type_comment:
        {
            ret += getWriteDataIntended(obj.comment.data(), obj.comment.size(), 0, noending);
        }
            break;
        case dtaObject::type_empty:
        {
            ret += getWriteDataIntended("()", 2, intendlevel, noending);
        }
            break;
        default:
//#warning DEBUG
//            return ret;
            reterror("unexpected type");
    }
    
    return ret;
}

dtaParser& dtaParser::operator+=(const dtaParser &add){
    _roots.insert(_roots.end(), add._roots.begin(), add._roots.end());
    return *this;
}

void dtaParser::verifyAndFixSongIDs(){
    std::set<std::string> songIDs;
    for (dtaObject &song : _roots) {
        assure(song.type == dtaObject::type_children);
        for (auto &child : song.children) {
            if (child.keys.size()) {
                std::string &key = child.keys.at(0);
                if (key == "song_id") {
                    std::string curID;
                    if (child.intValues.size()) {
                        curID = std::to_string(child.intValues.at(0));
                    }else if (child.strValue.size()){
                        curID = child.strValue;
                    }else{
                        reterror("error reading song_id");
                    }
                    if (songIDs.find(curID) == songIDs.end()) {
                        //unknown ID, this is fine!
                        songIDs.insert(curID);
                    }else{
                        std::string newID;
                        do{
                            newID = std::to_string(_nextSongID++);
                        } while (songIDs.find(newID) != songIDs.end());
                        warning("Duplicate songid '%s' found!, replaceing with '%s'",curID.c_str(),newID.c_str());
                        child.intValues.clear(); //make sure we don't have it numeric
                        child.strValue = newID;
                        songIDs.insert(newID);
                    }
                    goto doContinue;
                }
            }
        }
    doContinue:
        continue;
    }
}

void dtaParser::writeSongToFile(std::string outfile, uint32_t songnum){
    verifyAndFixSongIDs();
    int fd = -1;
    cleanup([&]{
        if (fd > 0) {
            close(fd); fd = -1;
        }
    });
    dtaObject &song = _roots.at(songnum);
    
    retassure((fd = open(outfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0755)) > 0, "failed to create file");
    std::string w = getWriteObjData(song, 0);
    write(fd, w.data(), w.size());
}


void dtaParser::writeToFile(std::string outfile){
    verifyAndFixSongIDs();
    int fd = -1;
    cleanup([&]{
        safeClose(fd);
    });
    
    retassure((fd = open(outfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0755)) > 0, "failed to create file");
    
    for (dtaObject &song : _roots) {
        std::string w = getWriteObjData(song, 0);
        write(fd, w.data(), w.size());
    }
}