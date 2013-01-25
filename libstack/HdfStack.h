//
// HdfStack.h
//

#pragma once

#include "Table.h"
#include <stdexcept>


class StackException : public std::runtime_error
{
    public:
        StackException(std::string const& msg):
            std::runtime_error(msg)
        {}
};

class LogFile;


//
// In-memory version of an HDF-STACK
// 
// Contains superpixel bounding boxes and superpixel, segment, body mappings.
//
// Two main users of HdfStack:
//
// createstack utility - calls loadTXT to parse the 3 TXT files and 
// then calls write() to save the HDF5 file.
//
// Raveler - calls load() to load the HDF5 file, then does arbitrarily
// many read and modify operations, then write() to save.
//
class HdfStack
{
    public:
        HdfStack();
        ~HdfStack();
        
        // Load from HDF5 
        void load(std::string path);
        
        // Load all data from the 3 TXT files, used when compiling 
        // the stack for the first time.
        void loadTXT(std::string root, std::string logpath);

        // Create from Tables.  This is for conversion from TXT files
        // or from legacy Raveler sessions.
        // 
        // bounds -> [PLANE, SPID, X, Y, WIDTH, HEIGHT, VOLUME]
        // segments -> [PLANE, SPID, SEGID]
        // bodies -> [SEGID, BODYID]
        //
        void create(Table* bounds, Table* segments, Table* bodies,
            std::string logpath="");
        
        // Get bodies added during create() step.  These are bodies
        // which didn't exist in the import tables.  These are bodies
        // we created to old superpixels which were previously part
        // of the zero segment.
        void getnewbodies(IntVec& result);
        
        // Return true if stack is internally consinstant, otherwise
        // any errors are printed.
        bool verify(bool repair = false);
        
        // Write the HDF5 file to the given path
        void save(std::string path, uint isbackup);
        
        // Lowest and highest numbered planes in the stack
        uint32 getzmin() const { return m_zmin; }
        uint32 getzmax() const { return m_zmax; }
        
        // Get count of all the superpixels on a given plane
        uint32 getnumsuperpixelsinplane(uint32 plane);
        
        // Get count of all the superpixels in a given body
        uint32 getnumsuperpixelsinbody(uint32 bodyid);
        
        // Return true if superpixel exists
        bool hassuperpixel(uint32 plane, uint32 spid);
        
        // Create a new superpixel, return the new spid
        uint32 createsuperpixel(uint32 plane);
                
        // Add superpixel to the given segment
        void addsuperpixel(uint32 plane, uint32 spid, uint32 segid);
                
        // Get highest numbered superpixel on a given plane
        uint32 getmaxsuperpixelid(uint32 plane);
        
        // Get (x, y, width, height) bounding area of this superpixel.
        Bounds getbounds(uint32 plane, uint32 spid);
        
        // Return exact area of the superpixel
        uint32 getvolume(uint32 plane, uint32 spid);
        
        // Set the bounds and volume of a given superpixel
        void setboundsandvolume(uint32 plane, uint32 spid, Bounds bounds, 
            uint32 volume);
        
        // Return true if segment exists
        bool hassegment(uint32 segid);
        
        // Get segment id from a (plane, spid)        
        uint32 getsegmentid(uint32 plane, uint32 spid);
        
        // Set segment id for a given (plane, spid)
        void setsegmentid(uint32 plane, uint32 spid, uint32 segid);
        
        // Get plane for the given segment
        uint32 getplane(uint32 segid);
        
        // Get all superpixels on the given plane
        void getsuperpixelsinplane(uint32 plane, IntVec& result);
        
        // Get bodyids of all superpixels in the plane
        void getsuperpixelbodiesinplane(uint32 plane, IntVec& result);
        
        // Get all superpixels in the given segment
        void getsuperpixelsinsegment(uint32 segid, IntVec& result);
        
        // Set the superpixels for the given segment
        void setsuperpixels(uint32 segid, uint32 plane, const IntVec& spids);
        
        // Get all superpixels in the given body
        void getsuperpixelsinbody(uint32 bodyid, IntVec& planes, IntVec& spids);

        // Get all superpixels in the given body which are in the given plane
        void getsuperpixelsinbodyinplane(uint32 bodyid, uint32 plane, IntVec& spids);
        
        // Create a new empty segment and return the segid
        uint32 createsegment();
        
        // Get the body id for the given segment
        uint32 getsegmentbodyid(uint32 segid);
        
        // Get all segments in the given body
        void getsegments(uint32 bodyid, IntVec& result);
        
        // Set all segment for a given body
        void setsegments(uint32 bodyid, IntVec& segments);
        
        // Create a new empty body, return the bodyid
        uint32 createbody();        
        
        // Return True if body exists
        bool hasbody(uint32 bodyid);
               
        // Add the segments to the given body
        void addsegments(const IntVec& segids, uint32 bodyid);

        // Delete segment completely
        void deletesegment(uint32 segid);

