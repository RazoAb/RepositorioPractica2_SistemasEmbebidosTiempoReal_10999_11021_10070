#ifndef DATA_TYPES
#define DATA_TYPES

#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef enum{
    DISPLAY_MODE_GRAPHIC = 0,
    DISPLAY_MODE_LEVEL
} display_mode_t;

typedef enum{
    ACQ_MODE_NORMAL = 0,
    ACQ_MODE_LOAD
} acq_mode_t;

typedef enum{
    CMD_CHANGE_MODE = 0,
    CMD_FREEZE,
    CMD_OVERLOAD    
} command_t;

typedef enum{
    SENSOR_TYPE_IMU = 0,
    SENSOR_TYPE_TEMP,
    SENSOR_TYPE_DISTANCE
} sensor_type_t;

typedef struct {
    sensor_type_t type;      // qué sensor generó el dato
    union {
        struct { float pitch; float roll; } imu;
        struct { float temperature; }       temp;
        struct { uint16_t distance_cm; }    distance;
    } data;
    uint32_t timestamp;      // cuándo se tomó la lectura
} sensor_message_t;

typedef struct
{
    float pitch;
    float roll;
    float temperature;
    uint16_t distance_cm;
    display_mode_t display_mode;
    acq_mode_t acq_mode;
    bool freeze;
} display_data_t;

typedef struct
{
    float roll;
    float pitch;
} horizon_data_t;

typedef struct dis_sensor {
    int trigger_pin;
    int echo_pin;
} dis_sensor_t;
#endif

