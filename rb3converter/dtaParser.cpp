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
#include <algorithm>

#define INTENDATIONCNT 3

dtaParser::dtaParser(std::string inpath)
: _nextSongID(1000000141)
{
    FileLoader *loader = nullptr;
    cleanup([&]{
        safeDelete(loader);
    });
    loader = new FileLoader(inpath);
    const uint8_t *_mem = loader->mem();
    size_t _memSize = loader->size();

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
            if (obj.keys.size()) {
                std::string title = obj.keys.at(0);
                std::transform(title.begin(), title.end(), title.begin(), tolower);
                obj.keys[0] = title;
            }
            if (obj.keys.size() == 0 && obj.children.size() == 1 && obj.type == dtaObject::type_children
                && obj.numValues.size() == 0 && obj.strValue.size() == 0 && obj.keywords.size() == 0
                ) {
                //WTF??
                debug("Unpacking only child!");
                _roots.push_back(obj.children.at(0));
            }else{
                _roots.push_back(obj);
            }
        }
        assure(bufSize < _memSize); //new size can't be bigger
    }
}

//#define DEBUG_PARSER

dtaParser::dtaParser(const void *mem, size_t memSize)
: _nextSongID(1000000141)
{
    const char *buf = (const char *)mem;
    size_t bufSize = memSize;
    while (bufSize>0) {
        dtaObject obj = {};
#ifndef DEBUG_PARSER
        try {
#endif
            obj = parseElement(buf, bufSize);
#ifndef DEBUG_PARSER
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
#endif
        
        buf += obj.usedSize;
        bufSize -= obj.usedSize;
        if (obj.type != dtaObject::type_empty) {
            if (obj.keys.size()) {
                std::string title = obj.keys.at(0);
                std::transform(title.begin(), title.end(), title.begin(), tolower);
                obj.keys[0] = title;
            }
            if (obj.keys.size() == 0 && obj.children.size() == 1 && obj.type == dtaObject::type_children
                && obj.numValues.size() == 0 && obj.strValue.size() == 0 && obj.keywords.size() == 0
                ) {
                //WTF??
                debug("Unpacking only child!");
                _roots.push_back(obj.children.at(0));
            }else{
                _roots.push_back(obj);
            }
        }
        assure(bufSize < memSize); //new size can't be bigger
    }
}

dtaParser::~dtaParser(){
    //
}

dtaObject dtaParser::parseElement(const char *buf, size_t size){
    dtaObject ret = {};
    bool didOpen = false;
    size_t origSize = size;
    const char *origBuf = buf;
    bool isNeg = false;
        
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
            bool isInteger = true;
            while (true) {
                if (isdigit(c)){
                    if (!isInteger) {
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
                    assure(ret.type == dtaObject::type_nums || ret.type == dtaObject::type_undefined);
                    isInteger = false;
                    floatNum = intNum;
                    decimalPoints = 1;
                }else if (c == ' ' || c == ')'){
                    if (ret.type == dtaObject::type_undefined) {
                        ret.type = dtaObject::type_nums;
                    }
                    if (!isInteger) {
                        if (isNeg) floatNum *= -1;
                        ret.numValues.push_back({
                            .isInteger = false,
                            .floatVal = floatNum
                        });
                    } else {
                        if (isNeg) intNum *= -1;
                        ret.numValues.push_back({
                            .isInteger = true,
                            .intVal = intNum
                        });
                    }
                    floatNum = 0;
                    decimalPoints = 0;
                    intNum = 0;
                    isInteger = true;
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
            while (c != ' ' && c != ')' && c != '\r' && c != '\n'){
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
        ret.numValues.size() == 0 &&
        ret.strValue.size() == 0 &&
        ret.children.size() == 0 &&
        ret.comment.size() == 0) {
        ret.type = dtaObject::type_empty;
    }
    
    if ((ret.keys.size() && ret.keys.at(0) == "song_id")
        || (ret.keywords.size() && ret.keywords.at(0) == "song_id")) {

        ret.keys.clear();
        ret.keywords.clear();

        ret.keys.push_back("song_id");
        ret.type = dtaObject::type_nums;
        if (ret.numValues.size() == 0 && ret.strValue.size() == 0) {
            ret.strValue = "id_fixme";
        }
    }
    
    if (ret.type == dtaObject::type_undefined) {
        reterror("parser error");
    }
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
        std::string key;
        if (child.keys.size()) {
            key = child.keys.at(0);
        }else if (child.keywords.size()){
            key = child.keywords.at(0);
        }
        
        if (key == "song_id") {
            if (child.numValues.size()) {
                auto &val = child.numValues.at(0);
                if (val.isInteger) {
                    return std::to_string(val.intVal);
                }else{
                    reterror("song_id shouldn't be a float!");
                }
            }else if (child.strValue.size()){
                return child.strValue;
            }else{
                reterror("error reading song_id");
            }
        }
    }
    
    reterror("failed to get song_id");
}


std::string dtaParser::getWriteDataIntended(const void *buf, size_t bufSize, int intendlevel, bool noEnding, bool doIntend){
    std::string ret;
    if (doIntend){
        for (int i = 0; i<intendlevel*INTENDATIONCNT; i++){
            ret += ' ';
        }
    }
    ret += std::string((unsigned char *)buf,(unsigned char *)buf+bufSize);
    if (!noEnding) {
        ret += "\r\n";
    }
    return ret;
}

std::string dtaParser::getWriteObjData (const dtaObject &obj, int intendlevel, bool noending, bool doIntend){
    std::string ret;
    switch (obj.type) {
        case dtaObject::type_children:
        {
            std::vector<std::string> keys_keywords;
            if (obj.keywords.size() == 1) {
                retassure(obj.keys.size() == 0, "can't have keys and keywords??");
                keys_keywords = obj.keywords;
            }else if (obj.keys.size() == 1){
                retassure(obj.keywords.size() ==  0, "can't have keys and keywords??");
                keys_keywords = obj.keys;
            }
            retassure(obj.children.size(), "needs children");

            {
                std::string kw{'('};
                bool noChildrenEnding = obj.children.size() == 1 && obj.children.at(0).type != dtaObject::type_children;
                if (obj.keys.size() == 1) {
                    kw += '\'' + obj.keys.at(0) + '\'';
                    if (noChildrenEnding) kw += ' ';
                }else if (obj.keywords.size() == 1){
                    kw += obj.keywords.at(0);
                    if (noChildrenEnding) kw += ' ';
                }else if (obj.keys.size() != 0){
                    reterror("todo");
                }

                ret += getWriteDataIntended(kw.data(), kw.size(), intendlevel, noChildrenEnding, doIntend);
                for (auto child : obj.children){
                    ret += getWriteObjData(child, intendlevel+1, noChildrenEnding, !noChildrenEnding);
                }
                ret += getWriteDataIntended(")", 1, intendlevel, noending, !noChildrenEnding);
            }
        }
            break;
        case dtaObject::type_str:
        {
            std::string ws{'('};
            if (obj.keys.size() == 1) {
                ws += '\'' + obj.keys.at(0) + '\'';
            }else if (obj.keywords.size() == 1) {
                ws += obj.keywords.at(0);
            }else{
                reterror("todo");
            }
            ws += " \"" + obj.strValue + "\")";
            ret += getWriteDataIntended(ws.c_str(), ws.size(), intendlevel, noending, doIntend);
        }
            break;

        case dtaObject::type_nums:
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
            
            for (auto v : obj.numValues) {
                if (w.size() > 1) w += ' ';
                if (v.isInteger) {
                    w += std::to_string(v.intVal);
                }else{
                    char buf[0x100];
                    snprintf(buf, sizeof(buf), "%.2f",v.floatVal);
                    w += buf;
                }
            }
            w += ')';
            ret += getWriteDataIntended(w.data(), w.size(), intendlevel, noending, doIntend);
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
            ret += getWriteDataIntended(w.data(), w.size(), intendlevel, noending, doIntend);
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
            ret += getWriteDataIntended(w.data(), w.size(), intendlevel, noending, doIntend);
        }
            break;
        case dtaObject::type_comment:
        {
            ret += getWriteDataIntended(obj.comment.data(), obj.comment.size(), 0, false);
        }
            break;
        case dtaObject::type_empty:
        {
            ret += getWriteDataIntended("()", 2, intendlevel, noending, false);
        }
            break;
        default:
//#warning DEBUG
//            return ret;
            reterror("unexpected type");
    }

    
    {
        auto isValidChar = [](char c)->bool{
            if (isalnum(c)) return true;
            switch (c) {
                case ' ':
                case '(':
                case ')':
                case '/':
                case '_':
                case '-':
                case '.':
                case ',':
                case '"':
                case ';':
                case ':':
                case '&':
                case '\r':
                case '\n':
                case '\'':
                    return true;
                    
                default:
                    return false;
            }
        };
        
        size_t b1 = 0;
        size_t b2 = 0;
        for (int i=0; i < ret.size(); i++){
            char *buf = (char*)ret.c_str();
            char c = buf[i];
            if (c == '(') b1++;
            else if (c == ')') b2++;
            if (!isValidChar(c)) {
                if ((uint8_t)buf[i] == 0xc3 && (uint8_t)buf[i+1] == 0x9f){
                    /*
                        Replace german 'sz' -> ss
                     */
                    c = buf[i] = 's';
                    buf[i+1] = 's';
                    continue;
                }
                
                if ((uint8_t)buf[i] > 0x7f || buf[i] == '!' || buf[i] == '#'){
                    buf[i] = 'T';
                    continue;
                }
                
                for (int j=i; j>=0; j--) {
                    if (buf[j] == '\n') break;
                    if (buf[j] == ';') goto ignore;
                }
                reterror("Invalid char '%c' found in buf='%s'",c,ret.c_str());
            ignore:
                debug("ignoring invalid char '%c' in buf='%s'",c,ret.c_str());
                continue;
            }
        }
        retassure(b2 <= b1, "dta sanity check failed");
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
            std::string key;
            if (child.keys.size()) {
                key = child.keys.at(0);
            }else if (child.keywords.size()){
                key = child.keywords.at(0);
            }
            if (key == "rating"){
                if (child.numValues.size() == 1) {
                    auto rating = child.numValues.at(0);
                    retassure(rating.isInteger, "rating is float");
                    if (rating.intVal > 2) {
                        child.numValues.clear();
                        child.numValues.push_back({
                            .isInteger = true,
                            .intVal = 2
                        });
                        debug("Setting rating to '2'");
                    }
                }
            }
            
            if (key == "song_id") {
                std::string curID;
                if (child.numValues.size()) {
                    auto &v = child.numValues.at(0);
                    retassure(v.isInteger, "song_id is a float");
                    curID = std::to_string(v.intVal);
                }else if (child.strValue.size()){
                    curID = child.strValue;
                }else{
                    debug("error reading song_id!");
                    goto make_new_id;
                }
                if (curID.size() == 0) {
                    debug("fixing empty song id!");
                    goto make_new_id;
                }
                                    
                if (std::to_string(atoll(curID.c_str())) != curID) {
                    debug("fixing non-numeric song id!");
                    goto make_new_id;
                }
                
                if (songIDs.find(curID) == songIDs.end()) {
                    //unknown ID, this is fine!
                    assert(curID.size());
                    songIDs.insert(curID);
                    continue;
                }
                {
            make_new_id:
                    uint32_t newID;
                    do{
                        newID = _nextSongID++;
                    } while (songIDs.find(std::to_string(newID)) != songIDs.end());
                    warning("Replacing '%s' with '%u'",curID.c_str(),newID);
                    child.strValue.clear(); //make sure we don't have it as string
                    child.numValues.clear(); //make sure we don't have other numbers
                    child.numValues.push_back({
                        .isInteger = true,
                        .intVal = (int)newID
                    });
                    songIDs.insert(std::to_string(newID));
                }
                goto doContinue;
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
    
    retassure((fd = open(outfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644)) > 0, "failed to create file '%s'",outfile.c_str());
    std::string w = getWriteObjData(song, 0);
    write(fd, w.data(), w.size());
}


void dtaParser::writeToFile(std::string outfile){
    verifyAndFixSongIDs();
    int fd = -1;
    cleanup([&]{
        safeClose(fd);
    });
    
    retassure((fd = open(outfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644)) > 0, "failed to create file '%s'",outfile.c_str());
    
    for (dtaObject &song : _roots) {
        std::string w = getWriteObjData(song, 0);
        write(fd, w.data(), w.size());
    }
}
