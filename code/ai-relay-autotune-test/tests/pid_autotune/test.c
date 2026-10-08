#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "pid_autotune.h"

int64_t test_time;

static void test_failures(void)
{
    pid_autotune tune;
    pidParm gains = {.Kp = 123.0f};
    float duty;
    test_time = 0;
    assert(pid_autotune_init(&tune, NAN) == AT_INPUT_ERR);
    assert(tune.output == 0.0f);
    assert(pid_autotune_init(&tune, 50.0f) == AT_OK);
    assert(pid_autotune_update(&tune, NAN, &duty, &gains) == AT_INPUT_ERR);
    assert(duty == 0.0f && gains.Kp == 123.0f);
    pid_autotune_init(&tune, 50.0f);
    for (test_time = 10; test_time <= 61000; test_time += 10) {
        if (pid_autotune_update(&tune, 6.0f, &duty, &gains) != AT_OK) break;
        assert(duty >= 7.9f && duty <= 20.0f);
    }
    assert(tune.state == AT_FAILED && tune.error == AT_BASE_ERR && duty == 0.0f);
}

static void test_pid_reset(void)
{
    pidParm gains = {.Kp = 1, .Ki = 1, .Kd = 1, .SetPoint = 50};
    float term;
    test_time = 0;
    pid_init();
    test_time = 1000;
    pid_calc(40, &term, gains);
    assert(fabsf(term - 20) < .001f);
    pid_init();
    test_time += 100;
    pid_calc(49, &term, gains);
    assert(fabsf(term - 1.1f) < .001f);
}

static void test_relay_extrema(void)
{
    pid_autotune tune;
    pidParm gains = {.Kp = 123};
    float duty;
    test_time = 0;
    pid_autotune_init(&tune, 50);
    tune.state = AT_RELAY_LOW;
    tune.output_start = 12;
    tune.output_step = 1;
    tune.pressure_min = tune.pressure_max = 50;
    // Start a complete cycle at the LOW->HIGH crossing.
    pid_autotune_update(&tune, 48, &duty, &gains);
    for (int cycle = 0; cycle < 5 && tune.state != AT_DONE; cycle++) {
        // Extrema occur AFTER switches at 51/49 Pa.
        float amplitude = cycle < 2 ? 10.0f : 4.0f;
        for (int sample = 1; sample <= 100 && tune.state != AT_DONE; sample++) {
            test_time += 10;
            float pressure = sample <= 25 ? 50 - amplitude :
                             sample <= 50 ? 52 :
                             sample <= 75 ? 50 + amplitude :
                             sample < 100 ? 50 : 48;
            int err = pid_autotune_update(&tune, pressure, &duty, &gains);
            assert(err == AT_OK);
            if (tune.cycles < 4) assert(gains.Kp == 123);
        }
    }
    assert(tune.state == AT_DONE);
    assert(fabsf(tune.amplitude - 4) < .05f);
    assert(fabsf(tune.Ku - 1.0f / 3.14159265f) < .005f);
}

static void test_plant(float gain, float lag, int delay)
{
    pid_autotune tune;
    pidParm gains = {0};
    float history[1000] = {0};
    float duty, pressure = 6.0f;
    test_time = 0;
    pid_autotune_init(&tune, 50);
    duty = tune.output;
    int n;
    for (n = 1; n < 30000 && tune.state != AT_DONE; n++) {
        test_time = n * 10;
        history[n % 1000] = floorf(duty * 6.4f) / 6.4f;
        float delayed = history[(n + 1000 - delay) % 1000];
        float target = 6 + gain * fmaxf(delayed - 7.0f, 0);
        pressure += (target - pressure) * .01f / lag;
        float measured = roundf((pressure + .3f * sinf(n * 2.17f)) / .152f) * .152f;
        int err = pid_autotune_update(&tune, measured, &duty, &gains);
        if (err) break;
        assert(duty >= 7.9f && duty <= 20);
    }
    printf("gain=%.1f lag=%.1f delay=%.2f: state=%d time=%.2f base=%.3f P=%.2f\n",
           gain, lag, delay * .01, tune.state, test_time * .001, tune.output_start, pressure);
    assert(tune.state == AT_DONE);
    assert(gains.Kp > 0 && gains.Ki > 0 && gains.Kd > 0);
    assert(fabsf(gains.Kp - tune.Ku / 2.2f) < .0001f);
    assert(fabsf(gains.Ki - gains.Kp / (2.2f * tune.Pu)) < .0001f);
    assert(fabsf(gains.Kd - gains.Kp * tune.Pu / 6.3f) < .0001f);

    // A large positive disturbance must turn the turbine off and recover.
    pid_init();
    pressure += 40.0f;
    int recovered = 0;
    for (int sample = 1; sample <= 2000; sample++) {
        test_time += 10;
        history[(n + sample) % 1000] = floorf(duty * 6.4f) / 6.4f;
        float delayed = history[(n + sample + 1000 - delay) % 1000];
        float target = 6 + gain * fmaxf(delayed - 7.0f, 0);
        pressure += (target - pressure) * .01f / lag;
        if (sample % 10 == 0) {
            float pid_term;
            pid_calc(pressure, &pid_term, gains);
            duty = tune.output_start + pid_term;
            duty = duty < 7.9f ? 0.0f : fminf(duty, 20.0f);
        }
        if (fabsf(pressure - 50.0f) <= 2.0f) {
            recovered++;
            if (recovered >= 100) break;
        } else {
            recovered = 0;
        }
    }
    assert(recovered >= 100);
}

int main(void)
{
    test_failures();
    test_pid_reset();
    test_relay_extrema();
    test_plant(5, .5f, 10);
    test_plant(10, 1, 30);
    test_plant(30, 2, 100);
    test_plant(10, 2, 200);
    puts("PASS");
}
