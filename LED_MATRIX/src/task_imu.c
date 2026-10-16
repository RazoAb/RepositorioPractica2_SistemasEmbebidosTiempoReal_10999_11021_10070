#include <math.h>
#include "tasks.h"
#include "mpu6050.h"
#include "app_config.h"
#include "esp_log.h"


static const char *TAG = "T_IMU";

#define ACCEL_LSB_PER_G   16384.0f      // MPU6050 configurado a ±2 g 
#define RAD_TO_DEG        57.29577951f

void vTaskIMU(void *pvParameters)
{

    sensor_task_params_t *params = (sensor_task_params_t *)pvParameters;

    if (params == NULL || params->sensor_queue == NULL || params->acq_mode == NULL) {
        ESP_LOGE(TAG, "Parámetros inválidos, tarea detenida");
        vTaskDelete(NULL);
    }

     //mpu6050_data_t sensor;
       
    if (mpu6050_init() != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo inicializar el MPU6050, tarea detenida");
        vTaskDelete(NULL);
    }


    TickType_t last_wake  = xTaskGetTickCount();
    //TickType_t last_stats = last_wake;

    while (1)
    {
        mpu6050_data_t raw;
        if (mpu6050_read(&raw) == ESP_OK)
        {
            // Convertir a g y calcular ángulos 
            float ax = raw.accel_x / ACCEL_LSB_PER_G;
            float ay = raw.accel_y / ACCEL_LSB_PER_G;
            float az = raw.accel_z / ACCEL_LSB_PER_G;

            float roll  = atan2f(ay, az) * RAD_TO_DEG;
            float pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG;

            sensor_message_t msg = {0};
            
            msg.type = SENSOR_TYPE_IMU;
            msg.data.imu.pitch = pitch;  // Inicializar pitch
            msg.data.imu.roll = roll;   // Inicializar roll
            msg.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;

            xQueueSend(params->sensor_queue, &msg, 0);
        }

        // task_imu.c, línea 57
        params->period_ms = (*params->acq_mode == ACQ_MODE_NORMAL) ? 200 : 50;
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(params->period_ms));

    }
}