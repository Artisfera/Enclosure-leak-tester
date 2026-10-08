#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/dt-bindings/adc/nrf-saadc.h>
#include <zephyr/logging/log.h>

#include "adc.h"

LOG_MODULE_REGISTER(adc, LOG_LEVEL_DBG);

static const struct device *adc = DEVICE_DT_GET(DT_NODELABEL(adc));

static const struct adc_channel_cfg channel_cfg = {
        .input_positive = NRF_SAADC_AIN7,
        .gain = ADC_GAIN_1_5,
        .reference = ADC_REF_INTERNAL,
        .acquisition_time = ADC_ACQ_TIME_DEFAULT,
        .channel_id = 0,
        .differential = 0,
};

static int16_t sample;

static struct adc_sequence sequence = {
        .channels = BIT(0),
        .buffer = &sample,
        .buffer_size = sizeof(sample),
        .resolution = 14,
        .oversampling = 6,
};


int adc_init(void)
{
    int err;

    err = adc_channel_setup(adc, &channel_cfg);

    if (err < 0) {
            LOG_ERR("ADC channel setup failed: %d", err);
            return ADC_CH_SETUP_ERR;
    }

    sequence.calibrate = true;
    
    err = adc_read(adc, &sequence);

    if (err < 0) {
            LOG_ERR("ADC calibration failed: %d", err);
            return ADC_READ_ERR;
    }

    sequence.calibrate = false;

        return ADC_OK;
}


int adc_read_flowrate(float *l_min)
{
    int err;

    err = adc_read(adc, &sequence);

    if (err < 0) {
        LOG_ERR("ADC read failed: %d", err);
        return ADC_READ_ERR;
    }

        int32_t voltage_mv = sample;

        float voltage = voltage_mv / 1000.0f;
        *l_min = (((((0.094003f * voltage - 0.564312f) * voltage
                  + 1.374705f) * voltage
                  - 1.601495f) * voltage
                  + 1.060657f) * voltage
                  - 0.269996f)
                  * 100.0f;

        return ADC_OK;
}