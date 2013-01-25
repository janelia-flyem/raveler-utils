#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "timers.h"
#include "HdfStack.h"

//
// verifystack
//
// Verify an HDF-STACK is internally consistent.
// Prints out any discrepencies.
//
int main(int argc, char* argv[])
{
    assert(sizeof(uint32) == 4);

    if (argc == 2 || argc == 3)
    {
        const char* path = argv[1];
        
        bool repair = false;
        
        if (argv[2])
        {
            repair = strcmp(argv[2], "-repair") == 0;
        }
        
        printf("Repair Mode: %s\n", repair ? "ON" : "OFF");

        HdfStack stack;
        
        try
        {
            {
                PBT pbt("load HDF-STACK");
                stack.load(path);
            }
            {
                PBT pbt("verify HDF-STACK");
                stack.verify(repair);
            }
            
            if (repair)
            {
                printf("Saving %s\n", path);
                PBT pbt("save HDF-STACK");
                stack.save(path, 0);
                
            }
        }
        catch (std::string& error)
        {
            printf("CAUGHT ERROR: %s\n", error.c_str());
        }
     
    }
    else
    {
        printf("USAGE: %s <stack.h5> [-repair]\n", argv[0]);
        exit(1);
    }

    printf("done\n");
    return 0;
}