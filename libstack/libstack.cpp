#include <stdio.h>
#include "HdfStack.h"
#include "util.h"
#include "timers.h"
#include "util.h"
#include "BodyColorTable.h"

//
// libstack.cpp
//
// This is the C interface to HdfStack.
//
// HdfStack is a C++ class.  C++ code can use HdfStack directly 
// and not use the below.  But Python code can use ctypes to call the
// below functions.  Raveler has a class LibStack which uses ctypes
// and has Python versions of the below functions.
//
// NOTE ABOUT CTYPES AND OTHER OPTIONS
// aka "don't do this"
//
// We do a two-step allocation process which leaves allocating
// completely on the Python code.  Every call that needs to return
// an array is split in two: queue and then get.
//
// There are supposedly ways to setup ctypes/numpy such that we could do
// allocation from C, and then pass back that buffer such that
// Python would own it and free it.  This would be much simpler/cleaner
// once working, but it looked complex to initially setup.
//
// Another option that might be cleaner is Cython instead of cytypes.
// Ctypes has a fair amount of boilerplate on the Python side and
// the C side and my impression is Cython would be cleaner.
//
// Boost.Python and pypluslus is another option, which Christopher is
// using for wrapping the V3D plugin API.
//
// The below works fine but it's ugly with all the queue/get pairs.
//

