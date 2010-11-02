#include "timer.h"

#include <windows.h>
#define FPS_UPDATE_INTERVAL 0.2f

LARGE_INTEGER g_HPF;
float  g_fFPS = 0;
double g_dFrameInterval = 0;

void InitTimer()
{
    if (!QueryPerformanceFrequency(&g_HPF))
        g_HPF.QuadPart = 0; //set 0 if not suppored

    TimerTick();
}

void TimerTick()
{
    static int nFrameCounter = 0;
    static double dLastTimeFPSUpdate = 0.0;
    static double dLastTime = 0.0;

    double dCurrentTime;

    if (g_HPF.QuadPart) //g_HPF.QuadPart == 0, if not supported
    {
        LARGE_INTEGER PerformanceCounter;
        QueryPerformanceCounter(&PerformanceCounter);
        dCurrentTime = (double)PerformanceCounter.QuadPart / (double)g_HPF.QuadPart;
    }
    else
        dCurrentTime = (double)timeGetTime() * 0.001;


    g_dFrameInterval = dCurrentTime - dLastTime;
    dLastTime = dCurrentTime;

    nFrameCounter++;

    if (dCurrentTime - dLastTimeFPSUpdate > FPS_UPDATE_INTERVAL)
    {
        g_fFPS = (float)((double)nFrameCounter / (dCurrentTime - dLastTimeFPSUpdate));

        dLastTimeFPSUpdate = dCurrentTime;
        nFrameCounter = 0;
    }
}
