#include "timer.h"
#include <time.h>

#define TPS_UPDATE_INTERVAL 0.2f

CTimer::CTimer()
{
    //if (!QueryPerformanceFrequency(&HPF))
    //    HPF.QuadPart = 0; //set 0 if not suppored

    Tick();
}

void CTimer::Tick()
{
    static double dLastTime = 0.0;

    double dCurrentTime = GetTime();
    dInterval = dCurrentTime - dLastTime;

    dLastTime = dCurrentTime;

    static int nFrameCounter = 0;
    static double dLastTimeFPSUpdate = 0.0;

    nFrameCounter++;

    if (dCurrentTime - dLastTimeFPSUpdate > TPS_UPDATE_INTERVAL)
    {
        fTPS = (float)((double)nFrameCounter / (dCurrentTime - dLastTimeFPSUpdate));

        dLastTimeFPSUpdate = dCurrentTime;
        nFrameCounter = 0;
    }
}

double CTimer::GetTime()
{
    /*if (HPF.QuadPart) //g_HPF.QuadPart is 0, if not supported
    {
        LARGE_INTEGER PerformanceCounter;
        QueryPerformanceCounter(&PerformanceCounter);
        return (double)PerformanceCounter.QuadPart / (double)HPF.QuadPart;
    }
    else*/
        return (double)clock() / (double)CLOCKS_PER_SEC;
}
