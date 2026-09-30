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
    float psi = 0.0f;
    float duty = 0.0f;
    float l_min = 0.0f;
    float pidTerm = 0.0f;
    //printf("Hello World!\n");
    
    pidParm enclosure1 = {
        .Kp = 200.0f,
        .Ki = 200.0f,
        .Kd = 5.0f,
        .SetPoint = 0.15f,
    };
    
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