extern "C"
{
    
//
// Functions
//
// The const char* return values work the same:
// 1) return NULL on success
// 2) return an error string on failure
//

// Call at library setup time, return NULL if no errors
const char* init();

// Load the HDF-STACK from disk
const char* load(const char* path);

// Save the HDF-STACK to disk
const char* save(const char* path, uint isbackup);

// Create from old session format
const char* create(
    uint32 bounds_rows, uint32* bounds_data,
    uint32 segment_rows, uint32* segment_data,
    uint32 bodies_rows, uint32* bodies_data);

// Get the count of bodies for getnewbodies
const char* queuenewbodies(uint32* count);

// Get all bodies using count from queunewbodies().
const char* getnewbodies(uint32 count, uint32* data);
    
// Get the lowest numbered plane
const char* getzmin(uint32* retval);

// Get the highest numbered plane
const char* getzmax(uint32* retval);

// Get count of all the superpixels on a given plane
const char* getnumsuperpixelsinplane(uint32 plane, uint32* retval);

// Get the number of superpixels in the given body
const char* getnumsuperpixelsinbody(uint32 bodyid, uint32 *count);

// Return 1 if superpixel exists
const char* hassuperpixel(uint32 plane, uint32 spid, uint32* retval);

// Create a new superpixel, return the new spid
const char* createsuperpixel(uint32 plane, uint32 *retval);

// Add superpixel to the given segment
const char* addsuperpixel(uint32 plane, uint32 spid, uint32 segid);    

// Get the count of superpixels for getsuperpixelsinplane()
const char* queuesuperpixelsinplane(uint32 plane, uint32 *count);

// Get superpixels on a given plane, using count from queuesuperpixelsinplane
const char* getsuperpixelsinplane(uint32 plane, uint32 count, uint32* data);

// Get the count of bodyids for getsuperpixelbodiesinplane()
const char* queuesuperpixelbodiesinplane(uint32 plane, uint32 *count);

// Get bodies for all superpixels on a given plane, count from queuesuperpixelbodiesinplane
const char* getsuperpixelbodiesinplane(uint32 plane, uint32 count, uint32* data);

// Get the count of superpixels for getsuperpixelsinsegment()
const char* queuesuperpixelsinsegment(uint32 segid, uint32* count);

// Get the superpixels using count from queuesuperpixelsinsegment().
const char* getsuperpixelsinsegment(uint32 segid, uint32 count, uint32* data);

// Get the count of superpixels for getsuperpixelsinbody()
const char* queuesuperpixelsinbody(uint32 bodyid, uint32* count);

// Get the superpixels using count from queuesuperpixelsinbody().
const char* getsuperpixelsinbody(uint32 bodyid, uint32 count,
    uint32* planes, uint32* spids);

// Get the count of superpixels for getsuperpixelsinbodyinplane()
const char* queuesuperpixelsinbodyinplane(uint32 segid, uint32 plane, uint32* count);

// Get the superpixels using count from queuesuperpixelsinbodyinplane().
const char* getsuperpixelsinbodyinplane(uint32 bodyid, uint32 plane, uint32 count, uint32* spids);
    
// Get highest numbered superpixel on a given plane
const char* getmaxsuperpixelid(uint32 plane, uint32 *retval);

// Return (x, y, width, height) bounding area of this superpixel
const char* getbounds(uint32 plane, uint32 spid, uint32* bounds);

// Return exact volume of the superpixel
const char* getvolume(uint32 plane, uint32 spid, uint32* retval);

// Set the bounds and volume of a given superpixel
const char* setboundsandvolume(uint32 plane, uint32 spid, Bounds bounds, 
    uint32 volume);

// Return 1 if segment exists
const char* hassegment(uint32 segid, uint32* retval);

// Get the segment id for the given (plane, spid)
const char* getsegmentid(uint32 plane, uint32 spid, uint32* retval);

// Set the segment id for the given (plane, spid)
const char* setsegmentid(uint32 plane, uint32 spid, uint32 segid);

// Get the plane for the given segment
const char* getplane(uint32 segid, uint32* retval);

// Set the superpixels for a given segment
const char* setsuperpixels(uint32 segid, uint32 plane, uint32 count, uint32* data);

// Create a new empty segment and return the id
const char* createsegment(uint32* retval);

// Get the body id for a given segment
const char* getsegmentbodyid(uint32 segid, uint32* retval);

// Get the count of segments for getsegments()
const char* queuesegments(uint32 bodyid, uint32* count);

// Get the segments using count from queuesegments().
const char* getsegments(uint32 bodyid, uint32 count, uint32* data);

// Set the segments for a given body
const char* setsegments(uint32 bodyid, uint32 count, uint32* segments);

// Create a new empty body and return the id
const char* createbody(uint32 *retval);

// Return 1 if body exists
const char* hasbody(uint32 bodyid, uint32* retval);

// Add the segments to the given body
const char* addsegments(uint32 count, uint32* segments, uint32 bodyid);

// Delete segment completely
const char* deletesegment(uint32 segid);

// Get count of all bodies
const char* getnumbodies(uint32* count);

// Get count of all bodies, not including the zero body
const char* getnumbodiesnonzero(uint32* count);

// Get the count of bodies for getallbodies
const char* queueallbodies(uint32* count);

// Get all bodies using count from queueallbodies().
const char* getallbodies(uint32 count, uint32* data);

// Return count of all segments
const char* getnumsegments(uint32* count);

// Get the count of segments for getallsegments
const char* queueallsegments(uint32* count);

// Get all segments using count from queueallsegments().
const char* getallsegments(uint32 count, uint32* data);

// Get min and max plane extents which the body occupies
const char* getplanelimits(uint32 bodyid, uint32* zmin, uint32* zmax);

// Get the count of plane limits for getallplanelimits
const char* queueallplanelimits(uint32* count);

// Return a table of with 3 values in each row (bodyid, zmins, and zmaxs),
// one row for each of count bodies.
const char* getallplanelimits(uint32 count, uint32* table);

// 
// volume API
// 

// Compute the volume of every body in the stack
const char* initbodyvolumes();

// Get the given body's volume
const char* getbodyvolume(uint32 bodyid, uint64* volume);

// Get the count of body/volume pairs for getallbodyvolumes
const char* queueallbodyvolumes(uint32* count);

// Get all bodyids and associated volumes
const char* getallbodyvolumes(uint32 count, uint32* bodyids, uint64* volumes);

// Recalculate the volume of each given body
const char* updatebodyvolumes(uint32 count, uint32* bodyids);

// Queue the "n" largest bodies for getlargestbodies
const char* queuelargestbodies(uint32 n, uint32* count);

// Get the bodies previously queued by queuelargestbodies, note these
// are returned in an arbitrary order right now
const char* getlargestbodies(uint32 count, uint32 *data);

//
// colormap API
//

// Assign a random color to every body in the stack
const char* initbodycolormap();

// Build a single plane's colormap, return the count of entries
const char* queueplanecolormap(uint32 plane, uint32* count);

// Get previously queued colormap
const char* getplanecolormap(uint32 plane, uint32 count, uint32* data);

// Create new random color for this body if it doesn't have one already
const char* addbodycolor(uint32 bodyid);

// Get packed RGBA value for a single body
const char* getbodycolor(uint32 bodyid, uint32* color);

//
// sideview/mapview APIs
//

// Prepare geometry to represent a single body in the XZ side view
const char* queuebodygeometryXZ(uint32 bodyid, uint32* count);

// Get previously queued geometry
const char* getbodygeometryXZ(uint32 bodyid, uint32 count, uint32* data);

// Prepare geometry to represent a single body in the ZY side view
const char* queuebodygeometryZY(uint32 bodyid, uint32* count);

// Get previously queued geometry
const char* getbodygeometryZY(uint32 bodyid, uint32 count, uint32* data);

// Prepare geometry to represent a single body in the YZ side view
const char* queuebodygeometryYZ(uint32 bodyid, uint32* count);

// Get previously queued geometry
const char* getbodygeometryYZ(uint32 bodyid, uint32 count, uint32* data);

// Prepare bounding boxes for a single body
const char* queuebodybounds(uint32 bodyid, uint32* count);

// Get previously queued bounding boxes
const char* getbodybounds(uint32 bodyid, uint32 count, uint32* data);

// Prepare vtk bounding boxes for a single body
const char* queuebodyboundsvtk(uint32 bodyid, float zaspect, uint32* countverts, uint32* counttris);

// Get previously queued vtk bounding boxes
const char* getbodyboundsvtk(uint32 bodyid, uint32 countverts, uint32 counttris, float* verts, int64* tris);

// Export a single body as a binary STL file
const char* exportstl(uint32 bodyid, const char* path, float zaspect);

} // extern 'C'

