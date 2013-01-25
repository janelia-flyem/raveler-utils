#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>

#include <string>
#include <list>
#include <ext/hash_map>
#include <iostream>
#include <fstream>

#include "PngImage.h"
#include "PixelBoundBox.h"
#include "Stack.h"

namespace ext = __gnu_cxx;

class BoundsCreator
{
public:
    BoundsCreator(std::string root, int tilesize);

    typedef ext::hash_map<unsigned int, PixelBoundBox> BoundsMap;
    typedef ext::hash_map<unsigned int, int> VolumeMap;

    void create();

    void processPlane(int z);
    void processTile(int z, int i, int j,
        BoundsMap& bounds,
        VolumeMap& volumes);

private:    
    bool isTileEmpty(const PngImage& image);

    Stack m_stack;
    int m_tilesize;

    FILE* m_outf;
};

BoundsCreator::BoundsCreator(std::string root, int tilesize) :
    m_stack(root, tilesize),
    m_tilesize(tilesize),
    m_outf(NULL)
{
}

void BoundsCreator::create()
{
    std::string outpath = m_stack.getSuperpixelBoundsPath();

    // Check if it already exists
    struct stat buf;
    if (stat(outpath.c_str(), &buf) == 0)
    {
        fprintf(stderr, "ERROR: superpixel_bounds.txt already exists\n");
        fprintf(stderr, "ERROR: delete first to recreate.\n");
        exit(1);
    }

    m_outf = fopen(outpath.c_str(), "w");
    
    if (!m_outf)
    {
        fprintf(stderr, "Cannot open output file: %s\n", outpath.c_str());
        return;
    }

    // Write the header
    fprintf(m_outf, "# superpixel bounding boxes and volumes\n");
    fprintf(m_outf, "# plane\tsp\tx y width height volume\n\n");
    
    printf("Rows=%d\n", m_stack.getNumRows(0));
    printf("Cols=%d\n", m_stack.getNumCols(0));

    int zmin = m_stack.getMetadataValue("zmin");
    int zmax = m_stack.getMetadataValue("zmax");

    printf("zmin=%d\n", zmin);
    printf("zmax=%d\n", zmax);

    for (int z = zmin; z < zmax + 1; ++z)
    {
        processPlane(z);
    }

    fclose(m_outf);
}

void BoundsCreator::processPlane(int z)
{
    BoundsMap bounds;
    VolumeMap volumes;

    for (int i = 0; i < m_stack.getNumCols(0); ++i)
    {
        for (int j = 0; j < m_stack.getNumRows(0); ++j)
        {
            processTile(z, i, j, bounds, volumes);
        }
    }

    typedef std::list<unsigned int> SpidList;
    SpidList spids;
    
    for (BoundsMap::iterator it = bounds.begin(); it != bounds.end(); ++it)
    {
        spids.push_back((*it).first);
    }

    spids.sort();

    for (SpidList::iterator it = spids.begin(); it != spids.end(); ++it)
    {
        unsigned int spid = *it;
        const PixelBoundBox& box = bounds[spid];    
        int trueArea = volumes[spid];
        int width = box.getWidth();
        int height = box.getHeight();
        
        fprintf(m_outf, "%d\t%d\t%d %d %d %d %d\n",
            z, spid, box.getX(), box.getY(), 
            width, height, trueArea);

        // Sanity check that the exact area/volume is never
        // greater than the bound box area.
        int rectArea = width * height;
        assert(trueArea <= rectArea);
    }
    
    printf("z=%d superpixels=%zu\n", z, bounds.size());
}

void BoundsCreator::processTile(int z, int i, int j,
    BoundsMap& bounds,
    VolumeMap& volumes)
{
    std::string path = m_stack.getTilePath(0, j, i, 's', z);
    printf("%s\n", path.c_str());

    PngImage image(path.c_str());    

    int width = image.getWidth();
    int height = image.getHeight();

    //printf("width=%d height=%d\n", image.getWidth(), image.getHeight());

    // Note that i, j always represent full tiles, because only the
    // final edge/corner tile is ever partial
    int baseX = m_tilesize * i;
    int baseY = m_tilesize * j;

    // As a special case for speed if tile is completely empty we can
    // union in the bounds in a single step.
    if (isTileEmpty(image))
    {
        int edgeX = baseX + width - 1;
        int edgeY = baseY + height - 1;
    
        PixelBoundBox tileBounds(baseX, baseY, edgeX, edgeY);
    
        if (bounds.find(0) == bounds.end())
        {        
            bounds[0] = tileBounds;
            volumes[0] = width * height;
        }
        else
        {
            bounds[0].unionBounds(tileBounds);
            volumes[0] += (width * height);
        }    
    }
    else
    {
        // The tile is not empty, union every pixel separately
        for (int tileX = 0; tileX < width; ++tileX)
        {
            for (int tileY = 0; tileY < height; ++tileY)
            {
                int x = baseX + tileX;
                int y = baseY + tileY;
                unsigned int spid = image.getPixelID(tileX, tileY);
            
                if (bounds.find(spid) == bounds.end())
                {
                    bounds[spid] = PixelBoundBox(x, y);
                    volumes[spid] = 1;
                }
                else
                {   
                    bounds[spid].unionPoint(x, y);
                    volumes[spid] += 1;
                }
            }
        }
    }
}

bool BoundsCreator::isTileEmpty(const PngImage& image)
{
    for (int x = 0; x < image.getWidth(); ++x)
    {
        for (int y = 0; y < image.getHeight(); ++y)
        {
            unsigned int spid = image.getPixelID(x, y);
        
            if (spid != 0)
            {
                return false;
            }
        }
    }

    return true;
}

int main(int argc, char **argv)
{
    std::string root;
    int tilesize = 1024;
    
    if (argc == 2)
    {    
        root = argv[1];
    }
    else if (argc == 3)
    {
        root = argv[1];
        tilesize = atoi(argv[2]);    
    }
    else
    {
        printf("Usage: %s <stack_path> [<tilesize>=1024]\n", argv[0]);
        exit(1);
    }

    BoundsCreator creator(root, tilesize);
    creator.create();

    return 0;
}
