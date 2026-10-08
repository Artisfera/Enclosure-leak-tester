#ifndef PWM_H_
#define PWM_H_

typedef enum {
    PWM_OK, 
    PWM_SET_ERR,
} pwm_status;

int pwm_set_percent(float duty);

#endif