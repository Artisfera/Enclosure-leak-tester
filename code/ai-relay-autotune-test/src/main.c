#include <zephyr/kernel.h>
#include <zephyr/console/console.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <math.h>

#include "adc.h"
#include "pwm.h"
#include "spi.h"
#include "pid.h"
#include "pid_autotune.h"
#include "enclosure_profiles.h"


LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static unsigned int select_profile(void)
{
    enclosure_profile profile;

    while (1) {
        printf("\r\nObudowy zapisane w MCU:\r\n");
        for (unsigned int i = 0; i < ENCLOSURE_PROFILE_COUNT; i++) {
            if (enclosure_profile_load(i, &profile) == 0) {
                printf("  %u. %s\r\n", i + 1, profile.name);
            } else {
                printf("  %u. <puste>\r\n", i + 1);
            }
        }
        printf("Wybierz numer obudowy [1-%u]: ", ENCLOSURE_PROFILE_COUNT);
        char *line = console_getline();
        if (line[0] >= '1' && line[0] <= '0' + ENCLOSURE_PROFILE_COUNT &&
            line[1] == '\0') {
            return (unsigned int)(line[0] - '1');
        }
        printf("Nieprawidlowy numer.\r\n");
    }
}

static int select_yes_no(const char *question)
{
    while (1) {
        printf("%s [y/n]: ", question);
        char *line = console_getline();
        if (line[0] == 'y' || line[0] == 'Y') {
            return 1;
        }
        if (line[0] == 'n' || line[0] == 'N') {
            return 0;
        }
        printf("Wpisz y albo n.\r\n");
    }
}

