//
// STLExport.cpp
//

#include "STLExport.h"

void STLMesh::addcube(Bounds& bounds, uint32 plane, float zaspect)
{
    // Each face is CCW as looking at it from the outside:
    //
    // a----d
    // |    |
    // b----c
    //
    // Then divided into two CCW triangles: abc and acd.
    //
    
    float x0 = bounds.x0();
    float x1 = bounds.x1();
    float y0 = bounds.y0();
    float y1 = bounds.y1();
    float z0 = plane * zaspect;
    float z1 = (plane + 1) * zaspect;
    
    // top
    addface(
        STLVector(x0, y0, z0),
        STLVector(x0, y1, z0),
        STLVector(x1, y1, z0),
        STLVector(x1, y0, z0),
        STLVector(0, 0, 1));

    // front
    addface(
        STLVector(x0, y1, z0),
        STLVector(x0, y1, z1),
        STLVector(x1, y1, z1),
        STLVector(x1, y1, z0),
        STLVector(0, 1, 0));

    // left
    addface(
        STLVector(x0, y0, z0),
        STLVector(x0, y0, z1),
        STLVector(x0, y1, z1),
        STLVector(x0, y1, z0),
        STLVector(-1, 0, 0));

    // right
    addface(
        STLVector(x1, y1, z0),
        STLVector(x1, y1, z1),
        STLVector(x1, y0, z1),
        STLVector(x1, y0, z0),
        STLVector(1, 0, 0));

    // back
    addface(
        STLVector(x1, y0, z0),
        STLVector(x1, y0, z1),
        STLVector(x0, y0, z1),
        STLVector(x0, y0, z0),
        STLVector(0, -1, 0));

    // bottom
    addface(
        STLVector(x0, y0, z1),
        STLVector(x1, y0, z1),
        STLVector(x1, y1, z1),
        STLVector(x0, y1, z1),
        STLVector(0, 0, -1));
}

// binary STL format
// from http://en.wikipedia.org/wiki/STL_(file_format)
//
// UINT8[80] – Header
// UINT32 – Number of triangles
//
// foreach triangle
// REAL32[3] – Normal vector
// REAL32[3] – Vertex 1
// REAL32[3] – Vertex 2
// REAL32[3] – Vertex 3
// UINT16 – Attribute byte count
// end
//
void STLMesh::write(FILE* outf)
{
    uint32 numtriangles = m_tris.size();
    
    // Write 80 byte header, can be anything except
    // should not start with the world "solid" because
    // that indicates an ascii STL file.
    char header[80];
    sprintf(header, "Binary STL by Raveler");
    fwrite(header, 1, sizeof(header), outf);
    
    // Write number of triangles
    fwrite(&numtriangles, sizeof(numtriangles), 1, outf);
    printf("Numtriangles = %d\n", numtriangles);
    
    for (uint32 i = 0; i < m_tris.size(); ++i)
    {
        STLVector& n = m_normals[i];
        STLTriangle& t = m_tris[i];
      
        n.write(outf);
        t.write(outf);
        
        // write 16-bit attribute (always zero)
        unsigned short attribute = 0;
        assert(sizeof(unsigned short) == 2);
        fwrite(&attribute, sizeof(unsigned short), 1, outf);
    }
}

