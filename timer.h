#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#include <windows.h>

class CTimer
{
public:
    CTimer();
    void Tick();
    double GetTime();

    float fTPS;      // Ticks per second
    double dInterval; // The time passed since the last call of Tick()

private:
    LARGE_INTEGER HPF;
};

#endif // TIMER_H_INCLUDED
