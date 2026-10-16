#include "data_types.h"
#include "queues.h"

static void impresion(display_data_t *d)
{
    printf("======== DATOS ========\n");
    printf("Temperatura: %.2f °C\n", d->temperature);
    printf("Distancia: %u cm\n", d->distance_cm);
    printf("IMU:\nROLL: %.2f\n", d->roll);
    printf("PITCH: %.2f\n", d->pitch);
    printf("======= ESTADOS ======\n");
    printf("ACQ MODE: %s\n", (d->acq_mode) ? "NORMAL" : "LOAD");
    printf("DISPLAY MODE: %s\n", (d->display_mode) ? "NUMBER" : "GRAPHIC");
    printf("FREEZE MODE: %s\n\n", (d->freeze) ? "FROZEN" : "NORMAL");
}

void vTaskDataManager(void *pvParameters)
{
    acq_mode_t *acq_mode = (acq_mode_t *)pvParameters;
    display_data_t display_data = {0};
    command_t command;
    sensor_message_t m;
    TickType_t last_wake  = xTaskGetTickCount();
    while(1)
    {
        if(xQueueReceive(g_commandQueue, &command, 0) == pdTRUE)
        {
            switch (command)
            {
            case CMD_CHANGE_MODE:
                //CAMBIAR DISPLAY MODE
                display_data.display_mode = (display_data.display_mode == DISPLAY_MODE_LEVEL) ? DISPLAY_MODE_GRAPHIC : DISPLAY_MODE_LEVEL;
                break;
            case CMD_OVERLOAD:
                //FREEZE
                display_data.acq_mode = (display_data.acq_mode == ACQ_MODE_NORMAL) ? ACQ_MODE_LOAD : ACQ_MODE_NORMAL;
                *acq_mode = display_data.acq_mode;
                break;
            case CMD_FREEZE:
                //CAMBIAR LOAD
                display_data.freeze = !display_data.freeze;
                break;
            
            default:
                break;
            }
            xQueueOverwrite(g_displayQueue, &display_data);
        }
        if(xQueueReceive(g_sensorQueue, &m, 0) == pdTRUE)
        {
            switch (m.type)
            {
            case SENSOR_TYPE_DISTANCE:
                display_data.distance_cm = m.data.distance.distance_cm;
                break;
            case SENSOR_TYPE_IMU:
                display_data.pitch = m.data.imu.pitch;
                display_data.roll = m.data.imu.roll;
                break;
            case SENSOR_TYPE_TEMP:
                display_data.temperature = m.data.temp.temperature;
                break;
            default:
                break;
            }
            xQueueOverwrite(g_displayQueue, &display_data);
        }
        impresion(&display_data);
        vTaskDelay(pdMS_TO_TICKS(20));
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));
    }
}