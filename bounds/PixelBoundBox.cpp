#include "PixelBoundBox.h"
#include <algorithm>

PixelBoundBox::PixelBoundBox()
{
    m_x0 = m_x1 = -1;
    m_y0 = m_y1 = -1;
}

PixelBoundBox::PixelBoundBox(int x, int y)
{
    m_x0 = m_x1 = x;
    m_y0 = m_y1 = y;
}

PixelBoundBox::PixelBoundBox(int x0, int y0, int x1, int y1) :
    m_x0(x0),
    m_x1(x1),
    m_y0(y0),
    m_y1(y1)
{

}

void PixelBoundBox::unionPoint(int x, int y)
{
    // Using if's here is slightly faster than
    // a more compact style with min/max.
    if (x < m_x0)
    {
        m_x0 = x;
    }
    else if (x > m_x1)
    {
        m_x1 = x;
    }

    if (y < m_y0)
    {
        m_y0 = y;
    }
    else if (y > m_y1)
    {
        m_y1 = y;
    }
}

void PixelBoundBox::unionBounds(const PixelBoundBox& other)
{
    // Using if's here is slightly faster than
    // a more compact style with min/max.
    if (other.m_x0 < m_x0)
    {
        m_x0 = other.m_x0;
    }
    if (other.m_x1 > m_x1)
    {
        m_x1 = other.m_x1;
    }
    if (other.m_y0 < m_y0)
    {
        m_y0 = other.m_y0;
    }
    if (other.m_y1 > m_y1)
    {
        m_y1 = other.m_y1;
    }
}