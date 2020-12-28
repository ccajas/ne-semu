#ifndef TIMER_H
#define TIMER_H

typedef struct Timer_struct
{
    double   currentTime, previousTime;
    double   residualTime;
    float    frameTime;
    uint64_t frameCount;
}
Timer;

inline void update_timer (Timer * timer, double currentTime)
{
    timer->frameCount++;

    /* If a second has passed */
    if (currentTime - timer->previousTime >= 1.0)
    {
        timer->frameTime = (float) 1000.0 / timer->frameCount;
        timer->frameCount = 0;
        timer->previousTime = currentTime;
    }
}

#endif