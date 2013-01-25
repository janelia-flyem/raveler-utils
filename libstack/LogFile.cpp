#include "LogFile.h"
#include "util.h"

LogFile::LogFile(std::string root, std::string filename, std::string header) :
    m_filename(filename),
    m_header(header),
    m_file(NULL)
{ 
    m_path = join(root, filename);
}

LogFile::~LogFile()
{
    if (m_file)
    {
        fclose(m_file);
        m_file = NULL;
    }
}

void LogFile::log(const std::string& format, ...)
{
    // Open file only on first write to avoid empty log files
    if (!m_file)
    {
        m_file = fopen(m_path.c_str(), "w"); 
        if (m_file && !m_header.empty())
        {
            fprintf(m_file, "%s\n", m_header.c_str());
        }
    }
    
    if (m_file)
    {
        char buffer[1024];

        va_list arglist;
        va_start(arglist, format);
        vsnprintf(buffer, sizeof(buffer), format.c_str(), arglist);
        va_end(arglist);

        fprintf(m_file, buffer);
        fprintf(m_file, "\n");
        fflush(m_file);
    }
}
