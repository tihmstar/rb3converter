//
//  songsManager.hpp
//  rb3converter
//
//  Created by tihmstar on 11.01.21.
//

#ifndef songsManager_hpp
#define songsManager_hpp

#include "FileLoader.hpp"
#include <iostream>

class songsManager : public FileLoader  {

public:
    songsManager(std::string inpath);
    ~songsManager();
    
};

#endif /* songsManager_hpp */
