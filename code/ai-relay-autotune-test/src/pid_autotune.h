#ifndef PID_AUTOTUNE_H_
#define PID_AUTOTUNE_H_

#include <stdint.h>

#include "pid.h"

typedef enum {
    AT_FIND_BASE,
    AT_RELAY_HIGH,
    AT_RELAY_LOW,
    AT_DONE,
    AT_FAILED,
} pid_autotune_state;

typedef enum {
    AT_OK,
    AT_TIMEOUT_ERR,
    AT_BASE_ERR,
    AT_AMPLITUDE_ERR,
    AT_OSCILLATION_ERR,
    AT_INPUT_ERR,
} pid_autotune_status;

typedef struct {
    pid_autotune_state state;
    int error;
    float setpoint;
    float output_start;
    float output_step;
    float noise_band;
    float base_noise;
    float output;
    float amplitude;
    float Ku;
    float Pu;

    int64_t start_time;
    int64_t state_time;
    int64_t window_time;
    int64_t cycle_time;
    int64_t control_time;
    float previous_pressure;
    float pressure_sum;
    float output_sum;
    float window_output_min;
    float window_output_max;
    unsigned int stable_windows;
    float window_min;
    float window_max;
    float pressure_min;
    float pressure_max;
    float amplitudes[3];
    float periods[3];
    unsigned int cycles;
    unsigned int samples;
    unsigned int window_samples;
} pid_autotune;

int pid_autotune_init(pid_autotune *tune, float setpoint);
int pid_autotune_update(pid_autotune *tune, float pressure_pa,
                        float *duty, pidParm *pidparm);

#endif
