#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm, LOG_LEVEL_DBG);

static const struct device *pwm0_motor = DEVICE_DT_GET(DT_NODELABEL(pwm0));

int pwm_set_percent(float duty)
{
    uint32_t period = PWM_KHZ(25);
    uint32_t pulse = (uint32_t)((period * duty) / 100.0f);

    LOG_DBG("Duty: %.2f", duty);

    int err = pwm_set(
        pwm0_motor,
        0,
        period,
        pulse,
        PWM_POLARITY_NORMAL
    );

    if (err < 0) {
        LOG_ERR("PWM set failed: %d", err);
        return err;
    }

    return 0;

    return 0;
}