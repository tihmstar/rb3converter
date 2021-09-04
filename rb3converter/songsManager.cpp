//
//  songsManager.cpp
//  rb3converter
//
//  Created by tihmstar on 11.01.21.
//

#include "songsManager.hpp"
#include <libgeneral/macros.h>
#include <fcntl.h>
#include <dirent.h>
#include "STFS.hpp"
#include "ConvertPNG.hpp"
#include "ConvertMogg.hpp"
#include <queue>
#include <thread>
#include <mutex>
#include <string.h>

inline bool ends_with(std::string const & value, std::string const & ending){
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}


songsManager::songsManager(std::string conPath, std::string ps3Path)
: _conPath(conPath), _ps3Path(ps3Path), _threads(1)
,_dta(NULL)
{
    if (_conPath.back() != '/') _conPath += '/';
    if (_ps3Path.back() != '/') _ps3Path += '/';

    std::string ps3DtaPath = _ps3Path;
    ps3DtaPath += "songs.dta";
    try {
        _dta = new dtaParser(ps3DtaPath);
    } catch (...) {
        warning("'%s' does not exist",ps3DtaPath.c_str());
        _dta = new dtaParser("\x00",1);
    }
    
}

songsManager::~songsManager(){
    safeDelete(_dta);
}

void songsManager::setThreads(int threads){
    if (threads < 1){
        _threads = 1;
    }else{
        _threads = threads;
    }
}


void songsManager::convertCONtoPS3(std::string klicpath, std::string rappath, ConvertMid::Region region){
    DIR *dir = NULL;
    cleanup([&]{
        safeFreeCustom(dir, closedir);
    });
    struct dirent *ent = NULL;
    
    assure(dir = opendir(_conPath.c_str()));
    
    std::mutex songsLck;
    
    std::queue<std::string> conPaths;
    std::mutex conPathsLck;
        
    while ((ent = readdir (dir)) != NULL) {
        //skip hidden files
        if (ent->d_type != DT_REG || strncmp(".", ent->d_name, 1) == 0) continue;
        std::string path = _conPath + ent->d_name;
        
        if (path.substr(path.size()-4) != ".con"){
            debug("skipping file '%s'",path.c_str());
            continue;
        }
        info("Processing song='%s'",ent->d_name);
        
        conPaths.push(path);
    }
    
    size_t conPathsSongNums = conPaths.size();
    
    auto processCon = [&](std::string path){
        STFS con(path);
        auto files = con.listFiles();

        for (auto &file : files) {
            if (ends_with(file, ".milo_xbox")) {
                std::string newfilename = file.substr(0,file.size()-(sizeof(".milo_xbox")-1))+".milo_ps3";
                debug("renaming '%s' -> '%s'",file.c_str(),newfilename.c_str());
                con.extract_file(file, _ps3Path, newfilename);
                
            } else if (ends_with(file, ".png_xbox")) {
                std::string newfilename = file.substr(0,file.size()-(sizeof(".png_xbox")-1))+".png_ps3";
                debug("converting '%s' -> '%s'",file.c_str(),newfilename.c_str());
                auto data = con.extract_file_to_buffer(file);
                ConvertPNG convPng(data.data(), data.size());
                convPng.convertToPS3(_ps3Path + newfilename);
                
            } else if (ends_with(file, ".mogg")) {
                debug("converting '%s'",file.c_str());
                auto data = con.extract_file_to_buffer(file);
                ConvertMogg convMogg(data.data(),data.size());
                convMogg.convertToPS3(_ps3Path + file);
            } else if (ends_with(file, ".mid")) {
                std::string newfilename = file + ".edat";
                debug("converting '%s' -> '%s'",file.c_str(),newfilename.c_str());
                auto data = con.extract_file_to_buffer(file);
                ConvertMid convMid(data.data(), data.size(), klicpath, rappath, region);
                convMid.convertToPS3(_ps3Path + newfilename);
            } else if (ends_with(file, "songs.dta")) {
                debug("parsing '%s'",file.c_str());
                auto data = con.extract_file_to_buffer(file);
                dtaParser curSongs(data.data(),data.size());
                {
                    std::unique_lock<std::mutex> ul(songsLck);
                    *_dta += curSongs;
                }
            } else {
                debug("extracting=%s",file.c_str());
                con.extract_file(file, _ps3Path);
            }
        }
    };
    
    std::vector<std::thread *> workers;
    for (int i=0; i<_threads; i++) {
        workers.push_back(new std::thread([&](int tid){
            while (true) {
                std::string path;
                {
                    std::unique_lock<std::mutex> ul(conPathsLck);
                    if (conPaths.size() == 0) {
                        info("[%d] thread retirning",tid);
                        return;
                    }
                    path = conPaths.front(); conPaths.pop();
                }
                processCon(path);
            }
        },i));
    }
    
    info("main thread waiting for workes to finish");
    for (std::thread *t : workers) {
        t->join();
        delete t;
    }
    workers.clear();
    
    
    info("fixing song ids");
    _dta->verifyAndFixSongIDs();
    
    _dta->writeToFile(_ps3Path + "songs/songs.dta");
    
    info("Done processing %llu songs!",conPathsSongNums);
}
