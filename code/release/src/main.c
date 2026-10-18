#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <inttypes.h>
#include <stdio.h>

#include "adc.h"
#include "pwm.h"
#include "spi.h"
#include "pid.h"

#define SW0_NODE DT_ALIAS(sw0) 
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

int main(void)
{
    float pressure_pa = 0.0f;
    float duty = 0.0f;
    float l_min = 0.0f;
    float pidTerm = 0.0f;
    float l_air = 0.0f;
    int ret;
    
    pidParm enclosure1 = {
        .Kp = 0.221764f,
        .Ki = 0.05809f,
        .Kd = 0.24869f,
        .SetPoint = 50.00f,
    };
    float enclosure1_output_start = 11.983146f;
    
    uint8_t err;   

    if (!gpio_is_ready_dt(&button)) {
        LOG_ERR("Button GPIO device is not ready");
        return -1;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
        LOG_ERR("Cannot configure button GPIO: %d", ret);
        return -1;
    }

    err = adc_init();
    if (err != 0){
        LOG_ERR("ADC failed with code error: %d", err);
        return -1;
    };

    float flow_cal = 0.0f;

    for (int count = 0; count < 100; count++) {
        int err = adc_read_flowrate(&l_min);

        if (err != ADC_OK) {
            return err;
        }

        flow_cal += l_min;
        k_msleep(10);
    }

    float flow_zero = flow_cal / 100.0f;

    err = spi_init();
    if (err != 0){
        LOG_ERR("SPI failed with code error: %d", err);
        return -1;
    };

    pid_init();

    while (1) {
        int val = gpio_pin_get_dt(&button);

        if (val < 0) {
            LOG_ERR("Cannot read button GPIO: %d", val);
            k_msleep(100);
            continue;
        }

        if (val > 0) {
            int64_t measurement_start = k_uptime_get();
            int64_t previous_sample_time_ms = measurement_start;
            float previous_l_min = 0.0f;
            bool have_previous_sample = false;

            l_air = 0.0f;
            printf("Start measure:\n");
            
            do {
            int adc_err = adc_read_flowrate(&l_min);
            l_min = l_min - flow_zero;

            if (adc_err == ADC_OK) {
                int64_t sample_time_ms = k_uptime_get();

                if (have_previous_sample) {
                    int64_t dt_ms = sample_time_ms - previous_sample_time_ms;
                    float average_l_min = 0.5f * (previous_l_min + l_min);

                    /* (L/min) * ms / (60 000 ms/min) = L */
                    l_air += average_l_min * ((float)dt_ms / 60000.0f);
                }

                previous_l_min = l_min;
                previous_sample_time_ms = sample_time_ms;
                have_previous_sample = true;
            }

            spi_read_pressure(&pressure_pa);
            pid_calc(pressure_pa, &pidTerm, enclosure1);

            duty = enclosure1_output_start + pidTerm;
            duty = CLAMP(duty, 0.0f, 50.0f);
            pwm_set_percent(duty);
            
            int64_t elapsed_ms = k_uptime_get() - measurement_start;
            printf(">l:%.4f,millisecond:%" PRId64 "\n",
                   (double)l_air, elapsed_ms);

            k_msleep(100);  

            val = gpio_pin_get_dt(&button);
            } while (val > 0);

        }    

         adc_read_flowrate(&l_min);
            //l_min = l_min - flow_zero;

         spi_read_pressure(&pressure_pa);
         pid_calc(pressure_pa, &pidTerm, enclosure1);

         duty = enclosure1_output_start + pidTerm;
         duty = CLAMP(duty, 0.0f, 100.0f);
         pwm_set_percent(duty);

          printf(">Pressure:%.4f,pidTerm:%.4f,duty:%.4f,l_min:%.4f\r\n",
              (double)pressure_pa,
              (double)pidTerm,
              (double)duty,
              (double)l_min);
         k_msleep(100);
}

    return 0;
}