// Single global stack.  This C api is not thread-safe and
// doesn't support having multiple stacks open.
HdfStack* g_stack = NULL;

// Get safely
HdfStack* getStack()
{
    if (!g_stack)
    {
        throw std::string("No stack loaded");
    }

    return g_stack;
}

// TRY_CATCH
//
// All our C API functions return "const char *".  We return NULL on
// success, or we return a string describing the error on an error.
//
// So this macro does 3 things:
// 1) runs the given code
// 2) returns NULL on success
// 3) returns error string on failure
//
// Note: using the HDF5-DIAG stuff it would be possible to get the
// entire HDF5 "error stack" and return it in some form. But we just
// return a single short error string right now.
//
#define TRY_CATCH(code_) \
    try \
    { \
        code_ \
        return NULL; \
    } \
    catch (std::string& error) \
    { \
        return error.c_str(); \
    } \
    catch (std::exception& e) \
    { \
        return e.what(); \
    }

//
// To allow memory allocation to stay on the Python side any API
// which needs to return an array is broken up into two calls:
//    1) queue
//    2) get
// The queue does the real work, it queues up all the values and
// returns a count of values.  The Python side then allocates 
// space for those values.  Then the get is called with the 
// allocated space, and we copy in the queued values.
//
template <class T>
class ValueQueue
{
public:
    ValueQueue();
    
    // Call this to start queuing, add values to the returned
    // vector.  Key is optional way to detect if queue/get
    // calls are not paired properly.
    std::vector<T>& start(uint32 key = 0);
    
    // Get reference to our values
    std::vector<T>& values() { return m_values; }

    // How many values are queued
    uint32 size() const { return m_values.size(); }
        
    // Get data that was previously queued
    // Key and count are checked against previous queue call.
    // Data is cleared to free up memory, can only get() once.
    void get(uint32 count, T* data, uint32 key = 0);
    
    // Add one value
    void push_back(T value) { m_values.push_back(value); }
    
    // Clear everything
    void clear() { m_values.clear(); }

private:
    std::vector<T> m_values;
    uint32 m_key;
};

template <class T>
ValueQueue<T>::ValueQueue() :
    m_key(0)
{}

template <class T>
std::vector<T>& ValueQueue<T>::start(uint32 key)
{
    // Should always be empty from last get, but just to be sure
    m_values.clear();
    
    // Optional key
    m_key = key;
    
    return m_values;
}

template <class T>
void ValueQueue<T>::get(uint32 count, T* data, uint32 key)
{
    if (m_key != key)
    {
        throw std::string("wrong key");
    }

    if (m_values.size() != count)
    {
        throw std::string("wrong count");
    }

    for (uint32 i = 0; i < m_values.size(); ++i)
    {
        data[i] = m_values[i];
    }
    
    m_values.clear();
    m_key = 0;
}


typedef ValueQueue<uint32> IntQueue;
typedef ValueQueue<uint64> Int64Queue;
typedef ValueQueue<Bounds> BoundsQueue;
typedef ValueQueue<float3> Float3Queue;

// Global queue used for multiple operations
static IntQueue g_intqueue;

// 
// Like ValueQueue but specifically for getsuperpixelsinbody
// which needs to return two arrays.
//
class SuperpixelGetter
{
public:
    // Returns a count of items queued
    uint32 queue(uint32 bodyid);
    
    // Get data that was previously queued
    void get(uint32 bodyid, uint32 count, uint32* planes, uint32* spids);

private:
    IntQueue m_planes;
    IntQueue m_spids;
};

uint32 SuperpixelGetter::queue(uint32 bodyid)
{
    IntVec& planes = m_planes.start(bodyid);
    IntVec& spids = m_spids.start(bodyid);
    getStack()->getsuperpixelsinbody(bodyid, planes, spids);
    assert(planes.size() == spids.size());
    return planes.size();
}

