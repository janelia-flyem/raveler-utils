//
// HdfStack.cpp
//
#include "HdfStack.h"
#include "HdfFile.h"
#include "timers.h"
#include "util.h"
#include "LogFile.h"
#include "STLExport.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <fstream>

std::string formatIntVec(const IntVec& vec)
{
    std::string result = "[";
    for (uint32 i = 0; i < vec.size(); ++i)
    {
        result += FormatString("%u", vec[i]);
        
        if (i < vec.size() - 1)
        {
            result += " ";
        }        
    }
    
    result += "]";
    
    return result;
}

char BOUNDS_FILE[] = "superpixel_bounds.txt";
char SEGMENT_FILE[] = "superpixel_to_segment_map.txt";
char BODY_FILE[] = "segment_to_body_map.txt";

HdfStack::HdfStack() :
    m_zmin(0),
    m_zmax(0),
    m_segment(NULL),
    m_segment_sp(NULL),
    m_body_index(NULL),
    m_body_seg(NULL),
    m_log(NULL)
{
    // TODO: make logging optional via ctypes?
    // m_log = new LogFile("hdfstack.log");
}

template <typename T>
void delete_ptr(T& ptr)
{
    delete ptr;
    ptr = NULL;
}

HdfStack::~HdfStack()
{
    for (uint32 z = m_zmin; z <= m_zmax; ++z)
    {
        delete_ptr(m_superpixel[z]);
    }
    m_superpixel.clear();
    
    delete_ptr(m_segment);
    delete_ptr(m_segment_sp);
    delete_ptr(m_body_index);
    delete_ptr(m_body_seg);
    delete_ptr(m_log);
}

void HdfStack::error(const std::string& format, ...)
{
    char buffer[1024];

    va_list arglist;
    va_start(arglist, format);
    vsnprintf(buffer, sizeof(buffer), format.c_str(), arglist);
    va_end(arglist);
    
    std::string msg("ERROR: ");
    msg += buffer;
    if (m_log)
    {
        m_log->log(msg);
    }
    
    throw StackException(buffer);
}

// 
// These Txt* enums descdribe the TXT files which are read
// into Tables. These 3 input TXT files are processed into
// the real HdfStack datastructures.

// bounds = superpixel_bounds.txt
enum TxtBounds
{
    TXT_BOUNDS_Z = 0,
    TXT_BOUNDS_SPID = 1,
    TXT_BOUNDS_X = 2,
    TXT_BOUNDS_Y = 3,
    TXT_BOUNDS_WIDTH = 4,
    TXT_BOUNDS_HEIGHT = 5,
    TXT_BOUNDS_VOLUME = 6,
    NUM_TXT_BOUNDS_COLUMNS = 7
};

// segments = superpixel_to_segment_map.txt as a Table
enum TxtSegment
{
    TXT_SEGMENT_Z = 0,
    TXT_SEGMENT_SPID = 1,
    TXT_SEGMENT_SEGID = 2,
    NUM_TXT_SEGMENT_COLUMNS = 3
};

// bodies = segment_to_body_map.txt as a Table
enum TxtBody
{
    TXT_BODY_SEGID = 0,
    TXT_BODY_BODYID = 1,
    NUM_TXT_BODY_COLUMNS = 2
};

//
// "Compresses" a packed list of lists like m_segment_sp or m_body_seg
//
// First creates a map of every index which is actually in-use.  Then
// walks through the lists-of-lists doing one of two things:
//
// If list IS referenced, copies it to a new possibly lower index,
// adjusts the reference to the new index.
//
// if list IS NOT referenced, skips it, doesn't copy it.
//
class ListCompressor
{
public:
    ListCompressor(Table* indexes, uint32 column, Table* lists, LogFile* log);    
    
    void compress();
    
private:

    void createReverseMap(IntMap& map);
    
    // Table which contains the indexes
    Table* m_indexes;
    
    // Column in the index table which contains the indexes
    int m_column; 
    
    // The list-of-lists which we are compressing
    Table* m_lists;
    
    LogFile* m_log;
};

ListCompressor::ListCompressor(
    Table* indexes, uint32 column, Table* lists, LogFile* log) :
    m_indexes(indexes),
    m_column(column),
    m_lists(lists),
    m_log(log)
{
}

void ListCompressor::createReverseMap(IntMap& map)
{
    for (uint32 i = 0; i < m_indexes->getRows(); ++i)
    {
        uint32 index = m_indexes->getValue(i, m_column) ;
        
        if (index != EMPTY_VALUE)
        {
            if (map.find(index) == map.end())
            {
                map[index] = i;
            }
            else
            {
                // TODO: real error reporting, batchup any important
                // errors/warnings and present to the user at-once?
                printf("libstack: corrupt HDF-STACK row=%u points to "
                       "existing index=%u\n", i, index);                
            }
        }
    }   
}    

void ListCompressor::compress()
{
    IntMap map;    
    createReverseMap(map);
     
    uint32 out = 0;
    uint32 outrows = 0;
    
    enum State { NEWLIST, SKIPPING, COPYING };
    State state = NEWLIST;
 
    // Rewrite m_lists in-place
    for (uint32 i = 0; i < m_lists->getRows(); ++i)
    {
        if (state == NEWLIST)
        {        
            // At the start of a new list, see if this list is
            // referenced by anyone
            IntMap::iterator it = map.find(i);
            
            if (it == map.end())
            {
                // List is not referenced by anyone so skip it
                state = SKIPPING;
                
                if (m_log)
                {
                    m_log->log("skipping orphaned list at index=%u", i);                    
                }                
            }
            else
            {
                // List is referenced so copy it
                state = COPYING;
                
                uint32 reference = (*it).second;
                
                // And adjust index to point to the new
                // "out" location of the list
                m_indexes->setValue(reference, m_column, out);
            }
        }
        
        if (state == SKIPPING)
        {
            // Keep skipping till the end of this list
            if (m_lists->getValue(i, 0) == END_OF_LIST)
            {
                state = NEWLIST;
            }
        }
        else if (state == COPYING)
        {
            // Keep copying
            uint32 item = m_lists->getValue(i, 0);            
            m_lists->setValue(out++, 0, item);

            // Until we've copied the final value
            if (item == END_OF_LIST)
            {
                state = NEWLIST;
                outrows = out;
            }            
        }
    }

    // Truncate the table to include only the lists we just copied
    m_lists->truncateRows(outrows);
}

void compressListOfLists(
    Table* indexes, uint32 column, Table* lists, LogFile* log)
{
    ListCompressor compressor(indexes, column, lists, log);
    compressor.compress();
}

// load from an HDF5 file.  Used at runtime by Raveler.
void HdfStack::load(std::string path)
{
    HdfFile file;
    file.openForRead(path);
    
    StringList planeStrings;
    file.listDatasets("/superpixel", planeStrings);
    
    m_zmin = INT_MAX;
    m_zmax = 0;
    
    // Read the superpixel datasets names, convert to integers so we 
    // can get zmin/zmax and verify we have every in-between plane    
    std::set<int> planes;
    for (StringList::iterator it = planeStrings.begin(); 
         it != planeStrings.end(); ++it)
    {        
        uint32 plane;
        
        if (!StrToInt(*it, plane))
        {
            std::string("Bad superpixel dataset name");
        }
        
        planes.insert(plane);
        
        m_zmin = std::min(m_zmin, plane);
        m_zmax = std::max(m_zmax, plane);
    }
    
    // Read each plane into m_superpixel[z]
    for (uint32 z = m_zmin; z <= m_zmax; ++z)
    {
        // We should have found this above
        if (planes.find(z) == planes.end())
        {
            error("Missing plane %d", z);
        }
        
        std::string path = FormatString("superpixel/%d", z);

        m_superpixel[z] = file.readTable(path);
        
        //printf("Read plane %d -> %d rows\n", z, m_superpixel[z]->getRows());
    }
    
    m_segment = file.readTable("segment");
    m_segment_sp = file.readTable("segment_superpixels");
    m_body_index = file.readTable("body_index");
    m_body_seg = file.readTable("body_segments");
}


