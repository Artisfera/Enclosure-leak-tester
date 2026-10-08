#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <float.h>
#include <math.h>

#include "pid_autotune.h"

LOG_MODULE_REGISTER(pid_autotune, LOG_LEVEL_DBG);

static const float output_min = 7.9f;
static const float output_max = 50.0f;
static const float base_band = 2.0f;

static int autotune_fail(pid_autotune *tune, float *duty, int error)
{
    LOG_ERR("Autotune stopped: state=%d, PWM=%.3f%%, elapsed=%lld ms, error=%d",
            tune->state, (double)tune->output,
            (long long)(k_uptime_get() - tune->start_time), error);
    tune->state = AT_FAILED;
    tune->error = error;
    tune->output = 0.0f;
    *duty = 0.0f;
    return error;
}

int pid_autotune_init(pid_autotune *tune, float setpoint)
{
    int64_t now = k_uptime_get();

    *tune = (pid_autotune) {
        .state = AT_FIND_BASE,
        .setpoint = setpoint,
        .output_step = 1.5f,
        .noise_band = 1.0f,
        .output = output_min,
        .start_time = now,
        .state_time = now,
        .window_time = now,
        .cycle_time = -1,
        .control_time = now,
        .previous_pressure = NAN,
        .window_output_min = output_min,
        .window_output_max = output_min,
        .window_min = FLT_MAX,
        .window_max = -FLT_MAX,
    };

    if (!isfinite(setpoint) || setpoint <= 0.0f) {
        return autotune_fail(tune, &tune->output, AT_INPUT_ERR);
    }

    LOG_INF("Autotune start: setpoint=%.2f Pa, PWM range=%.1f..%.1f%%",
            (double)setpoint, (double)output_min, (double)output_max);
    return AT_OK;
}

static int oscillations_regular(pid_autotune *tune, unsigned int count,
                                float amplitude, float period)
{
    for (unsigned int i = 0; i < count; i++) {
        if (fabsf(tune->amplitudes[i] - amplitude) > 0.25f * amplitude ||
            fabsf(tune->periods[i] - period) > 0.35f * period) {
            return 0;
        }
    }
    return 1;
}

static int autotune_finish(pid_autotune *tune, float *duty, pidParm *pidparm,
                           unsigned int count)
{
    float amplitude = 0.0f;
    float period = 0.0f;

    for (unsigned int i = 0; i < count; i++) {
        amplitude += tune->amplitudes[i];
        period += tune->periods[i];
    }
    amplitude /= (float)count;
    period /= (float)count;

    if (amplitude <= 0.1f) {
        return autotune_fail(tune, duty, AT_AMPLITUDE_ERR);
    }

    if (!oscillations_regular(tune, count, amplitude, period)) {
        return autotune_fail(tune, duty, AT_OSCILLATION_ERR);
    }

    // output_step is the relay amplitude in PWM percentage points.
    float Ku = 4.0f * tune->output_step / (3.14159265f * amplitude);
    float Kp = Ku / 2.2f;
    float Ti = 2.2f * period;
    float Td = period / 6.3f;
    float Ki = Kp / Ti;
    float Kd = Kp * Td;

    if (!isfinite(Kp) || !isfinite(Ki) || !isfinite(Kd) ||
        Kp <= 0.0f || Ki <= 0.0f || Kd <= 0.0f) {
        return autotune_fail(tune, duty, AT_OSCILLATION_ERR);
    }

    pidparm->Kp = Kp;
    pidparm->Ki = Ki;
    pidparm->Kd = Kd;
    pidparm->SetPoint = tune->setpoint;
    tune->amplitude = amplitude;
    tune->Ku = Ku;
    tune->Pu = period;
    tune->state = AT_DONE;
    tune->output = tune->output_start;
    *duty = tune->output;

    LOG_INF("Autotune done: a=%.3f Pa, Ku=%.3f, Pu=%.3f s",
            (double)amplitude, (double)Ku, (double)period);
    LOG_INF("PID: Kp=%.4f, Ki=%.4f, Kd=%.4f",
            (double)Kp, (double)Ki, (double)Kd);
    return AT_OK;
}

