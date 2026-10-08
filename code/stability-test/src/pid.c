#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "pid.h"

static int64_t prev_time;
static float integral = 0.0f;
static float previousError = 0.0f;


int pid_init(void)
{
    prev_time = k_uptime_get();

    return 0;
}


int pid_calc(float pressure_pa, float *pidTerm, pidParm pidparm)
{
    float error = pidparm.SetPoint - pressure_pa;

    float pTerm = pidparm.Kp * error;

    float derivative = 0.0f;



    float dt = (float)k_uptime_delta(&prev_time) / 1000.0f;
    if (dt <= 0.0f) {
     dt = 0.001f;
    }

    integral += error * dt;

    //integral = CLAMP(integral, -0.25f, 0.25f);

    float iTerm = pidparm.Ki * integral;

    derivative = (error - previousError) / dt;
    float dTerm = pidparm.Kd * derivative;

    previousError = error;

    *pidTerm = pTerm + iTerm + dTerm;

    printf("pTerm:%.4f,iTerm:%.4f,dTerm:%.4f,SetPoint:%.4f,pidTerm:%.4f\r\n",
        pTerm,
        iTerm,
        dTerm,
        pidparm.SetPoint,
        pidTerm);

    return 0;
}