//
// load from the 3 TXT files, used when compiling
// the HDF5 for the first time.
//
void HdfStack::loadTXT(std::string root, std::string logpath)
{
    Table* bounds = NULL;
    Table* segments = NULL;
    Table* bodies = NULL;

    {
        PBT pbt("read %s", BOUNDS_FILE);
        // [PLANE, SPID, X, Y, WIDTH, HEIGHT, VOLUME]
        bounds = readtxt(root, BOUNDS_FILE, 7);
    }

    {
        PBT pbt("read %s", SEGMENT_FILE);
        // [PLANE, SPID, SEGID]
        segments = readtxt(root, SEGMENT_FILE, 3);
    }

    {
        PBT pbt("read %s", BODY_FILE);
        // [SEGID, BODYID]
        bodies = readtxt(root, BODY_FILE, 2);
    }
    
    create(bounds, segments, bodies, logpath);
}

void HdfStack::dumptables(Table* bounds, Table* segments, Table* bodies)
{
    printf("bounds--------------------------------\n");    
    for (uint32 i = 0; i < 10; ++i)
    {
        printf("%u %u %u %u %u %u %u\n",        
            bounds->getValue(i, TXT_BOUNDS_Z),
            bounds->getValue(i, TXT_BOUNDS_SPID),
            bounds->getValue(i, TXT_BOUNDS_X),
            bounds->getValue(i, TXT_BOUNDS_Y),
            bounds->getValue(i, TXT_BOUNDS_WIDTH),
            bounds->getValue(i, TXT_BOUNDS_HEIGHT),
            bounds->getValue(i, TXT_BOUNDS_VOLUME));
    }
    
    printf("segments--------------------------------\n");    
    for (uint32 i = 0; i < 10; ++i)
    {
        printf("%u %u %u\n",        
            segments->getValue(i, TXT_SEGMENT_Z),
            segments->getValue(i, TXT_SEGMENT_SPID),
            segments->getValue(i, TXT_SEGMENT_SEGID));
    }    

    printf("bodies--------------------------------\n");    
    for (uint32 i = 0; i < 10; ++i)
    {
        printf("%u %u\n",        
            bodies->getValue(i, TXT_BODY_SEGID),
            bodies->getValue(i, TXT_BODY_BODYID));
    }        
}

// 
// At the Python level Raveler represents segments like this:
//
//   (z1, spid1), (z2, spid2), ...
//
// This format allows for a segment to have superpixels on different
// planes.  But in practice normal segments are planar, all their
// superpixels live in one plane.
//
// HDF-STACK takes advantage of the normal case and stores segments
// like this:
//
//   (z, (spid1, spid2, ...))
//
// Only one plane is specified, so the segment has to be planar.
//
// At the Python level this is still converted into the old multi-z
// format.  And in fact one special segment does exist on multiple
// planes.  The zero segment consists of the zero superpixel on
// every plane.  StackData knows this and returns a hardcoded
// list of these superpixels when someone asks for the zero segment.
//
// Pre-HDF raveler there were some stacks which also had normal
// superpixels assigned to the zero segment. This was due to
// errors in segmentation, or using the retired "delete superpixel"
// tool.
//
// Since the "zero segment" is fixed in HDF Raveler, we cannot 
// represent this extra superpixels in the zero segment.  So
// during conversion to HDF we create new segment-body pairs for
// each zero segment superpixel we find.
//
// Raveler calls our getnewbodies() method to learn which new
// bodies we created during conversion.  Each of these bodies 
// is annoted so we know they came from the zero segment.
//
void remapZeroSuperpixels(Table* bounds, Table* segments, Table* bodies,
    IntVec& bodiesCreated, std::string logpath)
{
    uint32 maxsegid = 0;
    uint32 maxbodyid = 0;
    
    LogFile log(logpath, "zerosuperpixels.txt");

    // Find max segid
    for (uint32 i = 0; i < segments->getRows(); ++i)
    {
        maxsegid = std::max(maxsegid, segments->getValue(i, TXT_SEGMENT_SEGID));
    }

    // Find max bodyid
    for (uint32 i = 0; i < bodies->getRows(); ++i)
    {
        maxbodyid = std::max(maxbodyid, bodies->getValue(i, TXT_BODY_BODYID));
    }
    
    int newseg = 0;
    
    // For each segment
    for (uint32 i = 0; i < segments->getRows(); ++i)
    {
        if (segments->getValue(i, TXT_SEGMENT_SPID) == 0)
        {
            // Skip zero superpixels, these are allowed
            continue;
        }
            
        // If mapped to zero segment
        if (segments->getValue(i, TXT_SEGMENT_SEGID) == 0)
        {
            ++newseg;
            
            // Create a new segment and body
            ++maxsegid;
            ++maxbodyid;
            bodiesCreated.push_back(maxbodyid);
            
            // Write in new segment
            segments->setValue(i, TXT_SEGMENT_SEGID, maxsegid);

            // Write new segment to body mapping
            uint32 bodiesrow = bodies->getRows();            
            bodies->addRows(1);
            bodies->setValue(bodiesrow, TXT_BODY_SEGID, maxsegid);
            bodies->setValue(bodiesrow, TXT_BODY_BODYID, maxbodyid);
            
            // Log this superpixel and its new segment/body            
            uint32 z = segments->getValue(i, TXT_SEGMENT_Z);
            uint32 spid = segments->getValue(i, TXT_SEGMENT_SPID);
            log.log("%u %u %u %u", z, spid, maxsegid, maxbodyid);
        }
    }
        
    if (newseg > 0)
    {
        printf("WARN: Remapped %d zero superpixels\n", newseg);
        printf("WARN: See %s\n", log.getFilename());
    }
}