void SuperpixelGetter::get(uint32 bodyid, uint32 count,
    uint32* planes, uint32* spids)
{
    m_planes.get(count, planes, bodyid);
    m_spids.get(count, spids, bodyid);    
}

// Global queue 
static SuperpixelGetter g_spgetter;

//
// Magic macro for compile-time asserts from linux kernel 
// as described here:
//     http://www.pixelbeat.org/programming/gcc/static_assert.html
// for example:
//     ct_assert(sizeof(size_t) == 8);
// must be placed in a function.
//
#define ct_assert(e) ((void)sizeof(char[1 - 2*!(e)]))

const char* init()
{
    // ct_assert must be in a function
    ct_assert(sizeof(uint32) == 4);
    ct_assert(sizeof(uint64) == 8);
    
    #ifndef __APPLE__
    ct_assert(sizeof(size_t) == 8);
    #endif
    
    // Nothing to init right now... no errors possible    
    return NULL;
}

const char* load(const char* path)
{
    TRY_CATCH(
        delete g_stack;
        g_stack = new HdfStack();
        getStack()->load(path);
    )
}

const char* create( 
    uint32 bounds_rows, uint32* bounds_data,
    uint32 segment_rows, uint32* segment_data,
    uint32 bodies_rows, uint32* bodies_data)
{
    // The Tables create copies of the incoming data, we could re-use
    // the memory but Table normally wants to control its own memory,
    // so we just make the copy to keep it simple.
    Table bounds(bounds_rows, 7, bounds_data);
    Table segments(segment_rows, 3, segment_data);
    Table bodies(bodies_rows, 2, bodies_data);    
    
    TRY_CATCH(
        delete g_stack;
        g_stack = new HdfStack();
        g_stack->create(&bounds, &segments, &bodies);
    )
}

const char* save(const char* path, uint isbackup)
{
    TRY_CATCH(
        getStack()->save(path, isbackup);
    )
}

const char* close()
{
    TRY_CATCH(
        delete g_stack;
        g_stack = NULL;
    )
}

const char* getzmin(uint32* retval)
{
    TRY_CATCH(
        *retval = getStack()->getzmin();
    )
}

const char* getzmax(uint32* retval)
{
    TRY_CATCH(
        *retval = getStack()->getzmax();
    )
}

const char* getplane(uint32 segid, uint32* retval)
{
    TRY_CATCH(
        *retval = getStack()->getplane(segid);
    )
}

const char* getnumsuperpixelsinplane(uint32 plane, uint32* retval)
{
    TRY_CATCH(
        *retval = getStack()->getnumsuperpixelsinplane(plane);
    )    
}

const char* getnumsuperpixelsinbody(uint32 bodyid, uint32 *count)
{
    TRY_CATCH(
        *count = getStack()->getnumsuperpixelsinbody(bodyid);
    )
}

const char* hassuperpixel(uint32 plane, uint32 spid, uint32* retval)
{
    TRY_CATCH(
        *retval = getStack()->hassuperpixel(plane, spid);
    )
}

const char* createsuperpixel(uint32 plane, uint32 *retval)
{
    TRY_CATCH(
        *retval = getStack()->createsuperpixel(plane);
    )
}

const char* addsuperpixel(uint32 plane, uint32 spid, uint32 segid)
{
    TRY_CATCH(
        getStack()->addsuperpixel(plane, spid, segid);
    )    
}

const char* queuesuperpixelsinplane(uint32 plane, uint32 *count)
{
    TRY_CATCH(
        IntVec& spids = g_intqueue.start(plane);
        getStack()->getsuperpixelsinplane(plane, spids);
        *count = g_intqueue.size();        
    )
}

const char* getsuperpixelsinplane(uint32 plane, uint32 count, uint32* data)
{
    TRY_CATCH(
        g_intqueue.get(count, data, plane);
    )
}

const char* queuesuperpixelbodiesinplane(uint32 plane, uint32 *count)
{
    TRY_CATCH(
        IntVec& bodies = g_intqueue.start(plane);
        getStack()->getsuperpixelbodiesinplane(plane, bodies);
        *count = g_intqueue.size();        
    )
}

const char* getsuperpixelbodiesinplane(uint32 plane, uint32 count, uint32* data)
{
    TRY_CATCH(
        g_intqueue.get(count, data, plane);
    )
}

const char* getmaxsuperpixelid(uint32 plane, uint32 *retval)
{
    TRY_CATCH(
        *retval = getStack()->getmaxsuperpixelid(plane);
    )    
}

const char* getbounds(uint32 plane, uint32 spid, uint32* retval)
{
    TRY_CATCH(
        Bounds bounds = getStack()->getbounds(plane, spid);
        
        retval[0] = bounds.x;
        retval[1] = bounds.y;
        retval[2] = bounds.width;        
        retval[3] = bounds.height;
    )
}

