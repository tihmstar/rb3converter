//
//  songsManager.hpp
//  rb3converter
//
//  Created by tihmstar on 11.01.21.
//

#ifndef songsManager_hpp
#define songsManager_hpp

#include "dtaParser.hpp"
#include <iostream>
#include "ConvertMid.hpp"

class songsManager {
    std::string _conPath;
    std::string _ps3Path;
    int _threads;

    dtaParser *_dta;
public:
    songsManager(std::string conPath, std::string ps3Path);
    ~songsManager();
    
    void setThreads(int threads);
    
    void convertCONtoPS3(std::string klicpath, std::string rappath, ConvertMid::Region region);
};

#endif /* songsManager_hpp */