// 
// Create a new stack from 3 Tables which correspond to the 3 TXT
// files which we normally import from.
//
// bounds - 7 columns
//   Z SPID X Y WIDTH HEIGHT VOLUME
//
// segments - 3 columns
//   Z SPID SEGID
//
// bodies - 2 columns
//   SEGID BODYID
//
// NOTE: this was written BEFORE all the HdfStack API calls
// were added, so it's doing raw-surgery on the datastructures.
// We could maybe modernize it to use the API's where appropriate.
//
void HdfStack::create(Table* bounds, Table* segments, Table* bodies,
    std::string logpath)
{
    // dumptables(bounds, segments, bodies);    
    remapZeroSuperpixels(bounds, segments, bodies, m_newbodies, logpath);
    
    ext::hash_map<uint32, IntSet> boundSet;

    m_zmin = INT_MAX;
    m_zmax = 0;
    
    // Create boundSet[z][spid]
    for (uint32 i = 0; i < bounds->getRows(); ++i)
    {
        uint32 z = bounds->getValue(i, TXT_BOUNDS_Z);
        uint32 spid = bounds->getValue(i, TXT_BOUNDS_SPID);
        
        boundSet[z].insert(spid);        
        
        m_zmin = std::min(m_zmin, z);
        m_zmax = std::max(m_zmax, z);   
    }
    
    uint32 numsegments = bodies->getRows();
    
    ext::hash_map<uint32, IntMap> segMap;

    // Create a segMap[z][spids] = row
    for (uint32 i = 0; i < segments->getRows(); ++i)
    {
        uint32 z = segments->getValue(i, TXT_SEGMENT_Z);
        uint32 spid = segments->getValue(i, TXT_SEGMENT_SPID);
        
        segMap[z][spid] = i;
    }

    // maxspid[z] = <maxspid>
    IntMap maxspid;

    // Find maxspid for each plane
    for (uint32 i = 0; i < bounds->getRows(); ++i)
    {
        uint32 z = bounds->getValue(i, TXT_BOUNDS_Z);
        uint32 spid = bounds->getValue(i, TXT_BOUNDS_SPID);
    
        if (maxspid.find(z) == maxspid.end())
        {
            maxspid[z] = spid;
        }
        else
        {
            maxspid[z] = std::max(maxspid[z], spid);
        }
    }

    // Create a table for each plane, sized by maxspid
    for (IntMap::iterator it = maxspid.begin(); it != maxspid.end(); ++it)
    {
        uint32 z = it->first;
        uint32 highest = it->second;
        m_superpixel[z] = new Table(highest + 1, NUM_SUPERPIXEL_COLUMNS);
    }
    
    uint32 drop = 0;
    uint32 keep = 0;
    
    LogFile orphans(logpath, "superpixels-in-bounds-only.txt", 
        "# z spid volume");
    
    // Populate superpixel arrays from bounds
    for (uint32 i = 0; i < bounds->getRows(); ++i)
    {        
        uint32 z = bounds->getValue(i, TXT_BOUNDS_Z);
        uint32 spid = bounds->getValue(i, TXT_BOUNDS_SPID);

        // If not in the segments table
        if (segMap.find(z) == segMap.end() ||
            segMap[z].find(spid) == segMap[z].end())
        {
            uint32 volume = bounds->getValue(i, TXT_BOUNDS_VOLUME);
            
            orphans.log("%u %u %u", z, spid, volume);
            
            // Drop it, this (z, spid) appears in the bounds but not
            // in the segment table.
            drop++;
        }
        else
        {        
            keep++;
            Table* sptable = m_superpixel[z];
            
            // Copy bounds columns [2..6] to superpixel [0..4] 
            // X Y WIDTH HEIGHT VOLUME
            for (uint32 j = 2; j < bounds->getColumns(); ++j)
            {
                uint32 value = bounds->getValue(i, j);
                sptable->setValue(spid, j - 2, value);
            }
        }
    }
    
    if (drop > 0)
    {
        printf("WARN: Drop %u orphaned superpixels and keep %u\n", drop, keep);
        printf("WARN: See %s\n", orphans.getFilename());
    }

    uint32 maxsegid = 0;
    
    drop = keep = 0;
    LogFile phantoms(logpath, "superpixels-in-map-only.txt", "# z spid segid");
            
    // Add the segment into the superpixel arrays and compute maxsegid
    for (uint32 i = 0; i < segments->getRows(); ++i)
    {
        uint32 z = segments->getValue(i, TXT_SEGMENT_Z);
        uint32 spid = segments->getValue(i, TXT_SEGMENT_SPID);
        uint32 segid = segments->getValue(i, TXT_SEGMENT_SEGID);
        
        IntSet& set = boundSet[z];
        
        // If we do not have bounds for this spid
        if (set.find(spid) == set.end())
        {
            // This is a "phantom" spid, it exists in the spseg map but 
            // we don't have bounds for it, thus it was not present in the
            // superpixel_maps.
            drop++;
            phantoms.log("%u %u %u", z, spid, segid);
        }
        else
        {
            keep++;    
            m_superpixel[z]->setValue(spid, SUPERPIXEL_SEGID, segid);
            maxsegid = std::max(maxsegid, segid);
        }
    }
    
    if (drop > 0)
    {
        printf("Drop %u phantom superpixels and keep %u\n", drop, keep);
        printf("WARN: See %s\n", phantoms.getFilename());
    }
 
    // Create our segment table
    m_segment = new Table(maxsegid + 1, NUM_SEGMENT_COLUMNS);

   // Fill in segment z from the superpixel tables   
    for (TableMap::iterator it = m_superpixel.begin(); 
         it != m_superpixel.end(); ++it)
    {
        uint32 z = it->first;
        Table* table = it->second;
    
        for (uint32 spid = 0; spid < table->getRows(); ++spid)
        {
            uint32 segid = table->getValue(spid, SUPERPIXEL_SEGID);
        
            if (segid != EMPTY_VALUE)
            {
                m_segment->setValue(segid, SEGMENT_Z, z);
            }
        }
    }
 
    IntSet deletedSegs;
    
    drop = keep = 0;
    LogFile empty_segments(logpath, "empty-segments.txt", "# segid");
    
    // Fill in segment body from the bodies data
    for (uint32 i = 0; i < bodies->getRows(); ++i)
    {
        uint32 segid = bodies->getValue(i, TXT_BODY_SEGID);
        uint32 bodyid = bodies->getValue(i, TXT_BODY_BODYID);
    
        // If we filled in a SEGMENT_Z value already
        if (segid < m_segment->getRows() &&
            m_segment->getValue(segid, SEGMENT_Z) != EMPTY_VALUE)
        {
            // Then this segment has superpixels, so go ahead
            // and fill in the body
            m_segment->setValue(segid, SEGMENT_BODYID, bodyid);
            keep++;
        }
        else
        {
            // Segment was empty
            drop++;
            empty_segments.log("%u", segid);
            
            // Keep track so we can ignore these segments later            
            deletedSegs.insert(segid);
        }
    }
    
    numsegments -= deletedSegs.size();
    
    if (drop > 0)
    {
        printf("INFO: dropped %u empty segments, keep %u\n", drop, keep);
        printf("WARN: See %s\n", empty_segments.getFilename());
    }
 
    typedef std::list<uint32> IntList;
    typedef ext::hash_map<uint32, IntList> IntListMap;

    // Create the reverse map from segment to its list of spids
    // spids[segid] = [spid1, spid2, ...]
    IntListMap spids;
    
    uint32 numsp = 0;
    
    // For each plane
    for (TableMap::iterator it = m_superpixel.begin(); 
        it != m_superpixel.end(); ++it)
    {
        //uint32 z = it->first;
        Table* table = it->second;

        // For each spid on this plane
        for (uint32 spid = 0; spid < table->getRows(); ++spid)
        {
            // Get the segid from this spid
            uint32 segid = table->getValue(spid, SUPERPIXEL_SEGID);
            
            if (segid != EMPTY_VALUE)
            {
                IntListMap::iterator it = spids.find(segid);
            
                if (it == spids.end())
                {
                    IntList list;
                    list.push_back(spid);
                    spids[segid] = list;
                }
                else
                {
                    spids[segid].push_back(spid);
                }
                
                numsp += 1;
            }
        }
    }

    // segment_sp is big enough for each spid plus one terminator per segment
    uint32 segment_sp_size = numsp + spids.size();
    m_segment_sp = new Table(segment_sp_size, 1);

    // index into m_segment_sp
    uint32 spindex = 0;

    // Convert spids map into m_segment and m_segment_sp tables
    for (IntListMap::iterator it = spids.begin(); it != spids.end(); ++it)
    {   
        uint32 segid = it->first;
        IntList& spids = it->second;
    
        // Point to location in m_segment_sp
        m_segment->setValue(segid, SEGMENT_SPINDEX, spindex);
        
        // Write all spids into m_segment_sp    
        for (IntList::iterator it2 = spids.begin(); it2 != spids.end(); ++it2)
        {
            m_segment_sp->setValue(spindex++, 0, *it2);
        }
    
        // Terminate the list
        m_segment_sp->setValue(spindex++, 0, END_OF_LIST);
    }
    
    assert(spindex == segment_sp_size);

    // Create a list of segments for each body
    // segs[bodyid] = [segid1, segid2, ...]
    IntListMap segs;
    uint32 maxbodyid = 0;
    
    // For each segment that was deleted, keep track of the body
    // it was mapped to.  That body implicitly gets deleted if no
    // other segments map to it.
    IntSet pendingBodies;
    
    // need to be sure segment IDs don't appear twice in the list
    IntSet uniqueSegments;
    
    
    for (uint32 i = 0; i < bodies->getRows(); ++i)
    {
        uint32 segid = bodies->getValue(i, TXT_BODY_SEGID);
        uint32 bodyid = bodies->getValue(i, TXT_BODY_BODYID);
        
        // check for duplicated segments
        if (uniqueSegments.find(segid) != uniqueSegments.end())
        {
            printf("Error: segment %u mapped to more than one body in segment_to_body_map.txt; quitting.\n", segid);
            exit(1);
        }
        else
        {
            uniqueSegments.insert(segid);
        }
        
        // If we deleted this segment
        if (deletedSegs.find(segid) != deletedSegs.end())
        {
            // ignore this mapping, might result in body
            // getting deleted if it has no other segments
            pendingBodies.insert(bodyid);
            continue;
        }
    
        maxbodyid = std::max(maxbodyid, bodyid);
    
        if (segs.find(bodyid) == segs.end())
        {
            IntList list;
            list.push_back(segid);
            segs[bodyid] = list;
        }
        else
        {
            segs[bodyid].push_back(segid);
        }
    }
    
    drop = keep = 0;
    LogFile empty_bodies(logpath, "empty-bodies.txt", "# bodyid");
    
    // Check which bodies got deleted, just to report them
    for (IntSet::iterator it = pendingBodies.begin();
         it != pendingBodies.end(); ++it)
    {
        uint32 bodyid = *it;
        
        // If it's not in the map
        if (segs.find(bodyid) == segs.end())
        {
            drop++;
            empty_bodies.log("%u", bodyid);            
        }
    }
    
    // Compute keep for number of bodies
    keep = segs.size();
    
    if (drop > 0)
    {
        printf("WARN: dropped %u empty bodies, keep %u\n", drop, keep);
        printf("WARN: See %s\n", empty_bodies.getFilename());
    }

    uint32 numbodies = segs.size();
    uint32 body_index_size = maxbodyid + 1;

    // Directly indexed by bodyid.  Value is an index into m_body_seg.
    m_body_index = new Table(body_index_size, 1);
    
    // Big enough for each seg plus one terminator per body
    uint32 body_seg_size = numbodies + numsegments;
    m_body_seg = new Table(body_seg_size, 1);

    uint32 bodyindex = 0;
    
    printf("Convert segs into body_index and body_seg arrays...\n");
       
    // Convert segs into body_index and body_seg arrays
    for (IntListMap::iterator it = segs.begin(); it != segs.end(); ++it)
    {   
        uint32 bodyid = it->first;
        IntList& seglist = it->second;
    
        m_body_index->setValue(bodyid, 0, bodyindex);
        
        for (IntList::iterator it2 = seglist.begin(); it2 != seglist.end(); ++it2)
        {
            m_body_seg->setValue(bodyindex++, 0, *it2);
        }
    
        m_body_seg->setValue(bodyindex++, 0, END_OF_LIST);
    }
        
    assert(bodyindex == body_seg_size);
    
    printf("Verifying...\n");
    verify();
}


