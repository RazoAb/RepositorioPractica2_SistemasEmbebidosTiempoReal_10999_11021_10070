#include "queues.h"

QueueHandle_t g_sensorQueue = NULL;
QueueHandle_t g_commandQueue = NULL;
QueueHandle_t g_displayQueue = NULL;

void queuesInit(void)
{
    g_sensorQueue = xQueueCreate(10, sizeof(sensor_message_t));
    g_commandQueue = xQueueCreate(5, sizeof(command_t));
    g_displayQueue = xQueueCreate(1, sizeof(display_data_t));
}