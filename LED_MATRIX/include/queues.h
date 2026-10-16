#ifndef QUEUES_H
#define QUEUES_H

#include "data_types.h"
#include "stdint.h"

//Cola de mensajes de sensore
extern QueueHandle_t g_sensorQueue;

//Cola de comandos de botones
extern QueueHandle_t g_commandQueue;

//Cola de datos de visualizacion
extern QueueHandle_t g_displayQueue;

void queuesInit(void);

#endif