// Read a TXT file which is a table of integer values.
// Ignore comment (#) or blank lines. 
// Return a linear array of size [rows*columns].
Table* HdfStack::readtxt(std::string root, std::string fname, int columns)
{
    printf("Reading %s...\n", fname.c_str());

    std::string path = root + "/" + fname;

    char buffer[1024];
    std::ifstream fin(path.c_str());
    
    if (!fin)
    {
        printf("Error opening: %s\n", path.c_str());
        exit(1);
    }

    std::vector<char *> lines;

    while (fin.getline(buffer, 1024))
    {
        int len = strlen(buffer);
        if (len > 0 && buffer[0] != '#')
        {
            char *line = new char[len + 1];
            strcpy(line, buffer);
            lines.push_back(line);
        }
    }

    Table* table = new Table(lines.size(), columns);
    table->import(lines);
   
    for (uint32 i = 0; i < lines.size(); ++i)
    {
        delete lines[i];
    }

    return table;
}

bool HdfStack::verify(bool repair)
{
    const int MAX_ERRORS = 30;
    int errors = 0;
    
    // For every segment:
    // 1) Make sure the row is all-empty or all non-emtpy
    // 2) Make sure each superpixel in the segment refers back to the segment
    // 3) For the body which the segment maps to:
    //    a) Make sure the body exists
    //    b) Make sure body refers back to the segment
    //    
    for (uint32 segid = 0; segid < m_segment->getRows(); ++segid)
    {
        uint32 plane = m_segment->getValue(segid, SEGMENT_Z);
        uint32 bodyid = m_segment->getValue(segid, SEGMENT_BODYID);
        uint32 spindex = m_segment->getValue(segid, SEGMENT_SPINDEX);
        
        // If first-value is empty
        if (plane == EMPTY_VALUE)
        {
            // Make sure row is all-empty
            if (bodyid != EMPTY_VALUE)
            {
                printf("ERROR: segid=%u expected empty BODYID\n", segid);
                ++errors;
            }
            if (spindex != EMPTY_VALUE)
            {
                printf("ERROR: segid=%u expected empty SPINDEX\n", segid);
                ++errors;
            }
        }
        else
        {
            // Make sure row is non-empty
            if (bodyid == EMPTY_VALUE)
            {
                printf("ERROR: segid=%u expected non-empty BODYID\n", segid);
                ++errors;
            }
            
            if (spindex == EMPTY_VALUE)
            {
                printf("ERROR: segid=%u expected non-empty SPINDEX\n", segid);
                ++errors;
            }
            else
            {
                IntVec spids;
                getsuperpixelsinsegment(segid, spids);
                
                // Check every superpixel in the segment refers back to
                // us as the segment.
                for (uint32 i = 0; i < spids.size(); ++i)
                {
                    uint32 spid = spids[i];
                    uint32 sp_seg = getsegmentid(plane, spid);
                    
                    if (segid != sp_seg)
                    {
                        printf("ERROR: segid=%u has spid (%u, %u)\n",
                            segid, plane, spid);
                        printf("ERROR: superpixel (%u, %u) had segid=%u\n",
                            plane, spid, sp_seg);
                        
                        if (++errors > MAX_ERRORS)
                        {
                            break;
                        }
                    }
                }
            }
            
            // Check body exists
            uint32 bodyid = getsegmentbodyid(segid);
            checkBody(bodyid);

            // Check we are in the body's list of segments
            IntVec bodysegs;
            getsegments(bodyid, bodysegs);
            
            if (!contains(bodysegs, segid))
            {
                printf("ERROR: bodyid=%u doesn't contain segid=%u\n",
                    bodyid, segid);
                    
                if (++errors > MAX_ERRORS)
                {
                    break;
                }
            }
        }
        
        if (errors > MAX_ERRORS)
        {
            break;
        }
    }

    // For every plane
    for (TableMap::iterator it = m_superpixel.begin(); it != m_superpixel.end(); ++it)
    {
        uint32 z = (*it).first;
        Table* table = (*it).second;
    
        // For every superpixel in this plane
        for (uint32 i = 0; i < table->getRows(); ++i)
        {
            // If does not exist, skip
            if (table->getValue(i, SUPERPIXEL_X) == EMPTY_VALUE)
            {
                continue;
            }
            
            // If empty segment
            if (table->getValue(i, SUPERPIXEL_SEGID) == EMPTY_VALUE)
            {
                printf("ERROR: Superpixel has no segment plane=%u spid=%u\n", z, i);
                
                if (repair) 
                {
                    // Repair by creating a new segment/body and adding
                    // the superpixel to this new segment/body.
                    uint32 bodyid = createbody();
                    uint32 segid = createsegment();                    
                    
                    IntVec segids;
                    segids.push_back(segid);
                    
                    addsegments(segids, bodyid);
                    
                    IntVec spids;
                    spids.push_back(i);

                    setsuperpixels(segid, z, spids);                    
                    setsegmentid(z, i, segid);
                    
                    printf("REPAIR: added z=%u spid=%u to new body=%u\n",
                        z, i, bodyid);
                }
            }
        }
    }

    
    return errors == 0;
}

uint32 HdfStack::getnumsuperpixelsinplane(uint32 plane)
{
    Table *table = getSuperpixelTable(plane);
    
    uint32 total = 0;
    
    for (uint32 i = 0; i < table->getRows(); ++i)
    {
        if (table->getValue(i, SUPERPIXEL_X) != EMPTY_VALUE)
        {
            ++total;
        }
    }
    
    return total;    
}

uint32 HdfStack::getnumsuperpixelsinbody(uint32 bodyid)
{
    uint32 total = 0;
    
    // Get the segments in this body
    IntVec segments;
    getsegments(bodyid, segments);

    // For each segment
    for (IntVecIt segIt = segments.begin(); 
         segIt != segments.end(); ++segIt)
    {
        // Get the superpixels in this segment
        IntVec spids;
        getsuperpixelsinsegment(*segIt, spids);
        
        total += spids.size();
    }
    
    return total;
}

bool HdfStack::hassuperpixel(uint32 plane, uint32 spid)
{
    Table *table = getSuperpixelTable(plane);
    
    if (spid >= table->getRows())
    {
        return false;
    }
    
    if (table->getValue(spid, SUPERPIXEL_X) == EMPTY_VALUE)
    {
        return false;
    }
    
    return true;
}

uint32 HdfStack::createsuperpixel(uint32 plane)
{
    Table *table = getSuperpixelTable(plane);
    int spid = table->getRows();
    
    // Already initialized to all EMPTY_VALUE
    table->addRows(1);
    
    return spid;
}