int main(void)
{
    float pressure_pa = 0.0f;
    float duty = 0.0f;
    float l_min = 0.0f;
    float pidTerm = 0.0f;
    float pressure_sum = 0.0f;
    float avg_pressure = 0.0f;
    unsigned int pressure_samples = 0;
    int64_t telemetry_time = 0;
    int64_t pid_time = 0;
    pid_autotune autotune;
    enclosure_profile profile;
    float output_start = 0.0f;
    unsigned int profile_index;
    int calibrating;
    //printf("Hello World!\n");
    
    pidParm enclosure1 = {
        .Kp = 0.5f,
        .Ki = 0.01f,
        .Kd = 1.0f,
        .SetPoint = 50.0f,
    };
    
    int err;
    err = pwm_set_percent(0.0f);
    if (err != PWM_OK) {
        LOG_ERR("PWM failed with code error: %d", err);
        goto stop_motor;
    }

    err = adc_init();
    if (err != 0){
        LOG_ERR("ADC failed with code error: %d", err);
        goto stop_motor;
    };

    err = spi_init();
    if (err != 0){
        LOG_ERR("SPI failed with code error: %d", err);
        goto stop_motor;
    };

    err = enclosure_profiles_init();
    if (err != 0) {
        LOG_ERR("Profile storage failed with code error: %d", err);
        goto stop_motor;
    }
    console_getline_init();

select_profile:
    profile_index = select_profile();
    calibrating = select_yes_no("Uruchomic kalibracje?");
    if (!calibrating) {
        if (enclosure_profile_load(profile_index, &profile) != 0) {
            printf("Brak zapisanego profilu. Wybierz kalibracje.\r\n");
            goto select_profile;
        }
        enclosure1 = profile.pid;
        output_start = profile.output_start;
        duty = output_start;
        err = pwm_set_percent(duty);
        if (err != PWM_OK) {
            LOG_ERR("PWM failed with code error: %d", err);
            goto stop_motor;
        }
        pid_init();
        pid_time = k_uptime_get();
        printf("Wczytano %s: base=%.6f%%, Kp=%.6f, Ki=%.6f, Kd=%.6f\r\n",
               profile.name, (double)output_start, (double)enclosure1.Kp,
               (double)enclosure1.Ki, (double)enclosure1.Kd);
    } else {
        pid_init();
        err = pid_autotune_init(&autotune, enclosure1.SetPoint);
        if (err != AT_OK) {
            goto stop_motor;
        }
        output_start = autotune.output;
        err = pwm_set_percent(output_start);
        if (err != PWM_OK) {
            LOG_ERR("PWM failed with code error: %d", err);
            goto stop_motor;
        }
    }

    


    while (1) {
        err = adc_read_flowrate(&l_min);
        if (err != ADC_OK) {
            LOG_ERR("ADC failed with code error: %d", err);
            goto stop_motor;
        }

        err = spi_read_pressure(&pressure_pa);
        if (err != SPI_OK || !isfinite(pressure_pa)) {
            LOG_ERR("Pressure read failed with code error: %d", err);
            goto stop_motor;
        }
        pressure_sum += pressure_pa;
        pressure_samples++;
        if (calibrating) {
            err = pid_autotune_update(&autotune, pressure_pa, &duty, &enclosure1);
            if (err != AT_OK) {
                goto stop_motor;
            }
            if (autotune.state == AT_DONE) {
                calibrating = 0;
                output_start = autotune.output_start;
                duty = output_start;
                err = pwm_set_percent(duty);
                if (err != PWM_OK) {
                    LOG_ERR("PWM failed with code error: %d", err);
                    goto stop_motor;
                }
                printf("\r\n/* Copy both the PID gains and the operating PWM. */\r\n"
                       "pidParm enclosure1 = {\r\n"
                       "    .Kp = %.6ff,\r\n"
                       "    .Ki = %.6ff,\r\n"
                       "    .Kd = %.6ff,\r\n"
                       "    .SetPoint = %.2ff,\r\n"
                       "};\r\n"
                       "float enclosure1_output_start = %.6ff;\r\n\r\n",
                       (double)enclosure1.Kp, (double)enclosure1.Ki,
                       (double)enclosure1.Kd, (double)enclosure1.SetPoint,
                       (double)output_start);
                if (select_yes_no("Zapisac nastawy do pamieci MCU?")) {
                    printf("Podaj nazwe obudowy: ");
                    char *name = console_getline();
                    err = enclosure_profile_save(profile_index, name, enclosure1,
                                                 output_start);
                    if (err != 0) {
                        LOG_ERR("Profile save failed with code error: %d", err);
                    } else {
                        printf("Zapisano profil %s w pozycji %u.\r\n",
                               name, profile_index + 1);
                    }
                }
                pid_init();
                pid_time = k_uptime_get();
            }
        } else if (k_uptime_get() - pid_time >=
                   (fabsf(enclosure1.SetPoint - pressure_pa) > 10.0f ? 50 : 100)) {
            pid_time = k_uptime_get();
            pid_calc(pressure_pa, &pidTerm, enclosure1);
            duty = output_start + pidTerm;
            duty = duty < 7.9f ? 0.0f : MIN(duty, 50.0f);
        }

        if (k_uptime_get() - telemetry_time >= 100) {
            telemetry_time = k_uptime_get();
            avg_pressure = pressure_sum / (float)pressure_samples;
            pressure_sum = 0.0f;
            pressure_samples = 0;
            printf(">Pressure:%.4f,avgPressure:%.4f,duty:%.4f,pidTerm:%.4f\r\n",
                   (double)pressure_pa, (double)avg_pressure,
                   (double)duty, (double)pidTerm);
        }

        err = pwm_set_percent(duty);
        if (err != PWM_OK) {
            LOG_ERR("PWM failed with code error: %d", err);
            goto stop_motor;
        }

        // printf(">Pressure:%.4f,pidTerm:%.4f,duty:%.4f,l_min:%.4f\r\n",
        //     (double)pressure_pa,
        //     (double)pidTerm,
        //     (double)duty,
        //     (double)l_min);
        k_msleep(10);
    }

stop_motor:
    LOG_ERR("Control stopped; setting turbine PWM to zero. Reset to retry.");
    err = pwm_set_percent(0.0f);
    if (err != PWM_OK) {
        LOG_ERR("Motor stop failed with code error: %d", err);
    }
    return -1;
}
