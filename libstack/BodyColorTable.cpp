#include "BodyColorTable.h"
#include "HdfStack.h"
#include "util.h"

// Random seed to assign colors predictably, so a dataset looks
// the same each time it is loaded.
static const int COLOR_SEED = 4656;


BodyColorTable::BodyColorTable(HdfStack* stack) :
    m_stack(stack),
    m_table(NULL)
{
    srand(COLOR_SEED);

    IntVec bodies;
    m_stack->getallbodies(bodies);
    
    uint32 maxbody = *std::max_element(bodies.begin(), bodies.end());
    
    // Create new table: this includes padding so if we append
    // a body only once in a while will it result in a big copy
    m_table = new Table(maxbody + 1, 1);
    
    // Assign a random color to every body which exists
    for (uint32 i = 0; i < bodies.size(); ++i)                        
    {
        uint32 bodyid = bodies[i];
        
        if (bodyid == EMPTY_VALUE)
        {
            // Use EMPTY_VALUE for non-existant values.  Our random
            // colors don't generate white so this is okay
            m_table->setValue(bodyid, 0, EMPTY_VALUE);
        }
        else
        {
            setrandom(bodyid);
        }
    }
    
    // Force body 0 to always be black
    m_table->setValue(0, 0, pack_rgba(0, 0, 0, 1));        
}    


void BodyColorTable::getplanecolormap(uint32 plane, IntVec& result)
{
    IntVec spids;
    m_stack->getsuperpixelsinplane(plane, spids);
    
    uint32 maxid = 0;
    
    if (!spids.empty())
    {
        maxid = *std::max_element(spids.begin(), spids.end());
    }
    
    result.resize(maxid + 1);
    
    // Find body id and then color for each spid
    for (IntVecIt it = spids.begin(); it != spids.end(); ++it)
    {
        uint32 segid = m_stack->getsegmentid(plane, *it);
        uint32 bodyid = m_stack->getsegmentbodyid(segid);
        uint32 color = m_table->getValue(bodyid, 0);
        result[*it] = color;
    }

    // keep the borders black
    result[0] = pack_rgba(0, 0, 0, 1);
}

void BodyColorTable::addbodycolor(uint32 bodyid)
{
    // Expand table as needed
    if (bodyid >= m_table->getRows())
    {
        uint32 oldrows = m_table->getRows();
        uint32 newrows = bodyid - oldrows + 1;
        m_table->addRows(newrows);
        
        // Initialize to all empty
        for (uint32 i = oldrows; i < newrows; ++i)
        {
            m_table->setValue(i, 0, EMPTY_VALUE);
        }
    }
        

    // Only create a new color if it doesn't already exist
    if (m_table->getValue(bodyid, 0) == EMPTY_VALUE)
    {
        setrandom(bodyid);
    }
}

uint32 BodyColorTable::getbodycolor(uint32 bodyid)
{
    return m_table->getValue(bodyid, 0);
}

void BodyColorTable::setrandom(uint32 bodyid)
{
    double r = 0;
    double g = 0;
    double b = 0;
    createRandomBodyColor(r, g, b);
    
    m_table->setValue(bodyid, 0, pack_rgba(r, g, b, 1));
}