void HdfStack::addsuperpixel(uint32 plane, uint32 spid, uint32 segid)
{
    if (m_log)
    {
        m_log->log("addsuperpixel(%u, %u, %u)", plane, spid, segid);
    }    
    
    Table* table = getSuperpixelTable(plane);
    
    if (segid == 0)
    {
        // We don't allow assignment to the zero segment
        error("addsuperpixel: cannot assign plane=%u spid=%u to the zero segment",
            plane, spid);
    }
    else
    {        
        checkSegment(segid);            
        
        table->setValue(spid, SUPERPIXEL_SEGID, segid);
        
        // Get the segment's current superpixels
        IntVec spids;
        getsuperpixelsinsegment(segid, spids);
        
        // Make sure sp is on the same plane
        if (spids.size() > 0)
        {
            uint32 segz = m_segment->getValue(segid, SEGMENT_Z);
            if (segz != plane)
            {
                error("Segment is on plane=%u, attempt "
                    "to add sp on plane=%u", segz, plane);
            }
        }    
        
        // Add this new spid if not already there
        if (!contains(spids, spid))
        {
            spids.push_back(spid);
                
            // Update the segment to have these new superpixels
            setsuperpixels(segid, plane, spids);            
        }
    }
}

void HdfStack::getsuperpixelsinplane(uint32 plane, IntVec& result)
{
    Table *table = getSuperpixelTable(plane);
    
    result.clear();
    for (uint32 i = 0; i < table->getRows(); ++i)
    {
        if (table->getValue(i, SUPERPIXEL_X) == EMPTY_VALUE)
        {
            // skip empty row
            continue;
        }
        
        result.push_back(i);
    }
}

void HdfStack::getsuperpixelbodiesinplane(uint32 plane, IntVec& result)
{
    Table *table = getSuperpixelTable(plane);
    
    result.clear();
    for (uint32 i = 0; i < table->getRows(); ++i)
    {
        if (table->getValue(i, SUPERPIXEL_X) == EMPTY_VALUE)
        {
            // skip empty row
            continue;
        }
        
        uint32 segid = table->getValue(i, SUPERPIXEL_SEGID);        
        uint32 bodyid = m_segment->getValue(segid, SEGMENT_BODYID);
        
        result.push_back(bodyid);
    }  
}

uint32 HdfStack::getmaxsuperpixelid(uint32 plane)
{
    Table *table = getSuperpixelTable(plane);
    return table->getRows() - 1;    
}


Table* HdfStack::getSuperpixelTableAndCheckRow(uint32 plane, uint32 spid)
{
    Table *table = getSuperpixelTable(plane);
    
    if (spid >= table->getRows())
    {
        error("spid=%u is out of range [0..%u]", spid, table->getRows() - 1);
    }
    
    // Only check bounds.. the SEGID column might be empty
    // if we haven't mapped it yet
    for (int i = 0; i < SUPERPIXEL_SEGID; ++i)
    {    
        if (table->getValue(spid, i) == EMPTY_VALUE)
        {
            error("spid=%u empty in column=%d", spid, i);
        }
    }
    
    return table;
}


Bounds HdfStack::getbounds(uint32 plane, uint32 spid)
{
    Table *table = getSuperpixelTableAndCheckRow(plane, spid);
    
    Bounds bounds;
    bounds.x = table->getValue(spid, SUPERPIXEL_X);
    bounds.y = table->getValue(spid, SUPERPIXEL_Y);
    bounds.width = table->getValue(spid, SUPERPIXEL_WIDTH);
    bounds.height = table->getValue(spid, SUPERPIXEL_HEIGHT);
    
    return bounds;    
}

uint32 HdfStack::getvolume(uint32 plane, uint32 spid)
{
    Table *table = getSuperpixelTableAndCheckRow(plane, spid);
    
    return table->getValue(spid, SUPERPIXEL_VOLUME);    
}

void HdfStack::setboundsandvolume(uint32 plane, uint32 spid, Bounds bounds, 
    uint32 volume)
{
    // Don't use getSuperpixelTableAndCheckRow because it could be empty
    Table *table = getSuperpixelTable(plane);
    
    table->setValue(spid, SUPERPIXEL_X, bounds.x);
    table->setValue(spid, SUPERPIXEL_Y, bounds.y);
    table->setValue(spid, SUPERPIXEL_WIDTH, bounds.width);
    table->setValue(spid, SUPERPIXEL_HEIGHT, bounds.height);
    table->setValue(spid, SUPERPIXEL_VOLUME, volume);    
}

bool HdfStack::hassegment(uint32 segid)
{
    if (segid >= m_segment->getRows())
    {
        return false;
    }
    
    // Check SEGMENT_SPINDEX, it indicates if segment exists.  Other
    // columns could be EMPTY_VALUE for existing but empty segment
    if (m_segment->getValue(segid, SEGMENT_SPINDEX) == EMPTY_VALUE)
    {
        return false;
    }
    
    return true;    
}

uint32 HdfStack::getsegmentid(uint32 plane, uint32 spid)
{
    Table* table = getSuperpixelTable(plane);
    
    if (spid < table->getRows())
    {
        uint32 segid = table->getValue(spid, SUPERPIXEL_SEGID);
        
        if (m_log)
        {
            m_log->log("getsegmentid(%u, %u) -> %u", plane, spid, segid);
        }
        
        return segid;
    }
    else
    {
        error("getsegmentid(%u, %u) invalid spid=%u", plane, spid, spid);
        return 0;
    }
}

void HdfStack::setsegmentid(uint32 plane, uint32 spid, uint32 segid)
{
    if (segid == 0)
    {
        error("setsegmentid: cannot assign plane=%u spid=%u to the zero segment.",
            plane, spid);
    }

    if (m_log)
    {
        m_log->log("setsegmentid(%u, %u, %u)", plane, spid, segid);
    }
    
    Table* table = getSuperpixelTable(plane);
    
    if (spid < table->getRows())
    {
        table->setValue(spid, SUPERPIXEL_SEGID, segid);        
    }
    else
    {
        error("setsegmentid(%u, %u, %u) invalid spid=%u", 
            plane, spid, segid, spid);
    }    
}

uint32 HdfStack::getplane(uint32 segid)
{
    checkSegment(segid);
    
    if (segid == 0)
    {
        // Segment zero has no plane per se, segment zero is the zero
        // spid on every plane.
        error("getplane(0) - zero segment has no plane");
    }    
    
    uint32 plane = m_segment->getValue(segid, SEGMENT_Z);    

    if (m_log)
    {
        m_log->log("getplane(%u) -> %u", segid, plane);
    }
    
    return plane;
}

void HdfStack::getsuperpixelsinsegment(uint32 segid, IntVec& result)
{
    checkSegment(segid);
        
    uint32 sp_index = m_segment->getValue(segid, SEGMENT_SPINDEX);
    
    if (sp_index == EMPTY_VALUE)
    {
        error("segid does not exist");
    }
    
    getList(m_segment_sp, sp_index, result);
    
    if (m_log)
    {
        std::string vec = formatIntVec(result);
        m_log->log("getsuperpixelsinsegment(%u) -> %s", segid, vec.c_str());
    }
}

void HdfStack::getsuperpixelsinbody(uint32 bodyid, IntVec& planes, IntVec& spids)
{
    IntVec segments;
    getsegments(bodyid, segments);
    
    planes.clear();
    spids.clear();

    // For each segment in the body
    for (IntVec::iterator it = segments.begin(); it != segments.end(); ++it)
    {
        // Get spids for this segment
        IntVec segsp;
        getsuperpixelsinsegment(*it, segsp);
        
        // Get the single plane for this segment
        uint32 z = getplane(*it);
        
        // Same plane for each spid
        for (uint32 i = 0; i < segsp.size(); ++i)
        {
            planes.push_back(z);
        }
        
        // Append the spids
        std::copy(segsp.begin(), segsp.end(),
          std::back_insert_iterator<IntVec>(spids));        
    }
    
    assert(planes.size() == spids.size());
}

void HdfStack::getsuperpixelsinbodyinplane(uint32 bodyid, uint32 plane, IntVec& spids)
{
    IntVec segments;
    getsegments(bodyid, segments);
    
    spids.clear();

    // For each segment in the body
    for (IntVec::iterator it = segments.begin(); it != segments.end(); ++it)
    {
        // Get the single plane for this segment
        uint32 z = getplane(*it);
        
        if (z != plane)
        {
            continue;
        }
        
        // Get spids for this segment
        IntVec segsp;
        getsuperpixelsinsegment(*it, segsp);
        
        // Append the spids
        std::copy(segsp.begin(), segsp.end(),
            std::back_insert_iterator<IntVec>(spids));        
    }
}

