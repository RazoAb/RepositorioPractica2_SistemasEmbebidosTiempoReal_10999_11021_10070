#ifndef MAX7219_H
#define MAX7219_H

#include <stdint.h>
#include "driver/spi_master.h"

typedef struct 
{
    spi_device_handle_t spi;
    uint8_t device_count;
} max7219_t;

void max7219_init(max7219_t *dev, spi_host_device_t host,int pin_mosi, int pin_clk, int pin_cs,uint8_t device_count);
void max7219_clear(max7219_t *dev);
void max7219_write_rows(
    max7219_t *dev,
    const uint8_t *matrix0,
    const uint8_t *matrix1,
    const uint8_t *matrix2,
    const uint8_t *matrix3
);

#endif
