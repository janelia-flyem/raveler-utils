#include "timers.h"
#include "util.h"

#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

// Print seconds as a friendly unit appropriate number
static std::string formatTime(double seconds)
{
    if (seconds < 1e-6)
        return FormatString("%.0fns", 1e9 * seconds);
    else if (seconds < 1e-3)
        return FormatString("%.1fus", 1e6 * seconds);
    else if (seconds < 1)
        return FormatString("%.1fms", 1000 * seconds);
    else
        return FormatString("%.1fs", seconds);
}


//
// Wrapper for gettimeofday to get time as a double in seconds since start
//
class Timer
{
public:
    Timer();

    // Return time in seconds since init
    double seconds();

private:
    static time_t s_init_sec;

};

// Offset so our time() is zero-based and has reasonable precision
time_t Timer::s_init_sec;

Timer::Timer()
{
    timeval tv;
    gettimeofday(&tv, NULL);

    s_init_sec = tv.tv_sec;
}

double Timer::seconds()
{
    timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec - s_init_sec) + (tv.tv_usec/1000000.0);
}

// Global
Timer g_timer;

double getTimeInSeconds()
{
    return g_timer.seconds();
}

PrintingBlockTimer::PrintingBlockTimer(const std::string& format, ...)
{
    char buffer[1024];

    va_list arglist;
    va_start(arglist, format);
    vsnprintf(buffer, sizeof(buffer), format.c_str(), arglist);
    va_end(arglist);
    
    m_label = buffer;
    m_start = g_timer.seconds();
}

PrintingBlockTimer::~PrintingBlockTimer()
{
    double elapsed = g_timer.seconds() - m_start;
    printf("%s: %s\n", m_label.c_str(), formatTime(elapsed).c_str());
}

PerfTimer::PerfTimer(const char* label) :
    m_label(label),
    m_start(0),
    m_total(0),
    m_count(0)
{
}

void PerfTimer::start()
{
    m_start = g_timer.seconds();    
}

void PerfTimer::stop()
{    
    m_total += (g_timer.seconds() - m_start);
    ++m_count;
}

void PerfTimer::report()
{
    double avg = m_total / m_count;
        
    printf("%s: count=%d total=%s avg=%s\n",
        m_label, m_count, formatTime(m_total).c_str(), formatTime(avg).c_str());
}
    