void HdfStack::getList(Table* table, uint32 index, IntVec& result)
{
    result.clear();
    
    uint32 num_rows = table->getRows();    
    uint32 i = index;
    
    // Copy out the values
    while (table->getValue(i, 0) != END_OF_LIST)
    {
        result.push_back(table->getValue(i++, 0));
        
        if (index >= num_rows)
        {
            error("Missing END_OF_LIST");
        }
    }
}

void HdfStack::checkPlane(uint32 plane)
{
    // Check that plane exists
    if (m_superpixel.find(plane) == m_superpixel.end())
    {
        error("plane=%u does not exist", plane);
    }        
}

void HdfStack::checkSuperpixel(uint32 plane, uint32 spid)
{    
    if (!hassuperpixel(plane, spid))
    {
        error("Superpixel plane=%u spid=%u does not exist", plane, spid);
    }    
}

void HdfStack::checkSegment(uint32 segid)
{
    if (segid >= m_segment->getRows())
    {
        error("segid=%u outside range [0..%u)", segid, 
            m_segment->getRows());
    }

    // SEGMENT_SPINDEX will be not EMPTY_VALUE if segment exists
    // SEGMENT_Z could be EMPTY_VALUE if segment has no superpixels
    // SEGMENT_BODYID could be EMPTY_VALUE is segment has no body
    if (m_segment->getValue(segid, SEGMENT_SPINDEX) == EMPTY_VALUE)
    {
        error("segid=%u does not exist in range [0..%u)", segid,
            m_segment->getRows());
    }    
}

void HdfStack::checkBody(uint32 bodyid)
{
    if (bodyid >= m_body_index->getRows())
    {
        error("bodyid=%u not in range [0..%u)", bodyid,
            m_body_index->getRows());
    }
    
    if (m_body_index->getValue(bodyid, 0) == EMPTY_VALUE)
    {
        error("bodyid=%u does not exist in range [0..%u)", bodyid,
            m_body_index->getRows());
    }    
}

void HdfStack::setsuperpixels(uint32 segid, uint32 plane, const IntVec& spids)
{
    if (m_log)
    {
        std::string vec = formatIntVec(spids);
        m_log->log("setsuperpixels(%u, %u, %s)", segid, plane, vec.c_str());
    }

    // Can't call checkSegment because segment might not exist yet, but
    // but we can at least check the range 
    if (segid >= m_segment->getRows())
    {
        error("setsuperpixels segid=%u outside range [0..%u)", segid, 
            m_segment->getRows());
    }

    if (plane != EMPTY_VALUE)
    {
        checkPlane(plane);
    }
    
    // For speed we only APPEND new lists, so the previous list 
    // is orphaned and is dead-space.  We cleanup dead-space 
    // before saving to the HDF5.
    uint32 spindex = appendList(m_segment_sp, spids);
    
    m_segment->setValue(segid, SEGMENT_Z, plane);
    m_segment->setValue(segid, SEGMENT_SPINDEX, spindex);
}

uint32 HdfStack::createsegment()
{
    int segid = m_segment->getRows();
    m_segment->addRows(1);
    
    m_segment->setValue(segid, SEGMENT_Z, EMPTY_VALUE);
    m_segment->setValue(segid, SEGMENT_BODYID, EMPTY_VALUE); 
    
    // Init the superpixels to an empty list
    IntVec spids;
    setsuperpixels(segid, EMPTY_VALUE, spids);    

    if (m_log)
    {
        m_log->log("createsegment() -> %u", segid);
    }            
    
    return segid;
}

//
// NOTE ABOUT EMPTY LISTS:
//
// An empty list is stored as an index directly to an 
// END_OF_LIST entry. There are other more compact 
// options but this keeps things nice and uniform.
//
uint32 HdfStack::appendList(Table* table, const IntVec& ids)
{
    int index = table->getRows();
    
    // Length plus terminator
    uint32 new_rows = ids.size() + 1;    
    table->addRows(new_rows);
    
    // Copy values - if empty there is nothing to copy
    for (uint32 i = 0; i < ids.size(); ++i)
    {
        table->setValue(index + i, 0, ids[i]);
    }
    
    // Terminate it
    table->setValue(index + ids.size(), 0, END_OF_LIST);
    
    return index;
}

void HdfStack::garbageCollect()
{
    if (m_log)
    {
        m_log->log("garbage collect start");
    }
    
    // Delete superpixels which have no pixels (which have empty bounds)
    for (TableMap::iterator it = m_superpixel.begin(); it != m_superpixel.end(); ++it)
    {
        uint32 z = (*it).first;
        Table* table = (*it).second;
        
        for (uint32 i = 0; i < table->getRows(); ++i)
        {
            if (table->getValue(i, SUPERPIXEL_X) == EMPTY_VALUE)
            {
                continue;
            }
            
            if (table->getValue(i, SUPERPIXEL_VOLUME) == 0)
            {
                if (m_log)
                {
                    m_log->log("deleting empty spid=%u", i);
                }    
                
                // Remove from segment
                removesuperpixel(z, i);
                
                // Clear the whole row
                table->setValue(i, SUPERPIXEL_X, EMPTY_VALUE);
                table->setValue(i, SUPERPIXEL_Y, EMPTY_VALUE);
                table->setValue(i, SUPERPIXEL_WIDTH, EMPTY_VALUE);
                table->setValue(i, SUPERPIXEL_HEIGHT, EMPTY_VALUE);
                table->setValue(i, SUPERPIXEL_VOLUME, EMPTY_VALUE);
                table->setValue(i, SUPERPIXEL_SEGID, EMPTY_VALUE);
            }
        }
    }
    
    // Delete segments which have no superpixels
    for (uint32 i = 0; i < m_segment->getRows(); ++i)
    {
        uint32 spindex = m_segment->getValue(i, SEGMENT_SPINDEX);
        
        // If segment has no superpixels
        if (spindex != EMPTY_VALUE && 
            m_segment_sp->getValue(spindex, 0) == END_OF_LIST)
        {
            deletesegment(i);
        }
    }
    
    // Delete bodies which have no segments
    for (uint32 i = 0; i < m_body_index->getRows(); ++i)
    {
        uint32 bodyindex = m_body_index->getValue(i, 0);
        
        // If body has no segments
        if (bodyindex != EMPTY_VALUE &&
            m_body_seg->getValue(bodyindex, 0) == END_OF_LIST)
        {
            if (m_log)
            {
                printf("deleting empty body=%u", i);
                m_log->log("deleting empty body=%u", i);
            }    
            
            m_body_index->setValue(i, 0, EMPTY_VALUE);
        }
    }
    
    // Compress our two lists of lists    
    
    if (m_log)
    {
        m_log->log("compressing m_segment_sp");
    }    
    
    compressListOfLists(m_segment, SEGMENT_SPINDEX, m_segment_sp, m_log);
    
    if (m_log)
    {
        m_log->log("compressing m_body_seg");
    }    
    
    compressListOfLists(m_body_index, 0, m_body_seg, m_log);

    if (m_log)
    {
        m_log->log("garbage collect end");
    }    
}

void HdfStack::save(std::string path, uint isbackup)
{
    // First get rid of empty entities, and compress down the 
    // reverse-maps to cleanup unused/dead space.
    // Only do this if it's not a backup!  For backups, we need to
    // keep the "empty" entities, because an undo might resurrect them
    if (!isbackup)
        garbageCollect();
    
    HdfFile file;
    file.openForWrite(path);
    
    file.createGroup("superpixel");
    for (TableMap::iterator it = m_superpixel.begin(); it != m_superpixel.end(); ++it)
    {
        uint32 z = (*it).first;
        Table* superpixels = (*it).second;
        char name[64];
        sprintf(name, "superpixel/%d", z);        
        file.writeDataset(name, *superpixels);
    }

    file.writeDataset("segment", *m_segment);
    file.writeDataset("segment_superpixels", *m_segment_sp);
    file.writeDataset("body_index", *m_body_index);
    file.writeDataset("body_segments", *m_body_seg);
}    

