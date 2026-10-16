#ifndef DISPLAY_H
#define DISPLAY_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void display_init(void);
void display_task(void *pvParameters);

#endif