#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "ds18b20.h"
#include "app_config.h"
#include "data_types.h"
#include "tasks.h"

void vTaskTemp(void *pvParameters)
{
    uint8_t sensor_addr[8] = {0};
    sensor_task_params_t *params = (sensor_task_params_t *) pvParameters;

    ds18b20_dev_t dev = {0};
    ds18b20_init(&dev, DS18B20_GPIO_PIN);
    ds18b20_search_reset(&dev);

    while (ds18b20_search(&dev, sensor_addr) != 1) {
        printf("Error: No se encontro ningun sensor DS18B20 en GPIO %d. Reintentando...\n", DS18B20_GPIO_PIN);
        ds18b20_search_reset(&dev);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    printf("Sensor DS18B20 encontrado!\n");

    TickType_t lastWake = xTaskGetTickCount();

    while (1)
    {
        sensor_message_t msg = {0};
        msg.type = SENSOR_TYPE_TEMP;

        params->period_ms = (*params->acq_mode == ACQ_MODE_NORMAL) ? 200 : 50;

        // 3. Ordenar al sensor que convierta la temperatura antes de leerla
        ds18b20_requestTemperatures(&dev);

        // Leer sensor
        msg.data.temp.temperature = ds18b20_getTempC(&dev, sensor_addr);
        msg.timestamp = xTaskGetTickCount();

        // Enviar a queue
        xQueueSend(params->sensor_queue, &msg, 0);

        //printf("Temperatura: %.2f °C\n", msg.data.temp.temperature);

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(params->period_ms));
    }
}