Table* HdfStack::getSuperpixelTable(uint32 plane)
{
    TableMap::iterator it = m_superpixel.find(plane);
       
    if (it == m_superpixel.end())
    {
        error("plane=%u does not exist", plane);
        return NULL;
    }
    else
    {
        return (*it).second;
    }
}

uint32 HdfStack::getsegmentbodyid(uint32 segid)
{
    checkSegment(segid);
    
    uint32 bodyid = m_segment->getValue(segid, SEGMENT_BODYID);
    
    if (m_log)
    {
        m_log->log("getsegmentbodyid(%u) -> %u", segid, bodyid);
    }                
    
    return bodyid;
}

void HdfStack::getsegments(uint32 bodyid, IntVec& result)
{
    checkBody(bodyid);
        
    uint32 index = m_body_index->getValue(bodyid, 0);
    
    getList(m_body_seg, index, result);

    if (m_log)
    {
        std::string vec = formatIntVec(result);
        m_log->log("getsegments(%u) -> %s", bodyid, vec.c_str());
    }
}

void HdfStack::setsegments(uint32 bodyid, IntVec& segments)
{
    checkBody(bodyid);

    // For speed we only APPEND new lists, so the previous list 
    // is orphaned and is dead-space.  We cleanup dead-space 
    // before saving to the HDF5.
    uint32 index = appendList(m_body_seg, segments);
    m_body_index->setValue(bodyid, 0, index);
    
    if (m_log)
    {
        std::string vec = formatIntVec(segments);
        m_log->log("setsegments(%u, %s)", bodyid, vec.c_str());
    }
}

uint32 HdfStack::createbody()
{
    int bodyid = m_body_index->getRows();
    m_body_index->addRows(1);
    
    // Set zero temporarily so checkBody in setsegments 
    // doesn't complain.  setsegment will write in the
    // real value.
    m_body_index->setValue(bodyid, 0, 0);    
       
    // Create EMPTY list of segments.
    IntVec segments;
    setsegments(bodyid, segments);

    if (m_log)
    {
        m_log->log("createbody() -> %u", bodyid);
    }
    
    return bodyid;
}

bool HdfStack::hasbody(uint32 bodyid)
{
    if (bodyid >= m_body_index->getRows())
    {
        return false;
    }
    
    if (m_body_index->getValue(bodyid, 0) == EMPTY_VALUE)
    {
        return false;
    }
    
    return true;    
}

void HdfStack::addsegments(const IntVec& segids, uint32 bodyid)
{
    if (bodyid == 0)
    {
        // Zero body is special and no one can assign to it
        error("Cannot assign segments to zero body");
    }

    IntVec existing;
    getsegments(bodyid, existing);
 
    checkBody(bodyid);
    
    for (uint32 i = 0; i < segids.size(); ++i)
    {
        uint32 segid = segids[i];
        
        checkSegment(segid);
        
        m_segment->setValue(segid, SEGMENT_BODYID, bodyid);
    
        // If not already there, add    
        if (!contains(existing, segid))
        {
            existing.push_back(segid);
        }
    }
    
    setsegments(bodyid, existing);
}

// 
// private, not part of the API
//
void HdfStack::removesuperpixel(uint32 plane, uint32 spid)
{
    if (m_log)
    {
        m_log->log("removesuperpixel(%u, %u)", plane, spid);
    }
    
    uint32 segid = getsegmentid(plane, spid);
    
    // Get segments's list of superpixels
    IntVec spids;
    getsuperpixelsinsegment(segid, spids);
    
    // Find this spid in the list
    IntVec::iterator it = std::find(spids.begin(), spids.end(), spid);
    
    if (it == spids.end())
    {
        error("spid=%u not in sp list", spid);
    }
    
    spids.erase(it);
    
    // Set revised segments
    setsuperpixels(segid, plane, spids);
}

// 
// private, not part of the API
//
void HdfStack::removesegment(uint32 segid)
{
    if (m_log)
    {
        m_log->log("removesegment(%u)", segid);
    }
    
    uint32 bodyid = getsegmentbodyid(segid);
    
    // Get body's list of segments
    IntVec segments;
    getsegments(bodyid, segments);
    
    // Find segid in the list
    IntVec::iterator it = std::find(segments.begin(), segments.end(), segid);
    
    if (it == segments.end())
    {
        error("segid=%u not in bodies list", segid);
    }
    
    segments.erase(it);
    
    // Set revised segments
    setsegments(bodyid, segments);    
}

void HdfStack::deletesegment(uint32 segid)
{
    if (m_log)
    {
        m_log->log("deletesegment(%u)", segid);
        uint32 plane = m_segment->getValue(segid, SEGMENT_Z);
        uint32 bodyid = m_segment->getValue(segid, SEGMENT_BODYID);
        uint32 spindex = m_segment->getValue(segid, SEGMENT_SPINDEX);
        m_log->log("row = %u %u %u", plane, bodyid, spindex);
    }
    
    if (hassegment(segid))
    {
        // This remove the segment from the bodie's list of segments
        removesegment(segid);
        
        // Set the row to be empty.  This orphans our list of
        // superpixels which is fine.
        m_segment->setValue(segid, SEGMENT_Z, EMPTY_VALUE);
        m_segment->setValue(segid, SEGMENT_BODYID, EMPTY_VALUE);
        m_segment->setValue(segid, SEGMENT_SPINDEX, EMPTY_VALUE);
    }    
}

uint32 HdfStack::getnumbodies()
{
    uint32 count = 0;
    
    for (uint32 i = 0; i < m_body_index->getRows(); ++i)
    {
        if (m_body_index->getValue(i, 0) != EMPTY_VALUE)
        {
            ++count;
        }
    }
    
    return count;
}

uint32 HdfStack::getnumbodiesnonzero()
{
    uint32 count = 0;
    
    for (uint32 i = 1; i < m_body_index->getRows(); ++i)
    {
        if (m_body_index->getValue(i, 0) != EMPTY_VALUE)
        {
            ++count;
        }
    }
    
    return count;
}

void HdfStack::getnewbodies(IntVec& result)
{
    result = m_newbodies;
}

void HdfStack::getallbodies(IntVec& result)
{
    result.clear();

    for (uint32 i = 0; i < m_body_index->getRows(); ++i)
    {
        if (m_body_index->getValue(i, 0) != EMPTY_VALUE)
        {
            result.push_back(i);
        }
    }
    
    if (m_log)
    {
        m_log->log("getallbodies() -> <%d bodies>", result.size());
    }    
}

uint32 HdfStack::getnumsegments()
{
    uint32 count = 0;
    
    for (uint32 i = 0; i < m_segment->getRows(); ++i)
    {
        if (m_segment->getValue(i, SEGMENT_Z) != EMPTY_VALUE)
        {
            ++count;
        }
    }
    
    return count;
}

void HdfStack::getallsegments(IntVec& result)
{
    result.clear();

    for (uint32 i = 0; i < m_segment->getRows(); ++i)
    {
        if (m_segment->getValue(i, SEGMENT_Z) != EMPTY_VALUE)
        {
            result.push_back(i);
        }
    }
    
    if (m_log)
    {
        m_log->log("getallsegments() -> <%d segments>", result.size());
    }    
}

void HdfStack::getplanelimits(uint32 bodyid, uint32& zmin, uint32& zmax)
{
    zmin = EMPTY_VALUE;
    zmax = 0;
    
    if (bodyid == 0)
    {
        // Body 0 is assumed to occupy the full stack
        zmin = m_zmin;
        zmax = m_zmax;
        return;
    }
    
    IntVec segments;
    getsegments(bodyid, segments);
 
    // For each segment in the body
    for (IntVec::iterator it = segments.begin(); it != segments.end(); ++it)
    {    
        uint32 z = m_segment->getValue(*it, SEGMENT_Z);   
        
        if (z != EMPTY_VALUE)
        {        
            zmin = std::min(zmin, z);
            zmax = std::max(zmax, z);
        }
    }
    
    // If body was empty, set both min/max to EMPTY_VALUE
    if (zmin == EMPTY_VALUE)
    {
        zmax = EMPTY_VALUE;
    }
}

