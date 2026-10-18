#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

#include "adc.h"
#include "pwm.h"
#include "spi.h"
#include "pid.h"


LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

int main(void)
{
    float pressure_pa = 0.0f;
    float duty = 0.0f;
    float l_min = 0.0f;
    float pidTerm = 0.0f;
    //printf("Hello World!\n");
    
    pidParm enclosure1 = {
        .Kp = 0.221764f,
        .Ki = 0.041809f,
        .Kd = 0.084869f,
        .SetPoint = 50.00f,
    };
    //float enclosure1_output_start = 11.983146f;
    
    uint8_t err;   
    err = adc_init();
    if (err != 0){
        LOG_ERR("ADC failed with code error: %d", err);
        return -1;
    };

    err = spi_init();
    if (err != 0){
        LOG_ERR("SPI failed with code error: %d", err);
        return -1;
    };

    pid_init();


    printf("Count, Pressure_(Pa),Duty\r\n");

    int count = 0;
    duty = 6.0f;

    int64_t next_duty_change = k_uptime_get() + 60000;
/*
    while (1) {
        adc_read_flowrate(&l_min);

        spi_read_pressure(&pressure_pa);
        printf(">Pressure:%.4f,duty:%.4f", (double)pressure_pa, duty);
        pid_calc(pressure_pa, &pidTerm, enclosure1);

        //duty = enclosure1_output_start + pidTerm;
        //duty = CLAMP(duty, 0.0f, 50.0f);
        duty = 10;
        pwm_set_percent(duty);

        // printf(">Pressure:%.4f,pidTerm:%.4f,duty:%.4f,l_min:%.4f\r\n",>
        //     (double)pressure_pa,
        //     (double)pidTerm,
        //     (double)duty,
        //     (double)l_min);
        k_msleep(100);
}
        */
    while (1) {
        adc_read_flowrate(&l_min);
        spi_read_pressure(&pressure_pa);

        count++;

        printf("%d,%.4f,%.4f,%4f\r\n",
            count,
            (double)pressure_pa,
            (double)duty,
            l_min);
        pwm_set_percent(duty);

        int64_t now = k_uptime_get();

        if (now >= next_duty_change) {
            duty += 2.0f;

            if (duty >= 30.0f) {
                pwm_set_percent(0.0f);
                break;
            }

            next_duty_change += 30000;
        }

        k_msleep(100);
    }

    return 0;
}