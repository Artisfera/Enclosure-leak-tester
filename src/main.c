#include <zephyr/kernel.h>
#include "adc.h"
#include "pwm.h"
#include "spi.h"
#include "pid.h"
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);



int main(void)
{
        //printf("Hello World!\n");

        adc_init();
        spi_init();

        float psi = 0.0f;
        float duty = 0;
        int ml_min = 0;
        //int monitoring_duration = 120000;
        int64_t prev_time = k_uptime_get();


        float Kp = 200.0f;
        float Ki = 200.0f;
        float Kd = 5.0f;

        float setPoint = 0.15f;
        float integral = 0.0f;
        float derivative = 0.0f;
                spi_read_pressure(&psi);
        float previousError = setPoint - psi;
        

        while(1) {
                adc_read_flowrate(&ml_min);
                spi_read_pressure(&psi);

                float error = setPoint - psi;
                float pTerm = Kp * error;

                float dt = (float)k_uptime_delta(&prev_time) / 1000.0f;
                integral += error * dt;
                float iTerm = Ki * integral;

                derivative = (error - previousError) / dt;
                float dTerm = Kd * derivative;
                previousError = error;

                duty = pTerm + iTerm + dTerm;
                duty = CLAMP(duty, 0.0f, 50.0f);
                pwm_set_percent(duty);
                printf(">Pressure:%.4f,setPoint:%.4f,error:%.4f,pTerm:%.4f,iTerm:%.4f,dTerm:%.4f,duty:%.4f,ml_min:%d\r\n", psi, setPoint, error, pTerm, iTerm, dTerm, duty, ml_min);

                k_msleep(50);
        }
        
        /*while(psi <= 0.3f) {
                duty = MIN(duty + 0.05f, 15.0f);
                pwm_set_percent(duty);

                spi_read_pressure(&psi);

                adc_read_flowrate(&ml_min);

                k_msleep(1000);

        }

        duty = 0;
        pwm_set_percent(duty);
        LOG_INF("Vaccum Genereted!");
        
        while(k_uptime_get() - start_time <= monitoring_duration){
                spi_read_pressure(&psi);
                adc_read_flowrate(&ml_min);

                LOG_INF("Pressure (psi): %.3f", (double)psi);
                LOG_INF("Flowrate (ml/min): %d", ml_min);
                int64_t time_remaining = (monitoring_duration - (k_uptime_get() - start_time)) / 1000;
                LOG_INF("Time remaining (s): %d", time_remaining);

                k_msleep(10000);
        }*/

        return 0;
}