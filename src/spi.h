#ifndef SPI_H_
#define SPI_H_

typedef enum {
    SPI_OK, 
    SPI_INTERFACE_ERR, 
    SPI_READ_ERR, 
    SENSOR_STATUS_ERR,
} spi_status;

int spi_init(void);
int spi_read_pressure(float *psi);


#endif