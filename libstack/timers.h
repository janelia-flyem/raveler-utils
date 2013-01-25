// timers.h

#pragma once

#include "common.h"

//
// Get time-in-seconds since program started
//
double getTimeInSeconds();

//
// PrintingBlockTimer times a block of code, and print out the duration
// when it falls out of scope.
//
// For example:
//
// {
//     PBT pbt("compute stuff");
//     computeStuff();
// }
//
class PrintingBlockTimer
{
public:
    PrintingBlockTimer(const std::string& format, ...); 
    ~PrintingBlockTimer();

private:
    std::string m_label;
    double m_start;        
};

//
// PerfTimer counts how many times stop() is called and accumulates
// time spent between start() and stop().  Call report() to print
// total and average durations.
//
// For example:
//
// {
//     PerfCounter foo("doWork");
//
//     for (int i = 0; i < 1000; ++i)
//     {
//         foo.start();
//         doWork();
//         foo.stop();
//     }
//
//     foo.report();
//
class PerfTimer
{
public:
    PerfTimer(const char* label);
    
    void start();
    void stop();
    void report();

private:
    const char* m_label;
    double m_start;
    double m_total;
    uint32 m_count;
};


typedef PrintingBlockTimer PBT;
