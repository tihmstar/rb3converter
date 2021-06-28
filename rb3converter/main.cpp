//
//  main.cpp
//  rb3converter
//
//  Created by tihmstar on 14.12.20.
//

#include <iostream>
#include "ConvertMogg.hpp"
#include "ConvertPNG.hpp"
#include "ConvertMid.hpp"
#include "STFS.hpp"
#include "dtaParser.hpp"
#include <libgeneral/macros.h>
#include <getopt.h>
#include "songsManager.hpp"

static struct option longopts[] = {
    { "help",           no_argument,        NULL, 'h' },
    { "klic",           required_argument,  NULL, 'k' },
    { "rap",            required_argument,  NULL, 'p' },
    { "region",         required_argument,  NULL, 'r' },
    { "threads",        required_argument,  NULL, 'j' },
    { NULL, 0, NULL, 0 }
};


void cmd_help(){
    printf("Usage: rb3converter <CON dir> <PS3 dir>\n");
    printf("Converts RB3 XBOX songs to PS3 songs\n\n");
    printf("  -h, --help\t\t\tprints usage information\n");
    printf("  -k, --klic <path>\tPath to klic.txt\n");
    printf("  -p, --rap <path>\tPath to .rap\n");
    printf("  -r, --region <reg>\tSet region to 'pal' or 'ntsc'\n");
    printf("  -j, --threads <num>\tSet the number of threads to use\n");
}

int main_r(int argc, const char * argv[]) {
    info("%s", VERSION_STRING);

    int optindex = 0;
    int opt = 0;
    const char *condir = NULL;
    const char *ps3dir = NULL;

    const char *klicPath = NULL;
    const char *rapPath = NULL;
    
    int threads = 1;
        
    ConvertMid::Region region = ConvertMid::Region_undefined;
    
    while ((opt = getopt_long(argc, (char* const *)argv, "hk:p:r:j:", longopts, &optindex)) > 0) {
        switch (opt) {
            case 'h':
                cmd_help();
                return 0;
            case 'k': //long-opt klic
                klicPath = optarg;
                break;
            case 'p': //long-opt rap
                rapPath = optarg;
                break;
            case 'j': //long-opt threads
                threads = atoi(optarg);
                break;
            case 'r': //long-opt region
                if (strcasecmp(optarg, "pal") == 0) {
                    region = ConvertMid::Region_PAL;
                } else if (strcasecmp(optarg, "ntsc") == 0) {
                    region = ConvertMid::Region_NTSC;
                } else {
                    reterror("unknown region '%s'",optarg);
                }

                break;

            default:
                cmd_help();
                return -1;
        }
    }
    
    retassure(region != ConvertMid::Region_undefined, "Error: region not set");

    if (argc-optind == 2) {
        argc -= optind;
        argv += optind;
        condir = argv[0];
        ps3dir = argv[1];
    }else{
        cmd_help();
        return -2;
    }

    songsManager mgr(condir,ps3dir);
    
    if (threads > 1) {
        info("Setting threads to %d",threads);
        mgr.setThreads(threads);
    }
    
    mgr.convertCONtoPS3(klicPath, rapPath, region);
    return 0;
}

int main(int argc, const char * argv[]) {
#ifdef DEBUG
    return main_r(argc, argv);
#else
    try {
        return main_r(argc, argv);
    } catch (tihmstar::exception &e) {
        printf("%s: failed with exception:\n",PACKAGE_NAME);
        e.dump();
        return e.code();
    }
#endif
}
