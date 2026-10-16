#ifndef BUTTONS_H
#define BUTTONS_H

#include "app_config.h"
#include <stdint.h>
#include "driver/gpio.h"
#include "esp_attr.h" // <- Crucial para que el compilador entienda IRAM_ATTR

//static void IRAM_ATTR gpio_isr_handler(void* arg);
void button_init(gpio_num_t pin);
void button_config(void);

#endif