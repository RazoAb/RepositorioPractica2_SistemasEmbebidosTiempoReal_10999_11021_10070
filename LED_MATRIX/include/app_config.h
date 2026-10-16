#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>
#include "driver/gpio.h"

/*#define DS18B20_GPIO_PIN 4
#define BUTTON_FREEZE   5
#define BUTTON_MODE     6
#define BUTTON_LOAD     7
//SPI para displays
#define PIN_MOSI 35
#define PIN_CLK  37
#define PIN_CS   36
//I2C para MPU
#define PIN_SCL     GPIO_NUM_8
#define PIN_SDA     GPIO_NUM_9
//Sensor Ultrasonico
#define DISTANCE_ECHO_PIN         GPIO_NUM_14
#define DISTANCE_TRIG_PIN     GPIO_NUM_15*/

#define DS18B20_GPIO_PIN 4
#define BUTTON_FREEZE   16
#define BUTTON_MODE     17
#define BUTTON_LOAD     18
//SPI para displays
#define PIN_MOSI 42
#define PIN_CLK  40
#define PIN_CS   41
//I2C para MPU
#define PIN_SCL     5
#define PIN_SDA     6
//Sensor Ultrasonico
#define DISTANCE_ECHO_PIN     15
#define DISTANCE_TRIG_PIN     7

#endif