#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <sys/stat.h>

#include <iostream>
#include <sstream>

// printf style creation of a std::string
std::string FormatString(const std::string& format, ...)
{
    char buffer[1024];

    va_list arglist;
    va_start(arglist, format);
    int length = vsnprintf(buffer, sizeof(buffer), format.c_str(), arglist);
    va_end(arglist);

    return std::string(buffer, length);
}

bool StrToInt(const std::string& s, uint32 &result)
{
    std::istringstream ss(s);
    ss >> result;
    return (!ss) ? false : true;
}

bool contains(IntVec& vec, uint32 value)
{
    return std::find(vec.begin(), vec.end(), value) != vec.end();    
}

std::string join(const std::string& p1, const std::string& p2)
{
    char sep = '/';
    std::string tmp = p1;

    if (p1.length() == 0)
    {
        return p2;
    }

    if (p1[p1.length() - 1] != sep)
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

bool fileExists(const std::string& path)
{
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}    

static float clamp(float value, float min, float max)
{
    return std::min(std::max(value, min), max);
}

#define TWO_PI (M_PI * 2.0)
#define THIRD_PI (M_PI / 3.0)

// Adapted from hsi2rgb in PmwColor.py because that is the version we
// were using in Python, and we wanted our C++ version to be the same.
static void hsi2rgb(double h, double s, double i, double &r, double &g, double &b)
{
    if (s == 0)
    {
	r = g = b = i;
    }
    else
    {
	while (h < 0)
	    h += TWO_PI;
	while (h >= TWO_PI)
	    h -= TWO_PI;
	h /= THIRD_PI;
	double f = h - floor(h);
	double p = i * (1.0 - s);
	double q = i * (1.0 - s * f);
	double t = i * (1.0 - s * (1.0 - f));

	switch (int(h))
	{
	    case 0: r = i; g = t; b = p; break;
	    case 1: r = q; g = i; b = p; break;
	    case 2: r = p; g = i; b = t; break;
	    case 3: r = p; g = q; b = i; break;
	    case 4: r = t; g = p; b = i; break;
	    case 5: r = i; g = p; b = q; break;
	}
    }
	
    r = clamp(r, 0.0, 1.0);
    g = clamp(g, 0.0, 1.0);
    b = clamp(b, 0.0, 1.0);
}

static double randrange(double min, double max)
{
    double unit = (rand()/((double)RAND_MAX + 1));
    double range = max - min;
    return min + range * unit;
}

void createRandomBodyColor(double &r, double &g, double &b)
{
    // Use narrow HSI ranges to avoid white (the selection color)
    // and avoid undesirable colors.
    double h = randrange(0.0, 6.283);
    double s = randrange(0.3, 1.0); 
    double i = randrange(0.3, 0.8);
    hsi2rgb(h, s, i, r, g, b);
}

uint32 pack_rgba(double r, double g, double b, double a)
{
    // We truncate not round on purpose. Rounding would create unequal
    // size bins and produce a half-color-value shift.
    int ir = int(r * 255);
    int ig = int(g * 255);
    int ib = int(b * 255);
    int ia = int(a * 255);
    return ia << 24 | ib << 16 | ig << 8 | ir;
}
