#include <zephyr/kernel.h>


#define Kp 200.0f
#define Ki 200.0f
#define Kd 5.0f
#define setPoint 0.15f

static int64_t prev_time;
static float integral = 0.0f;
static float previousError = 0.0f;

int pid_init(void)
{
    prev_time = k_uptime_get();
 
    return 0;
}

int pid_calc(float psi, float *pidTerm)
{
    float error = setPoint - psi;


    float pTerm = Kp * error;

    float derivative = 0.0f;



    float dt = (float)k_uptime_delta(&prev_time) / 1000.0f;
    integral += error * dt;
    float iTerm = Ki * integral;

    derivative = (error - previousError) / dt;
    float dTerm = Kd * derivative;

    previousError = error;

    *pidTerm = pTerm + iTerm + dTerm;

    return 0;
}