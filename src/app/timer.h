#ifndef TIMER_H
#define TIMER_H

typedef struct Timer_struct
{
    double currentTime, previousTime;
    float  frameTime;
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
        //printf("\033[34;1mFrame time: \033[0m\033[36m%lf ms\033[0m \n", timer->frameTime);
        timer->frameCount = 0;
        timer->previousTime = currentTime;
    }
}

#endif