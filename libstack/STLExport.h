//
// STLExport.h
//

#include "common.h"

struct STLVector
{
    STLVector(float x_, float y_, float z_) :
        x(x_),
        y(y_),
        z(z_)
    {}

    void write(FILE* outf)
    {
        fwrite(&x, sizeof(float), 1, outf);
        fwrite(&y, sizeof(float), 1, outf);
        fwrite(&z, sizeof(float), 1, outf);
    }    

    float x;
    float y;
    float z;    
};

struct STLTriangle
{
    STLTriangle(STLVector a_, STLVector b_, STLVector c_) :
        a(a_),
        b(b_),
        c(c_)
    {}
    
    void write(FILE* outf)
    {
        a.write(outf);
        b.write(outf);
        c.write(outf);
    }    

    STLVector a;
    STLVector b;
    STLVector c;
};

typedef std::vector<STLVector> STLNormalList;
typedef std::vector<STLTriangle> STLTriangleList;

class STLMesh
{
public:
    void addcube(Bounds& bounds, uint32 plane, float zaspect);
    
    // write binary stl 
    void write(FILE *outf);
    
private:
    void addface(
        const STLVector& a,
        const STLVector& b,
        const STLVector& c, 
        const STLVector& d,
        const STLVector& n)
    {
        m_normals.push_back(n);
        m_tris.push_back(STLTriangle(a, b, c));
        m_normals.push_back(n);
        m_tris.push_back(STLTriangle(a, c, d));    
    }

    STLNormalList m_normals;
    STLTriangleList m_tris;
};