const char* getvolume(uint32 plane, uint32 spid, uint32* retval)
{
    TRY_CATCH(
        *retval = getStack()->getvolume(plane, spid);
    )
}

const char* setboundsandvolume(uint32 plane, uint32 spid, Bounds bounds, 
    uint32 volume)
{
    TRY_CATCH(
        getStack()->setboundsandvolume(plane, spid, bounds, volume);
    )    
}

const char* hassegment(uint32 segid, uint32* retval)
{
    TRY_CATCH(
        *retval = getStack()->hassegment(segid);
    )
}

const char* getsegmentid(uint32 plane, uint32 spid, uint32* retval)
{
    TRY_CATCH(
        *retval = getStack()->getsegmentid(plane, spid);
    )
}

const char* setsegmentid(uint32 plane, uint32 spid, uint32 segid)
{
    TRY_CATCH(
        getStack()->setsegmentid(plane, spid, segid);
    )
}

const char* queuesuperpixelsinsegment(uint32 segid, uint32* count)
{
    TRY_CATCH(
        IntVec& spids = g_intqueue.start(segid);
        getStack()->getsuperpixelsinsegment(segid, spids);
        *count = g_intqueue.size();        
    )
}

const char* getsuperpixelsinsegment(uint32 segid, uint32 count, uint32* data)
{
    TRY_CATCH(
        g_intqueue.get(count, data, segid);
    )
}

const char* queuesuperpixelsinbody(uint32 bodyid, uint32* count)
{
    TRY_CATCH(
        *count = g_spgetter.queue(bodyid);        
    )
}

const char* getsuperpixelsinbody(uint32 bodyid, uint32 count,
    uint32* planes, uint32* spids)
{
    TRY_CATCH(
        g_spgetter.get(bodyid, count, planes, spids);
    )
}

const char* queuesuperpixelsinbodyinplane(uint32 bodyid, uint32 plane, uint32* count)
{
    TRY_CATCH(
        IntVec& spids = g_intqueue.start(bodyid);
        getStack()->getsuperpixelsinbodyinplane(bodyid, plane, spids);
        *count = g_intqueue.size();        
    )
}

const char* getsuperpixelsinbodyinplane(uint32 bodyid, uint32 plane, uint32 count, uint32* spids)
{
    TRY_CATCH(
        g_intqueue.get(count, spids, bodyid);
    )
}

const char* setsuperpixels(uint32 segid, uint32 plane, uint32 count,
    uint32* spids)
{
    IntVec spidvec;

    for (uint32 i = 0; i < count; i++)
    {
        spidvec.push_back(spids[i]);
    }

    TRY_CATCH(
        getStack()->setsuperpixels(segid, plane, spidvec);
    )
}

const char* createsegment(uint32* retval)
{
    TRY_CATCH(
        *retval = getStack()->createsegment();
    )
}

const char* getsegmentbodyid(uint32 segid, uint32* retval)
{
    TRY_CATCH(
        *retval = getStack()->getsegmentbodyid(segid);
    )
}

const char* queuesegments(uint32 bodyid, uint32* count)
{
    TRY_CATCH(
        IntVec& segments = g_intqueue.start(bodyid);
        getStack()->getsegments(bodyid, segments);
        *count = g_intqueue.size();        
    )
}

const char* getsegments(uint32 bodyid, uint32 count, uint32* data)
{
    TRY_CATCH(
        g_intqueue.get(count, data, bodyid);
    )
}

const char* setsegments(uint32 bodyid, uint32 count, uint32* segments)
{
    IntVec segvec;

    for (uint32 i = 0; i < count; i++)
    {
        segvec.push_back(segments[i]);
    }

    TRY_CATCH(
        getStack()->setsegments(bodyid, segvec);
    )
}

const char* createbody(uint32 *retval)
{
    TRY_CATCH(
        *retval = getStack()->createbody();
    )
}

const char* hasbody(uint32 bodyid, uint32* retval)
{
    TRY_CATCH(
        *retval = getStack()->hasbody(bodyid);
    )
}

const char* addsegments(uint32 count, uint32* segments, uint32 bodyid)
{
    IntVec segvec;

    for (uint32 i = 0; i < count; i++)
    {
        segvec.push_back(segments[i]);
    }
    
    TRY_CATCH(
        getStack()->addsegments(segvec, bodyid);
    )
}

const char* deletesegment(uint32 segid)
{
    TRY_CATCH(
        getStack()->deletesegment(segid);
    )
}