        // Get count of all bodies (including zerobody)
        uint32 getnumbodies();
        
        // Get count of all bodies, not including the zero body
        uint32 getnumbodiesnonzero();
        
        // Get all bodies (including zerobody)
        void getallbodies(IntVec& result);

        // Get count of all segments
        uint32 getnumsegments();
        
        // Get all segments
        void getallsegments(IntVec& result);
        
        // Get min/max plane extents of the given body
        void getplanelimits(uint32 bodyid, uint32& zmin, uint32& zmax);
        
        // Get min/max plane extents of all bodies
        void getallplanelimits(IntVec& bodyids, IntVec& zmin, IntVec& zmax);
        
        // Return the volume of one body
        uint64 getbodyvolume(uint32 bodyid);
        
        // Get XZ points for the bounding boxes of one body
        void getBodyGeometryXZ(uint32 bodyid, IntVec& x, IntVec& z);

        // Get ZY points for the bounding boxes of one body
        void getBodyGeometryYZ(uint32 bodyid, IntVec& y, IntVec& z);
        
        // Get bounding boxes for one body
        void getBodyBounds(uint32 bodyid, IntVec& z, BoundsVec& bounds);
        
        // Get VTK specific verts and triangles
        void getBodyBoundsVTK(uint32 bodyid, float zaspect, Float3Vec& verts, 
            IntVec& tris);
    
        // Export the given body as a binary STL file
        void exportstl(uint32 bodyid, const char* path, float zaspect);

    private:
        // Read one TXT file into a table directly
        Table* readtxt(std::string root, std::string fname, int columns);
        
        // Get a single superpixel table, throw if invalid plane
        Table* getSuperpixelTable(uint32 plane);
        
        // Remove superpixel from segment list
        void removesuperpixel(uint32 plane, uint32 spid);
        
        // Remove segment from body list
        void removesegment(uint32 segid);
        
        // Add a list of ids to a 1 column table
        // Returns the index where the list starts
        uint32 appendList(Table* table, const IntVec& ids);
        
        // Get the content of a list
        void getList(Table* table, uint32 index, IntVec& result);
        
        // Error checking
        void checkPlane(uint32 plane);
        void checkSuperpixel(uint32 plane, uint32 spid);
        void checkSegment(uint32 segid);
        void checkBody(uint32 bodyid);
        
        // We garbage collect before save
        void garbageCollect();
        
        Table* getSuperpixelTableAndCheckRow(uint32 plane, uint32 spid);
        
        void log(const std::string& format, ...);
        void error(const std::string& format, ...);
        
        void dumptables(Table* bounds, Table* segments, Table* bodies);
        
        uint64 getsegmentvolume(uint32 segid);
        
        // Bodies added during create()
        IntVec m_newbodies;
                        
        // Metadata
        uint32 m_zmin;
        uint32 m_zmax;

        // Per-superpixel data.  One Table for each section.
        //
        // m_superpixel[z] = Table(maxsp+1, NUM_SUPERPIXEL_COLUMNS)
        TableMap m_superpixel;
    
        enum Superpixel
        {
            SUPERPIXEL_X = 0,
            SUPERPIXEL_Y = 1,
            SUPERPIXEL_WIDTH = 2,
            SUPERPIXEL_HEIGHT = 3,
            SUPERPIXEL_VOLUME = 4,
            SUPERPIXEL_SEGID = 5,
            NUM_SUPERPIXEL_COLUMNS = 6
        };
    
        // Per-segment data, directly indexed by segid.
        // non-existant segment will have all EMPTY values.
        // m_segment = Table(maxsegid+1, NUM_SEGMENT_COLUMNS)
        //
        // A segment which doesn't exist at all has EMPTY_VALUE
        // in every column.
        //
        // A segment which exists but is empty has EMPTY_VALUE
        // for SEGMENT_Z, it has a real value for SEGMENT_SPINDEX
        // but the corresponding list in m_segment_sp is empty.
        // It may or may not have EMPTY_VALUE for SEGMENT_BODYID.
        //
        Table* m_segment;
    
        enum Segment
        {
            SEGMENT_Z = 0,
            SEGMENT_BODYID = 1,
            SEGMENT_SPINDEX = 2,
            NUM_SEGMENT_COLUMNS = 3
        };
    
        // Superpixels for each segment. 
        // One-column table (linear array).
        // Indexed by the SEGMENT_SPINDEX value from m_segment.
        // Each "list" of superpixel IDs is terminated by END_OF_LIST.
        Table* m_segment_sp;
        
        // Per-body indexes into the m_body_sp array. 
        // One-column table (linear array).
        // Indexed by bodyid
        Table* m_body_index;
    
        // Segments for each body.
        // One-column table (linear array).
        // Indexed by m_body_index value.  Each "list" of segments 
        // is terminated by END_OF_LIST.
        Table* m_body_seg;
        
        LogFile* m_log;
};
