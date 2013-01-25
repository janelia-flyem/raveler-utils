#include "common.h"

//
// Simple text log file
//
class LogFile
{
public:
    // File is opened and header is written only when first
    // line is logged, to avoid empty log files.
    LogFile(std::string root, std::string filename, std::string header = "");
    ~LogFile();

    const char* getFilename() { return m_filename.c_str(); }
    
    // No newline required, will be appened
    void log(const std::string& format, ...); 

private:
    std::string m_filename;
    std::string m_path;
    std::string m_header;
    FILE* m_file;    
};
