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
    //printf("Hello World!\n");
    
    uint8_t err;   
    err = adc_init();
    if (err != 0){
        LOG_ERR("ADC faile with code error %d", err);
        return -1;
    }
    spi_init();
    pid_init();

    pidParm enclosure1 = {
        200.0f,
        200.0f,
        5.0f,
        0.15f
    };
    
    float psi = 0.0f;
    float duty = 0.0f;
    float l_min = 0.0f;
    float pidTerm = 0.0f;


    while (1) {
        adc_read_flowrate(&l_min);

        spi_read_pressure(&psi);

        pid_calc(psi, &pidTerm, enclosure1);

        duty = pidTerm;
        //duty = CLAMP(duty, 0.0f, 50.0f);

        pwm_set_percent(duty);

        printf(">Pressure:%.4f,pidTerm:%.4f,duty:%.4f,l_min:%.4f\r\n",
            (double)psi,
            (double)pidTerm,
            (double)duty,
            (double)l_min);

        k_msleep(50);
}

    return 0;
}