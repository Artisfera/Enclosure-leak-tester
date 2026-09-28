#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/dt-bindings/adc/nrf-saadc.h>
#include <zephyr/logging/log.h>

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
        int err = adc_channel_setup(adc, &channel_cfg);

        if (err < 0) {
                LOG_ERR("ADC channel setup failed: %d", err);
                return err;
        }

        sequence.calibrate = true;

        err = adc_read(adc, &sequence);

        sequence.calibrate = false;

        if (err < 0) {
                LOG_ERR("ADC calibration failed: %d", err);
                return err;
        }

        return 0;
}


int adc_read_flowrate(int *ml_min)
{
        int err = adc_read(adc, &sequence);

        if (err < 0) {
                LOG_ERR("ADC read failed: %d", err);
                return err;
        }

        int32_t voltage_mv = sample;

        err = adc_raw_to_millivolts(
                600,
                channel_cfg.gain,
                sequence.resolution,
                &voltage_mv
        );

        if (err < 0) {
                LOG_ERR("ADC conversion failed: %d", err);
                return err;
        }

        //*ml_min = (voltage_mv - 500) / 20;
        *ml_min = voltage_mv;

        /*if (*ml_min > 100) {
                LOG_ERR("Flow is too HIGH! - %d ml/min", *ml_min);
        } else {
                LOG_DBG("Voltage: %d mV", voltage_mv);
                LOG_DBG("Flow: %d ml/min", *ml_min);
        }*/

        return 0;
}