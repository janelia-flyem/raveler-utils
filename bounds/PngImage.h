#define PNG_DEBUG 3
#include <png.h>


class PngImage
{
public:
    PngImage(const char* filename);
    ~PngImage();

    void write(const char* filename);

    png_byte* getPixel(int x, int y) const
    {
        png_byte* row = m_row_pointers[m_height - y - 1];
        return &(row[x*m_colsize]);
    }

    // Interpret pixel value as an unsigned int, as we do with
    // our superpixel index images.  Handles 16-bit or 32-bit PNGs.
    unsigned int getPixelID(int x, int y) const
    {
        if (m_rgba)
        {
            return *(unsigned int*)getPixel(x, y);
        }
        else
        {
            png_byte* p = getPixel(x, y);
            return (p[0] << 8) + p[1];
        }
    }

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    bool m_rgba;
    int m_colsize;

    int m_width;
    int m_height;

    png_byte m_color_type;
    png_byte m_bit_depth;
    
    png_bytep *m_row_pointers;
};
