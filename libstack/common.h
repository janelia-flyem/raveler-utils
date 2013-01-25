#pragma once

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <string>

#include <list>
#include <set>
#include <vector>
#include <ext/hash_map>
#include <ext/hash_set>

namespace ext = __gnu_cxx;

class HdfFile;
class Table;

typedef unsigned int uint32;

typedef long long int64;
typedef unsigned long long uint64;

typedef std::list<std::string> StringList;

typedef std::vector<uint32> IntVec;
typedef IntVec::iterator IntVecIt;

typedef ext::hash_map<uint32, uint32> IntMap;
typedef ext::hash_map<uint32, Table*> TableMap;
typedef ext::hash_set<uint32> IntSet;

typedef std::vector<uint64> Int64Vec;
typedef ext::hash_map<uint32, uint64> Int64Map;

// EMPTY_VALUE means data is completely not there, equivalent
// to an entry not existing in a dictionary.
#define EMPTY_VALUE 0xFFFFFFFF

// END_OF_LIST is used to terminate lists in our packed arrays
// of lists.  Same value as EMPTY_VALUE
#define END_OF_LIST 0xFFFFFFFF 

// Number of reserved ids like END_OF_LIST, we have only 1 right
// now but leave room for more in case we need them
#define RESERVED_IDS 256

// Because tables contain only uint32 values, we can't have tables
// bigger than this because otherwise no index could refer to that 
// row.
#define MAX_TABLE_ROWS (0xFFFFFFFF - RESERVED_IDS)

// A single 2D bounding box
struct Bounds
{
    uint32 x;
    uint32 y;
    uint32 width;
    uint32 height;
    
    bool isEmpty() const { return width == 0 && height == 0; }
    
    float x0() { return x; }
    float x1() { return float(x) + width - 1; }
    float y0() { return y; }
    float y1() { return float(y) + height - 1; }
};

typedef std::vector<Bounds> BoundsVec;

struct float3    
{
    float3(float x_, float y_, float z_) :
        x(x_), 
        y(y_),
        z(z_)
    {}
    float x, y, z;
};

typedef std::vector<float3> Float3Vec;

