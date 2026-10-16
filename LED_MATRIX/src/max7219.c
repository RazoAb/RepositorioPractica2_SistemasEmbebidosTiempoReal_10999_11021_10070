#include "max7219.h"
#include <string.h>
#include "driver/gpio.h"
#include "esp_check.h"

#define REG_DECODE_MODE  0x09
#define REG_INTENSITY    0x0A
#define REG_SCAN_LIMIT   0x0B
#define REG_SHUTDOWN     0x0C
#define REG_DISPLAY_TEST 0x0F

static void send_quad(
    max7219_t *dev,
    uint8_t reg0, uint8_t data0,
    uint8_t reg1, uint8_t data1,
    uint8_t reg2, uint8_t data2,
    uint8_t reg3, uint8_t data3)
{
    /*
     * En cascada:
     * dispositivo más lejano primero.
     */
    uint8_t tx[8] = {
        reg3, data3,
        reg2, data2,
        reg1, data1,
        reg0, data0
    };

    spi_transaction_t t =
    {
        .length = 64,
        .tx_buffer = tx
    };

    spi_device_transmit(dev->spi, &t);
}

static void write_register_all(max7219_t *dev, uint8_t reg, uint8_t value)
{
    send_quad(
        dev,
        reg, value,
        reg, value,
        reg, value,
        reg, value
    );
}

void max7219_init(max7219_t *dev, spi_host_device_t host,int pin_mosi, int pin_clk, int pin_cs,uint8_t device_count)
{
    spi_bus_config_t buscfg = 
    {
        .mosi_io_num = pin_mosi,
        .miso_io_num = -1,
        .sclk_io_num = pin_clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4
    };

    spi_device_interface_config_t devcfg = 
    {
        .clock_speed_hz = 1000000,
        .mode = 0,
        .spics_io_num = pin_cs,
        .queue_size = 1
    };

    dev->device_count = device_count;

    ESP_ERROR_CHECK(spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(host, &devcfg, &dev->spi));

    write_register_all(dev, REG_DISPLAY_TEST, 0x00);
    write_register_all(dev, REG_DECODE_MODE,  0x00);
    write_register_all(dev, REG_SCAN_LIMIT,   0x07);
    write_register_all(dev, REG_INTENSITY,    0x03);
    write_register_all(dev, REG_SHUTDOWN,     0x01);

    max7219_clear(dev);
}

void max7219_clear(max7219_t *dev)
{
    for (uint8_t row = 1; row <= 8; ++row)
    {
        send_quad(
            dev,
            row, 0x00,
            row, 0x00,
            row, 0x00,
            row, 0x00
        );
    }
}

void max7219_write_rows(
    max7219_t *dev,
    const uint8_t *matrix0,
    const uint8_t *matrix1,
    const uint8_t *matrix2,
    const uint8_t *matrix3)
{
    for (uint8_t row = 0; row < 8; ++row)
    {
        send_quad(
            dev,
            row + 1, matrix0[row],
            row + 1, matrix1[row],
            row + 1, matrix2[row],
            row + 1, matrix3[row]
        );
    }
}