const char* getnumbodies(uint32* count)
{
    TRY_CATCH(
        *count = getStack()->getnumbodies();
    )
}

const char* getnumbodiesnonzero(uint32* count)
{
    TRY_CATCH(
        *count = getStack()->getnumbodiesnonzero();
    )
}

// These are just random keys we use to check that "queue" and "get"
// calls are paired.  We don't want someone queuing up one type
// of value then getting something else, they would be getting
// the wrong thing.
static const int KEY_NEW_BODIES = 1001;
static const int KEY_ALL_BODIES = 1002;
static const int KEY_ALL_SEGMENTS = 1003;
static const int KEY_LARGEST_BODIES = 1004;

const char* queuenewbodies(uint32* count)
{
    TRY_CATCH(
        IntVec& bodies = g_intqueue.start(KEY_NEW_BODIES);
        getStack()->getnewbodies(bodies);
        *count = g_intqueue.size();        
    )
}

const char* getnewbodies(uint32 count, uint32* data)
{
    TRY_CATCH(
        g_intqueue.get(count, data, KEY_NEW_BODIES);
    )
}

const char* queueallbodies(uint32* count)
{
    TRY_CATCH(
        IntVec& bodies = g_intqueue.start(KEY_ALL_BODIES);
        getStack()->getallbodies(bodies);
        *count = g_intqueue.size();        
    )
}

const char* getallbodies(uint32 count, uint32* data)
{
    TRY_CATCH(
        g_intqueue.get(count, data, KEY_ALL_BODIES);
    )
}

const char* getnumsegments(uint32* count)
{
    TRY_CATCH(
        *count = getStack()->getnumsegments();
    )
}

const char* queueallsegments(uint32* count)
{
    TRY_CATCH(
        IntVec& segments = g_intqueue.start(KEY_ALL_SEGMENTS);
        getStack()->getallsegments(segments);
        *count = g_intqueue.size();        
    )
}

const char* getallsegments(uint32 count, uint32* data)
{
    TRY_CATCH(
        g_intqueue.get(count, data, KEY_ALL_SEGMENTS);
    )
}

const char* getplanelimits(uint32 bodyid, uint32* zmin, uint32* zmax)
{
    TRY_CATCH(
        getStack()->getplanelimits(bodyid, *zmin, *zmax);
    )    
}

// For queue/get all plane limits
IntQueue g_bodyids;
IntQueue g_zmin;
IntQueue g_zmax;

const char* queueallplanelimits(uint32* count)
{
    TRY_CATCH(
        IntVec& bodies = g_bodyids.start();
        IntVec& zmin = g_zmin.start();
        IntVec& zmax = g_zmax.start();
        
        getStack()->getallplanelimits(bodies, zmin, zmax);
        
        assert(bodies.size() == zmin.size());    
        assert(bodies.size() == zmax.size());    
        
        *count = bodies.size();
    )
}

// Return bodyids, zmins, and zmaxs for all bodies
const char* getallplanelimits(
    uint32 count, uint32* table)
{
    TRY_CATCH(
        IntVec& bodies = g_bodyids.values();
        IntVec& zmin = g_zmin.values();
        IntVec& zmax = g_zmax.values();
        
        for (uint32 i = 0; i < bodies.size(); ++i)
        {
            table[i*3 + 0] = bodies[i];
            table[i*3 + 1] = zmin[i];
            table[i*3 + 2] = zmax[i];
        }
    )
}

static BodyColorTable* g_bodycolors = NULL;

const char* initbodycolormap()
{
    TRY_CATCH(
        delete(g_bodycolors);
        g_bodycolors = new BodyColorTable(getStack());    
    )
}

const char* queueplanecolormap(uint32 plane, uint32* count)
{
    TRY_CATCH(
        IntVec& colors = g_intqueue.start(plane);        
        g_bodycolors->getplanecolormap(plane, colors);
        *count = g_intqueue.size();
    )    
}

const char* getplanecolormap(uint32 plane, uint32 count, uint32* data)
{
    TRY_CATCH(
        g_intqueue.get(count, data, plane);
    )    
}

const char* addbodycolor(uint32 bodyid)
{
    TRY_CATCH(
        g_bodycolors->addbodycolor(bodyid);
    )
}    

const char* getbodycolor(uint32 bodyid, uint32* color)
{
    TRY_CATCH(
        *color = g_bodycolors->getbodycolor(bodyid);    
    )
}

IntQueue g_x;
IntQueue g_y;
IntQueue g_z;

const char* queuebodygeometryXZ(uint32 bodyid, uint32* count)
{
    TRY_CATCH(
    IntVec& x = g_x.start(bodyid);
    IntVec& z = g_z.start(bodyid);
    
    getStack()->getBodyGeometryXZ(bodyid, x, z);

    // Should be the same size
    assert(x.size() == z.size());
    
    *count = x.size();
    )
}