int pid_autotune_update(pid_autotune *tune, float pressure_pa,
                        float *duty, pidParm *pidparm)
{
    int64_t now = k_uptime_get();

    if (tune->state == AT_FAILED) {
        *duty = 0.0f;
        return tune->error;
    }
    if (tune->state == AT_DONE) {
        *duty = tune->output_start;
        return AT_OK;
    }
    if (!isfinite(pressure_pa)) {
        return autotune_fail(tune, duty, AT_INPUT_ERR);
    }
    if (now - tune->start_time >= 300000) {
        return autotune_fail(tune, duty, AT_TIMEOUT_ERR);
    }

    if (tune->state == AT_FIND_BASE) {
        if (!isfinite(tune->previous_pressure)) {
            tune->previous_pressure = pressure_pa;
        }
        tune->window_min = MIN(tune->window_min, pressure_pa);
        tune->window_max = MAX(tune->window_max, pressure_pa);
        tune->pressure_sum += pressure_pa;
        tune->output_sum += tune->output;
        tune->window_output_min = MIN(tune->window_output_min, tune->output);
        tune->window_output_max = MAX(tune->window_output_max, tune->output);
        tune->window_samples++;

        if (now - tune->start_time >= 60000) {
            LOG_ERR("Cannot establish base pressure within 60 s");
            return autotune_fail(tune, duty, AT_BASE_ERR);
        }

        if (now - tune->control_time >= 100) {
            float dt = (float)(now - tune->control_time) / 1000.0f;
            // Incremental PI for base acquisition: react to rising pressure before overshoot.
            // Limit the integral ramp only; clipping the proportional change biases noisy data.
            float ramp = CLAMP(0.04f * (tune->setpoint - pressure_pa),
                               -4.0f, 0.8f) * dt;
            float damping = CLAMP(-0.10f * (pressure_pa - tune->previous_pressure),
                                  -0.5f, 0.5f);
            float change = ramp + damping;
            float output = CLAMP(tune->output + change,
                                 output_min, output_max);
            if (output > output_min && output < output_max) {
                tune->state_time = now;
            }
            tune->output = output;
            tune->previous_pressure = pressure_pa;
            tune->control_time = now;
        }

        if (now - tune->window_time >= 500) {
            float mean_pressure = tune->pressure_sum / (float)tune->window_samples;
            float mean_output = tune->output_sum / (float)tune->window_samples;
            int settled = tune->window_samples >= 20 &&
                fabsf(mean_pressure - tune->setpoint) <= base_band &&
                tune->window_max - tune->window_min <= 6.0f &&
                tune->window_output_max - tune->window_output_min <= 1.0f;
            if (settled) {
                if (fabsf(mean_output - tune->output_start) > 0.25f) {
                    tune->stable_windows = 0;
                    tune->base_noise = 0.0f;
                }
                tune->stable_windows++;
                tune->output_start = mean_output;
                tune->base_noise = MAX(tune->base_noise,
                    (tune->window_max - tune->window_min) / 2.0f);
            } else {
                tune->stable_windows = 0;
                tune->base_noise = 0.0f;
            }
            LOG_INF("Find base: PWM=%.3f%%, P=%.2f Pa, range=%.2f..%.2f Pa, samples=%u, settled=%d",
                    (double)tune->output, (double)pressure_pa,
                    (double)tune->window_min, (double)tune->window_max,
                    tune->window_samples, settled);

            if (now - tune->state_time >= 3000 &&
                tune->window_max - tune->window_min <= 2.0f * tune->noise_band) {
                if (tune->output >= output_max && mean_pressure < tune->setpoint - base_band) {
                    LOG_ERR("Cannot reach setpoint at maximum PWM");
                    return autotune_fail(tune, duty, AT_BASE_ERR);
                }
                if (tune->output <= output_min && mean_pressure > tune->setpoint + base_band) {
                    LOG_ERR("Pressure exceeds setpoint at minimum running PWM");
                    return autotune_fail(tune, duty, AT_BASE_ERR);
                }
            }
            if (tune->stable_windows >= 2) {
                // Relay thresholds must exceed measured fluctuations at the base.
                tune->noise_band = MAX(1.0f, tune->base_noise + 0.2f);
                // Keep HIGH/LOW symmetric so the Ku formula remains valid.
                tune->output_step = MIN(tune->output_step,
                    MIN(tune->output_start - output_min,
                        output_max - tune->output_start));
                if (tune->output_step < 0.1f) {
                    return autotune_fail(tune, duty, AT_BASE_ERR);
                }
                tune->state = AT_RELAY_HIGH;
                tune->output = tune->output_start + tune->output_step;
                tune->pressure_min = pressure_pa;
                tune->pressure_max = pressure_pa;
                LOG_INF("Autotune base: %.3f%%, step: %.3f%%, noise band: %.2f Pa",
                        (double)tune->output_start, (double)tune->output_step,
                        (double)tune->noise_band);
                tune->state_time = now;
            }

            tune->window_time = now;
            tune->window_min = FLT_MAX;
            tune->window_max = -FLT_MAX;
            tune->window_samples = 0;
            tune->pressure_sum = 0.0f;
            tune->output_sum = 0.0f;
            tune->window_output_min = tune->output;
            tune->window_output_max = tune->output;
        }
    } else {
        if (now - tune->state_time >= 3500) {
            float max_step = MIN(tune->output_start - output_min,
                                 output_max - tune->output_start);
            float next_step = MIN(tune->output_step + 0.5f, max_step);
            if (next_step <= tune->output_step + 0.01f) {
                LOG_ERR("Relay cannot cross pressure threshold within PWM range");
                return autotune_fail(tune, duty, AT_BASE_ERR);
            }
            tune->output_step = next_step;
            tune->output = tune->state == AT_RELAY_HIGH ?
                tune->output_start + tune->output_step :
                tune->output_start - tune->output_step;
            tune->cycles = 0;
            tune->cycle_time = -1;
            tune->pressure_min = pressure_pa;
            tune->pressure_max = pressure_pa;
            tune->samples = 0;
            tune->state_time = now;
            LOG_INF("Relay step increased to %.3f%%", (double)tune->output_step);
        }

        // Include every sample, including overshoot after a relay switch.
        tune->pressure_min = MIN(tune->pressure_min, pressure_pa);
        tune->pressure_max = MAX(tune->pressure_max, pressure_pa);
        tune->samples++;

        if (tune->state == AT_RELAY_HIGH &&
            pressure_pa > tune->setpoint + tune->noise_band) {
            tune->state = AT_RELAY_LOW;
            tune->state_time = now;
            tune->output = tune->output_start - tune->output_step;
        } else if (tune->state == AT_RELAY_LOW &&
                   pressure_pa < tune->setpoint - tune->noise_band) {
            // A full cycle runs from LOW->HIGH to the next LOW->HIGH.
            if (tune->cycle_time >= 0) {
                float amplitude = (tune->pressure_max - tune->pressure_min) / 2.0f;
                float period = (float)(now - tune->cycle_time) / 1000.0f;
                tune->cycles++;

                if (tune->cycles > 2) {
                    if (amplitude <= 0.1f) {
                        return autotune_fail(tune, duty, AT_AMPLITUDE_ERR);
                    }
                    if (period < 0.2f || tune->samples < 20) {
                        return autotune_fail(tune, duty, AT_OSCILLATION_ERR);
                    }
                    unsigned int i = tune->cycles - 3;
                    tune->amplitudes[i] = amplitude;
                    tune->periods[i] = period;
                    LOG_INF("Autotune cycle %u: a=%.3f Pa, Pu=%.3f s",
                            i + 1, (double)amplitude, (double)period);

                    unsigned int measured_cycles = tune->cycles - 2;
                    if (measured_cycles >= 2) {
                        float mean_amplitude = 0.0f;
                        float mean_period = 0.0f;
                        for (unsigned int j = 0; j < measured_cycles; j++) {
                            mean_amplitude += tune->amplitudes[j];
                            mean_period += tune->periods[j];
                        }
                        mean_amplitude /= (float)measured_cycles;
                        mean_period /= (float)measured_cycles;
                        if (measured_cycles == 3 ||
                            oscillations_regular(tune, measured_cycles,
                                                 mean_amplitude, mean_period)) {
                            return autotune_finish(tune, duty, pidparm,
                                                   measured_cycles);
                        }
                    }
                }
            }

            tune->cycle_time = now;
            tune->pressure_min = pressure_pa;
            tune->pressure_max = pressure_pa;
            tune->samples = 0;
            tune->state = AT_RELAY_HIGH;
            tune->state_time = now;
            tune->output = tune->output_start + tune->output_step;
        }
    }

    tune->output = CLAMP(tune->output, output_min, output_max);
    *duty = tune->output;
    return AT_OK;
}
