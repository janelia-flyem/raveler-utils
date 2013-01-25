//
// util.h 
//

#include <string>
#include <stdarg.h>

#include "common.h"

// Create a std::string using printf style formatting
std::string FormatString(const std::string& format, ...);

// Safe conversion, false if not a valid string
bool StrToInt(const std::string& s, uint32 &result);

// Search for given value
bool contains(IntVec& vec, uint32 value);

// Join two paths together
std::string join(const std::string& p1, const std::string& p2);

// Return true if path exists
bool fileExists(const std::string& path);

// Get a random body color [0..1] values
void createRandomBodyColor(double& r, double &g, double &b);

// Input colors should be in range [0..1]
uint32 pack_rgba(double r, double g, double b, double a);