const char* getbodygeometryXZ(uint32 bodyid, uint32 count, uint32* data)
{
    TRY_CATCH(
        IntVec& x = g_x.values();
        IntVec& z = g_z.values();
        
        for (uint32 i = 0; i < count; ++i)
        {
            data[i*2 + 0] = x[i];
            data[i*2 + 1] = z[i];
        }
    )
}

const char* queuebodygeometryZY(uint32 bodyid, uint32* count)
{
    TRY_CATCH(
    IntVec& z = g_z.start(bodyid);
    IntVec& y = g_y.start(bodyid);
    
    getStack()->getBodyGeometryYZ(bodyid, y, z);

    // Should be the same size
    assert(y.size() == z.size());
    
    *count = y.size();
    )
}

const char* getbodygeometryZY(uint32 bodyid, uint32 count, uint32* data)
{
    TRY_CATCH(
        IntVec& y = g_y.values();
        IntVec& z = g_z.values();
        
        for (uint32 i = 0; i < count; ++i)
        {
            data[i*2 + 0] = z[i];
            data[i*2 + 1] = y[i];
        }
    )
}

const char* queuebodygeometryYZ(uint32 bodyid, uint32* count)
{
    TRY_CATCH(
    IntVec& y = g_y.start(bodyid);
    IntVec& z = g_z.start(bodyid);
    
    getStack()->getBodyGeometryYZ(bodyid, y, z);

    // Should be the same size
    assert(y.size() == z.size());
    
    *count = y.size();
    )
}

const char* getbodygeometryYZ(uint32 bodyid, uint32 count, uint32* data)
{
    TRY_CATCH(
        IntVec& y = g_y.values();
        IntVec& z = g_z.values();
        
        for (uint32 i = 0; i < count; ++i)
        {
            data[i*2 + 0] = y[i];
            data[i*2 + 1] = z[i];
        }
    )
}

BoundsQueue g_bounds;

const char* queuebodybounds(uint32 bodyid, uint32* count)
{
    TRY_CATCH(
        IntVec& z = g_intqueue.start(bodyid);
        BoundsVec& bounds = g_bounds.start(bodyid);
        
        getStack()->getBodyBounds(bodyid, z, bounds);
        
        assert(z.size() == bounds.size());
    
        *count = z.size();
    )
}

const char* getbodybounds(uint32 bodyid, uint32 count, uint32* data)
{
    TRY_CATCH(
        IntVec& z = g_intqueue.values();
        BoundsVec& bounds = g_bounds.values();
        
        for (uint32 i = 0; i < count; ++i)
        {
            uint32 j = i * 5;
            Bounds &b = bounds[i];
            data[j + 0] = z[i];
            data[j + 1] = b.x0();
            data[j + 2] = b.y0();
            data[j + 3] = b.x1();
            data[j + 4] = b.y1();
        }
    )
}


Float3Queue g_verts;

const char* queuebodyboundsvtk(uint32 bodyid, float zaspect, uint32* countverts, uint32* counttris)
{
    TRY_CATCH(
        Float3Vec& verts = g_verts.start(bodyid);
        IntVec& tris = g_intqueue.start(bodyid);        
        
        getStack()->getBodyBoundsVTK(bodyid, zaspect, verts, tris);
        
        *countverts = verts.size();
        *counttris = tris.size();
    )    
}

// Get previously queued vtk bounding boxes
const char* getbodyboundsvtk(uint32 bodyid, uint32 countverts, uint32 counttris, 
    float* verts, int64* tris)
{
    TRY_CATCH(
        Float3Vec& vertvec = g_verts.values();
        IntVec& trivec = g_intqueue.values();
        
        for (uint32 i = 0; i < countverts; ++i)
        {
            uint32 j = i * 3;
            float3 &v = vertvec[i];
            verts[j + 0] = v.x;
            verts[j + 1] = v.y;
            verts[j + 2] = v.z;
        }
        
        for (uint32 i = 0; i < counttris; ++i)
        {
            tris[i] = trivec[i];
        }
        
        g_verts.clear();
        g_intqueue.clear();
    )    
}


static Int64Map s_volumes;

const char* initbodyvolumes()
{
    TRY_CATCH(
        s_volumes.clear();
        
        HdfStack* stack = getStack();
        IntVec bodies;
        
        stack->getallbodies(bodies);
        
        // For each body
        for (IntVecIt bodyIt = bodies.begin(); 
            bodyIt != bodies.end(); ++bodyIt)
        {
            uint64 volume = stack->getbodyvolume(*bodyIt);
            s_volumes[*bodyIt] = volume;
        } 
    )
}

