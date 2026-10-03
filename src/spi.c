#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi, LOG_LEVEL_DBG);

#include "spi.h"

#define SPIOP SPI_WORD_SET(8) | SPI_TRANSFER_MSB
struct spi_dt_spec sensor_pressure = SPI_DT_SPEC_GET(DT_NODELABEL(sensor_pressure), SPIOP);


int spi_init(void)
{
    bool ready;

    ready = spi_is_ready_dt(&sensor_pressure);

    if (!ready) {
        LOG_ERR("SPI interface: NOT READY");
        return SPI_INTERFACE_ERR;
    }


    LOG_INF("SPI interface: READY");

        return SPI_OK;
}

int spi_read_pressure(float *pressure_pa)
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

        err = spi_read_dt(&sensor_pressure, &rx);

        if (err < 0) {
        LOG_ERR("SPI read failed: %d", err);
        return SPI_READ_ERR;
        }

    uint8_t status = rx_data[0] >> 6;

    LOG_DBG("SPI read data: %02X %02X", rx_data[0], rx_data[1]);
    LOG_DBG("SPI read status: %d", status);

    // if (status != 0) {
    //     LOG_ERR("Sensor status error: %d", status);
    //     return SENSOR_STATUS_ERR;
    // }

    int output = (rx_data[0] & 0x3F) * 256 + rx_data[1];

    *pressure_pa = -498.18f + ((output - 1638.0f) * (996.36f / 13108.0f));

    LOG_DBG("Pressure: %.2f Pa", (double)*pressure_pa);

    return SPI_OK;
}

