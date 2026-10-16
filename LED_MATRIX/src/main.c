
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "max7219.h"
#include "tasks.h"
#include "queues.h"
#include "buttons.h"
#include "distance.h"
#include "data_types.h"
#include "mpu6050.h"
#include "display.h"


QueueHandle_t rollQueue;
void app_main(void)
{
    queuesInit();
    button_config();
    display_init();

  

    // -----------------------------
    // Task del display
    // -----------------------------

    // -----------------------------
    // Task MPU6050
    // -----------------------------
    
    static acq_mode_t acq_mode = ACQ_MODE_NORMAL;

    static sensor_task_params_t t = {0};
    static sensor_task_params_t d = {0};
    static sensor_task_params_t i = {0};

    t.acq_mode = &acq_mode;
    t.sensor_type = SENSOR_TYPE_TEMP;
    t.sensor_queue = g_sensorQueue;

    d.acq_mode = &acq_mode;
    d.sensor_type = SENSOR_TYPE_DISTANCE;
    d.sensor_queue = g_sensorQueue;

    i.acq_mode = &acq_mode;
    i.sensor_type = SENSOR_TYPE_IMU;
    i.sensor_queue = g_sensorQueue;
    xTaskCreate(vTaskTemp, "ds18b20_task", 2048, &t, 2, NULL);
    xTaskCreate(vTaskDistance, "distancia", 2048, &d, 2, NULL);
    xTaskCreate(vTaskIMU, "IMU", 4096, &i, 2, NULL);
    xTaskCreate(vTaskDataManager, "DataManager", 2048, &acq_mode, 4, NULL);
    xTaskCreate(display_task,"TaskDisplay",4096,NULL,2,NULL);
}
