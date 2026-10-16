#ifndef MPU6050_H
#define MPU6050_H

#include "esp_err.h"
#include <stdint.h>


#define MPU6050_ADDR 0x68

typedef struct
{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;

    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;

    int16_t temperature;

} mpu6050_data_t;

esp_err_t mpu6050_init(void);

esp_err_t mpu6050_read(mpu6050_data_t *data);


#endif