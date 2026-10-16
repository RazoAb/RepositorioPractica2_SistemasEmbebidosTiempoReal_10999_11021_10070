#ifndef TASKS
#define TASKS

#include "data_types.h"

extern TaskHandle_t g_taskIMU;
extern TaskHandle_t g_taskTemp;
extern TaskHandle_t g_taskDistance;
extern TaskHandle_t g_taskButtons;
extern TaskHandle_t g_taskDataManager;
extern TaskHandle_t g_taskDisplay;

typedef struct {
    sensor_type_t sensor_type;
    uint32_t period_ms;
    acq_mode_t *acq_mode;
    QueueHandle_t sensor_queue;
} sensor_task_params_t;

void vTaskIMU(void *pvParameters);
void vTaskTemp(void *pvParameters);
void vTaskDistance(void *pvParameters);
void vTaskButtons(void *pvParameters);
void vTaskDataManager(void *pvParameters);
void display_task(void *pvParameters);


#endif