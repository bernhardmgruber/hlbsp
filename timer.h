#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

extern float  g_fFPS;           // The current value of the calculated FPS
extern double g_dFrameInterval; // The time passed since the last call of TimerTick()

void InitTimer(); // Must be called during initialization
void TimerTick(); // Must be called at the beginning of every rendered frame to update variables

#endif // TIMER_H_INCLUDED
