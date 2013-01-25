#pragma once

#include "common.h"

class HdfStack;

//
// Table which stores a random but consistent per-body color
//
class BodyColorTable
{    
public:
    BodyColorTable(HdfStack* stack);
    
    // Get a color for every spid, that is result[X] is the packed
    // RGBA color for spid X.    
    void getplanecolormap(uint32 plane, IntVec& result);
    
    // Assign the given body a new random color if it doesn't
    // have one already.  If it has one already, no change.
    void addbodycolor(uint32 bodyid);
    
    // Get packed RGBA color for the given body
    uint32 getbodycolor(uint32 bodyid);    

private:
    // Assign the given body a new randomized color
    void setrandom(uint32 bodyid);

    // Our stack with superpixel to segment to body mappings
    HdfStack* m_stack;
    
    // Table with 1 column
    // row N is the 32-bit packed RGBA color for body N.
    Table* m_table;  
};