void HdfStack::getallplanelimits(IntVec& bodyids, IntVec& zmin, IntVec& zmax)
{
    getallbodies(bodyids);
    
    // For each body
    for (IntVec::iterator it = bodyids.begin(); it != bodyids.end(); ++it)
    {
        uint32 planezmin;
        uint32 planezmax;
        
        getplanelimits(*it, planezmin, planezmax);
        
        zmin.push_back(planezmin);
        zmax.push_back(planezmax);
    }
}

uint64 HdfStack::getsegmentvolume(uint32 segid)
{
    uint64 volume = 0;
    uint32 z = getplane(segid);
    
    IntVec spids;
    getsuperpixelsinsegment(segid, spids);
    
    // For each spid
    for (IntVecIt spidIt = spids.begin(); 
         spidIt != spids.end(); ++spidIt)
    {
        volume += getvolume(z, *spidIt);
    }
    
    return volume;    
}

uint64 HdfStack::getbodyvolume(uint32 bodyid)
{
    uint64 volume = 0;
    
    if (bodyid == 0)
    {
        // Zero body is special, get the zero segment on each plane
        for (uint32 z = m_zmin; z <= m_zmax; ++z)
        {
            volume += getvolume(z, 0);
        }       
    }
    else
    {
        // Get the segments in this body
        IntVec segments;    
        getsegments(bodyid, segments);
    
        // For each segment
        for (IntVecIt segIt = segments.begin(); 
             segIt != segments.end(); ++segIt)
        {
            volume += getsegmentvolume(*segIt);
        }
    }
    
    return volume;
}

void HdfStack::getBodyGeometryXZ(uint32 bodyid, IntVec& x, IntVec& z)
{
    // Get the segments in this body
    IntVec segments;
    getsegments(bodyid, segments);

    // For each segment
    for (IntVecIt segIt = segments.begin(); 
         segIt != segments.end(); ++segIt)
    {
        // Get the superpixels in this segment
        IntVec spids;
        getsuperpixelsinsegment(*segIt, spids);
        
        // Get plane for this segment
        uint32 plane = getplane(*segIt);
        
        // Get bounds of each superpixel
        for (uint32 i = 0; i < spids.size(); ++i)
        {
            uint32 spid = spids[i];
            
            Bounds bounds = getbounds(plane, spid);
            
            // lower left
            x.push_back(bounds.x);
            z.push_back(plane);
            
            // lower right
            x.push_back(bounds.x + bounds.width);
            z.push_back(plane);
            
            // upper right
            x.push_back(bounds.x + bounds.width);
            z.push_back(plane + 1);
            
            // upper left
            x.push_back(bounds.x);
            z.push_back(plane + 1);             
        }
    }
}

void HdfStack::getBodyGeometryYZ(uint32 bodyid, IntVec& y, IntVec& z)
{
    // Get the segments in this body
    IntVec segments;
    getsegments(bodyid, segments);

    // For each segment
    for (IntVecIt segIt = segments.begin(); 
         segIt != segments.end(); ++segIt)
    {
        // Get the superpixels in this segment
        IntVec spids;
        getsuperpixelsinsegment(*segIt, spids);
        
        // Get plane for this segment
        uint32 plane = getplane(*segIt);
        
        // Get bounds of each superpixel
        for (uint32 i = 0; i < spids.size(); ++i)
        {
            uint32 spid = spids[i];
            
            Bounds bounds = getbounds(plane, spid);
            
            // lower left
            y.push_back(bounds.y);
            z.push_back(plane);
            
            // lower right
            y.push_back(bounds.y + bounds.height);
            z.push_back(plane);
            
            // upper right
            y.push_back(bounds.y + bounds.height);
            z.push_back(plane + 1);
            
            // upper left
            y.push_back(bounds.y);
            z.push_back(plane + 1);             
        }
    }
}

void HdfStack::getBodyBounds(uint32 bodyid, IntVec& z, BoundsVec& bounds)
{
    // Get the segments in this body
    IntVec segments;
    getsegments(bodyid, segments);

    // For each segment
    for (IntVecIt segIt = segments.begin(); 
         segIt != segments.end(); ++segIt)
    {
        // Get the superpixels in this segment
        IntVec spids;
        getsuperpixelsinsegment(*segIt, spids);
        
        // Get plane for this segment
        uint32 plane = getplane(*segIt);
        
        // Get bounds of each superpixel
        for (uint32 i = 0; i < spids.size(); ++i)
        {
            uint32 spid = spids[i];
            
            Bounds b = getbounds(plane, spid);
            
            z.push_back(plane);
            bounds.push_back(b);
        }
    }    
}

inline void addCell(IntVec& vec, uint32 numverts, uint32 a, uint32 b, uint32 c)
{
    vec.push_back(3);
    vec.push_back(numverts + a);
    vec.push_back(numverts + b);
    vec.push_back(numverts + c);
}   

inline void addVert(Float3Vec& vec, float x, float y, float z)
{
    vec.push_back(float3(x, y, z));
}

void HdfStack::getBodyBoundsVTK(uint32 bodyid, float zaspect, Float3Vec& verts, 
    IntVec& tris)
{
    // Get the segments in this body
    IntVec segments;
    getsegments(bodyid, segments);
    
    uint32 numverts = 0;

    // For each segment
    for (IntVecIt segIt = segments.begin(); 
         segIt != segments.end(); ++segIt)
    {
        // Get the superpixels in this segment
        IntVec spids;
        getsuperpixelsinsegment(*segIt, spids);
        
        // Get plane for this segment
        uint32 plane = getplane(*segIt);
                    
        // Get bounds of each superpixel
        for (uint32 i = 0; i < spids.size(); ++i)
        {
            Bounds b = getbounds(plane, spids[i]);
            
            if (b.isEmpty())
            {
                continue;
            }
            
            float x0 = b.x0();
            float y0 = b.y0();
            float x1 = b.x1();
            float y1 = b.y1();            
            float z = -float(plane);
            
            // Add the 8 corner points of the bounds box
            addVert(verts, x0, y0, (z + 1) * zaspect);
            addVert(verts, x1, y0, (z + 1) * zaspect);
            addVert(verts, x1, y1, (z + 1) * zaspect);
            addVert(verts, x0, y1, (z + 1) * zaspect);
            addVert(verts, x0, y0, z * zaspect);
            addVert(verts, x1, y0, z * zaspect);
            addVert(verts, x1, y1, z * zaspect);
            addVert(verts, x0, y1, z * zaspect);
            
            // Using these 8 corner vertices, make 12 face triangles
            // Create them CCW as looking from the outside at the box
            addCell(tris, numverts, 0, 1, 2);
            addCell(tris, numverts, 2, 3, 0);
            addCell(tris, numverts, 2, 1, 5);
            addCell(tris, numverts, 6, 2, 5);
            addCell(tris, numverts, 1, 0, 4);
            addCell(tris, numverts, 1, 4, 5);
            addCell(tris, numverts, 7, 3, 2);
            addCell(tris, numverts, 6, 7, 2);
            addCell(tris, numverts, 3, 7, 4);
            addCell(tris, numverts, 0, 3, 4);
            addCell(tris, numverts, 7, 5, 4);
            addCell(tris, numverts, 6, 5, 7);

            numverts += 8;
        }
    }    
}

void HdfStack::exportstl(uint32 bodyid, const char* path, float zaspect)
{
    FILE *outf = fopen(path, "wb");
    
    if (!outf)
    {
        error("Cannot open output file '%s'", path);
    }
    
    STLMesh mesh;
    
    // Get the segments in this body
    IntVec segments;
    getsegments(bodyid, segments);

    // For each segment
    for (IntVecIt segIt = segments.begin(); 
         segIt != segments.end(); ++segIt)
    {
        // Get the superpixels in this segment
        IntVec spids;
        getsuperpixelsinsegment(*segIt, spids);
        
        // Get plane for this segment
        uint32 plane = getplane(*segIt);
        
        // Get bounds of each superpixel
        for (uint32 i = 0; i < spids.size(); ++i)
        {
            uint32 spid = spids[i];
            
            Bounds bounds = getbounds(plane, spid);
            
            if (!bounds.isEmpty())
            {            
                mesh.addcube(bounds, plane, zaspect);
            }
        }
    }
    
    mesh.write(outf);
    fclose(outf);
}
