#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "timers.h"
#include "HdfStack.h"
#include "util.h"

int compilestack(std::string root, std::string outpath)
{
    std::string outfile = join(outpath, "stack.h5");
   
    if (fileExists(outfile))
    {
        printf("ERROR: file '%s' already exists\n", outfile.c_str());
        printf("ERROR: cannot overwrite existing file, delete it manually.\n");
        return -1;
    }

    HdfStack stack;
    {
        PBT pbt("loadTXT");
        stack.loadTXT(root, outpath);
    }
    {
        PBT pbt("write");
        stack.save(outfile.c_str(), 0);
    }

    return 0;
}

//
// compilestack
//
// Create an HDF-STACK from 3 TXT files:
//     superpixel_bounds.txt
//     superpixel_to_segment_map.txt
//     segment_to_body_map.txt
//
int main(int argc, char* argv[])
{
    assert(sizeof(uint32) == 4);

    if (argc == 2 || argc == 3)
    {
        const char* stackpath = argv[1];
        const char* outpath = argv[1];
    
        if (argc == 3)
        {
            outpath = argv[2];
        }
    
        try
        {
            return compilestack(stackpath, outpath);
        }
        catch (std::string& error)
        {
            fprintf(stderr, error.c_str());
        }
        catch (std::exception& e)
        {
            fprintf(stderr, e.what());
        }
    }
    else
    {
        printf("USAGE: %s <stack-path> [output-path]\n", argv[0]);
        exit(1);
    }
}
