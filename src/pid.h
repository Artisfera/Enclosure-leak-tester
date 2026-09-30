#ifndef PID_H_
#define PID_H_

typedef struct {
    float Kp;
    float Ki;
    float Kd;
    float SetPoint;
} pidParm;

extern pidParm enclosure1;

int pid_init(void);
int pid_calc(float psi, float *pidTerm, pidParm pidparm);

#endif