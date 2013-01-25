#include "Stack.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include <string>
#include <map>
#include <iostream>
#include <fstream>

//
// Is there a stdlib thing for this?
//
std::string join(const std::string& p1, const std::string& p2)
{
   char sep = '/';
   std::string tmp = p1;

    if (p1[p1.length()] != sep)
    {
        // Need to add a path separator
        tmp += sep;
        return (tmp + p2);
    }
    else
    {
        return(p1 + p2);
    }
}

// 
// Metadata
//

Metadata::Metadata(std::string root)
{
    std::string path = join(root, "tiles/metadata.txt");

    char buffer[1024];
    std::ifstream fin(path.c_str());
    
    if (!fin)
    {
        printf("Error opening: %s\n", path.c_str());
        exit(1);
    }

    while (fin.getline(buffer, 1024))
    {
        int len = strlen(buffer);
        if (len > 0 && buffer[0] != '#')
        {
            char *p = buffer;
            char *key = buffer;
            strsep(&p, "=");
            char *value = strsep(&p, "=");
        
            m_values[key] = value;
        }
    }
}

int Metadata::getIntValue(std::string key)
{
    if (m_values.find(key) == m_values.end())
    {
        fprintf(stderr, "ERROR: no metadata value for '%s'\n", key.c_str());
        exit(1);
    }
    else
    {
        return atoi(m_values[key].c_str());
    }
}

// 
// Stack
//

Stack::Stack(const std::string& root, int tilesize) :
    m_root(root),
    m_tilesize(tilesize),
    m_metadata(root)
{
}

int Stack::getMetadataValue(std::string key)
{
    return m_metadata.getIntValue(key);
}

int Stack::getNumRows(int lod)
{
    int height = m_metadata.getIntValue("height");
    return getNumTiles(height, lod);
}

int Stack::getNumCols(int lod)
{
    int width = m_metadata.getIntValue("width");
    return getNumTiles(width, lod);
}

int Stack::getNumTiles(int imagesize, int lod)
{
    int dim = m_tilesize * pow(2, lod);

    // Fractional rows/cols needed
    float fracNum = float(imagesize) / dim;

    // Take the next-highest-integer, since we need an additional
    // tile to cover the fractional part
    return int(ceil(fracNum));
}

std::string Stack::getSuperpixelBoundsPath()
{
    return join(m_root, "superpixel_bounds.txt");
}

std::string Stack::getTilePath(int lod, int row, int col, char channel, int section)
{
    char path[1024];
    
    if (section < 1000)
    {
        snprintf(path, 1024, "%s/tiles/%d/%d/%d/%d/%c/%03d.png",
            m_root.c_str(), m_tilesize, lod, row, col, channel, section);
        return path;
    }
    else
    {
        int subdir = (section / 1000) * 1000;
        snprintf(path, 1024, "%s/tiles/%d/%d/%d/%d/%c/%d/%d.png",
            m_root.c_str(), m_tilesize, lod, row, col, channel, subdir, section);
        return path;
        
        
    }
}
