//
//  ConvertPNG.hpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#ifndef ConvertPNG_hpp
#define ConvertPNG_hpp

#include "FileLoader.hpp"

class ConvertPNG : public FileLoader{
    
public:
    ConvertPNG(std::string inpath);
    ~ConvertPNG();
    
    void convertToPS3(std::string outpath);
    
};

#endif /* ConvertPNG_hpp */
