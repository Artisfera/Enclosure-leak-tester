#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi, LOG_LEVEL_DBG);

#define SPIOP SPI_WORD_SET(8) | SPI_TRANSFER_MSB
struct spi_dt_spec honeywell_pressure = SPI_DT_SPEC_GET(DT_NODELABEL(honeywell_pressure), SPIOP);



int spi_init(void)
{
    bool ready;

    do {
        ready = spi_is_ready_dt(&honeywell_pressure);

        if (!ready) {
            LOG_ERR("SPI interface: NOT READY");
        }

    } while (!ready);

    LOG_INF("SPI interface: READY");

    return 0;
}

int spi_read_pressure(float *psi)
{
    uint8_t rx_data[2] = {0};

    struct spi_buf rx_buf = {
        .buf = rx_data,
        .len = sizeof(rx_data)
    };

    struct spi_buf_set rx = {
        .buffers = &rx_buf,
        .count = 1
    };

    int err;

    do {
        err = spi_read_dt(&honeywell_pressure, &rx);

        if (err < 0) {
        LOG_ERR("SPI read failed: %d", err);
        }
    } while (err < 0);

    uint8_t status = rx_data[0] >> 6;

    LOG_DBG("SPI read data: %02X %02X", rx_data[0], rx_data[1]);
    LOG_DBG("SPI read status: %d", status);

    if (status != 0) {
        LOG_ERR("Honeywell status error: %d", status);
        return -1;
    }

    int output = (rx_data[0] & 0x3F) * 256 + rx_data[1];
    *psi = ((output - 1638.0f) * 2.0f) / 13108.0f - 1.0f;

    LOG_DBG("Pressure: %.3f psi", (double)*psi);

    return 0;
}