const char* getbodyvolume(uint32 bodyid, uint64* volume)
{
    TRY_CATCH(  
        *volume = s_volumes[bodyid];
    )
}

const char* updatebodyvolumes(uint32 count, uint32* bodyids)
{
    TRY_CATCH(
        HdfStack* stack = getStack();
        
        for (uint32 i = 0; i < count; ++i)
        {
            uint32 bodyid = bodyids[i];
            uint64 volume = stack->getbodyvolume(bodyid);
            s_volumes[bodyids[i]] = volume;
        }    
    )
}

// For getnlargest heap
struct BodyVolume
{
    BodyVolume(uint32 bodyid, uint64 volume) :
        m_bodyid(bodyid),
        m_volume(volume)
    {}
    
    bool operator>(const BodyVolume& other) const
    {
       return m_volume > other.m_volume;
    }

    uint32 m_bodyid;
    uint64 m_volume;
};

static void getnlargest(uint32 n, IntVec& result)
{
    // Don't trust the bodies in s_volumes, they might include stale 
    // entries, bodies that have been removed.  Instead get a trusted
    // list of all bodies from the stack, then lookup each volume
    // in s_volumes
    IntVec bodies;
    getStack()->getallbodies(bodies);
    
    // Create a heap of the n biggest bodies
    std::vector<BodyVolume> heap;
    
    for (uint32 i = 0; i < bodies.size(); ++i)
    {
        uint32 bodyid = bodies[i];
        
        if (bodyid == 0)
            continue;
            
        uint64 volume = s_volumes[bodyid];            
            
        if (heap.size() < n)
        {
            // Heap is not even full, so add every body at this point
            heap.push_back(BodyVolume(bodyid, volume));
            push_heap(heap.begin(), heap.end(), std::greater<BodyVolume>());
        }
        else if (volume > heap[0].m_volume)
        {
            // Heap is full but this body is bigger than the smallest one
            // in the heap. So add it and remove the smallest.
            heap.push_back(BodyVolume(bodyid, volume));
            push_heap(heap.begin(), heap.end(), std::greater<BodyVolume>());
            pop_heap(heap.begin(), heap.end(), std::greater<BodyVolume>());
            heap.pop_back();
        }
    }
    
    result.clear();
    
    // Copy just the bodyids out of the heap (in heap order, NOT sorted)
    for (std::vector<BodyVolume>::iterator it = heap.begin();
        it != heap.end(); ++it)
    {
        result.push_back(it->m_bodyid);
    }
}

const char* queuelargestbodies(uint32 n, uint32* count)
{
    TRY_CATCH(
        IntVec& bodies = g_intqueue.start(KEY_LARGEST_BODIES);
        
        getnlargest(n, bodies);
        
        *count = g_intqueue.size();        
    )    
}

const char* getlargestbodies(uint32 count, uint32 *data)
{
    TRY_CATCH(
        g_intqueue.get(count, data, KEY_LARGEST_BODIES);
    )    
}

class BodyVolumeQueue
{
public:
    // Return count of elements queued
    uint32 queue();
    
    // Get previously queued values
    void get(uint32 count, uint32* bodyids, uint64* volumes);

private:

    IntQueue m_bodyids;
    Int64Queue m_volumes;
};

uint32 BodyVolumeQueue::queue()
{
    IntVec& bodies = m_bodyids.start();
    Int64Vec& volumes = m_volumes.start();
    
    HdfStack* stack = getStack();    
    stack->getallbodies(bodies);
    
    // For each body, look up the volume
    for (IntVecIt bodyIt = bodies.begin(); 
        bodyIt != bodies.end(); ++bodyIt)
    {
        volumes.push_back(s_volumes[*bodyIt]);
    }
    
    assert(m_bodyids.size() == m_volumes.size());
    
    return m_bodyids.size();
}

void BodyVolumeQueue::get(uint32 count, uint32* bodyids, uint64* volumes)
{
    m_bodyids.get(count, bodyids);
    m_volumes.get(count, volumes);
}

BodyVolumeQueue g_bodyVolumeQueue;

const char* queueallbodyvolumes(uint32* count)
{
    TRY_CATCH(
        *count = g_bodyVolumeQueue.queue();
    )
}

const char* getallbodyvolumes(uint32 count, uint32* bodyids, uint64* volumes)
{
    TRY_CATCH(
        g_bodyVolumeQueue.get(count, bodyids, volumes);
    )
}

const char* exportstl(uint32 bodyid, const char* path, float zaspect)
{
    TRY_CATCH(
        getStack()->exportstl(bodyid, path, zaspect);
    )
}

