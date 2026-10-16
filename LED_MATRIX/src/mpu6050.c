#include "mpu6050.h"
#include "math.h"
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "data_types.h"

#include "driver/i2c.h"
#include "esp_err.h"
#include "app_config.h"

extern QueueHandle_t rollQueue;

// --------------------------------------------------
// Configuración I2C
// --------------------------------------------------

#define I2C_PORT        I2C_NUM_0


#define I2C_FREQ_HZ     100000

// --------------------------------------------------
// Registros MPU6050
// --------------------------------------------------

#define REG_SMPLRT_DIV      0x19
#define REG_CONFIG          0x1A
#define REG_GYRO_CONFIG     0x1B
#define REG_ACCEL_CONFIG    0x1C

#define REG_ACCEL_XOUT_H    0x3B
#define REG_TEMP_OUT_H     0x41
#define REG_GYRO_XOUT_H     0x43

#define REG_PWR_MGMT_1     0x6B
#define REG_WHO_AM_I       0x75

// --------------------------------------------------
// Función para escribir un registro
// --------------------------------------------------

horizon_data_t horizon_data;

static esp_err_t mpu6050_write_register(uint8_t reg, uint8_t value)
{
    uint8_t data[2];

    data[0] = reg;
    data[1] = value;

    return i2c_master_write_to_device(
        I2C_PORT,
        MPU6050_ADDR,
        data,
        sizeof(data),
        pdMS_TO_TICKS(100)
    );
}

// --------------------------------------------------
// Función para leer registros
// --------------------------------------------------

static esp_err_t mpu6050_read_registers(
    uint8_t reg,
    uint8_t *data,
    size_t length)
{
    return i2c_master_write_read_device(
        I2C_PORT,
        MPU6050_ADDR,
        &reg,
        1,
        data,
        length,
        pdMS_TO_TICKS(100)
    );
}

// --------------------------------------------------
// Inicialización I2C
// --------------------------------------------------

static esp_err_t i2c_init(void)
{
    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,

        .sda_io_num = PIN_SDA,
        .scl_io_num = PIN_SCL,

        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,

        .master.clk_speed = I2C_FREQ_HZ
    };

    esp_err_t err;

    err = i2c_param_config(I2C_PORT, &config);

    if (err != ESP_OK)
        return err;

    err = i2c_driver_install(
        I2C_PORT,
        I2C_MODE_MASTER,
        0,
        0,
        0
    );

    return err;
}

// --------------------------------------------------
// Inicialización MPU6050
// --------------------------------------------------

esp_err_t mpu6050_init(void)
{
    esp_err_t err;

    printf("\nInicializando MPU6050...\n");

    // Inicializar bus I2C
    err = i2c_init();

    if (err != ESP_OK)
    {
        printf("ERROR: No se pudo inicializar I2C\n");
        return err;
    }

    // --------------------------------------------------
    // Comprobar WHO_AM_I
    // --------------------------------------------------

    uint8_t who_am_i = 0;

    err = mpu6050_read_registers(
        REG_WHO_AM_I,
        &who_am_i,
        1
    );

    if (err != ESP_OK)
    {
        printf("ERROR: No se pudo leer WHO_AM_I\n");
        return err;
    }

    printf("MPU6050 WHO_AM_I = 0x%02X\n", who_am_i);

    switch (who_am_i)
    {
    case 0x68:
        printf("Dispositivo identificado como MPU6050\n");
        break;

    case 0x70:
        printf("Dispositivo identificado como MPU6500 o compatible\n");
        break;

    case 0x71:
        printf("Dispositivo identificado como MPU9250 o compatible\n");
        break;

    default:
        printf("Dispositivo desconocido, WHO_AM_I = 0x%02X\n", who_am_i);
        break;
    }

    // --------------------------------------------------
    // Sacar MPU6050 del modo sleep
    // --------------------------------------------------

    err = mpu6050_write_register(
        REG_PWR_MGMT_1,
        0x00
    );

    if (err != ESP_OK)
        return err;

    vTaskDelay(pdMS_TO_TICKS(100));

    // --------------------------------------------------
    // Sample Rate
    // 1 kHz / (1 + 4) = 200 Hz
    // --------------------------------------------------

    err = mpu6050_write_register(
        REG_SMPLRT_DIV,
        4
    );

    if (err != ESP_OK)
        return err;

    // --------------------------------------------------
    // Configuración filtro
    // --------------------------------------------------

    err = mpu6050_write_register(
        REG_CONFIG,
        0x03
    );

    if (err != ESP_OK)
        return err;

    // --------------------------------------------------
    // Giroscopio ±250 °/s
    // --------------------------------------------------

    err = mpu6050_write_register(
        REG_GYRO_CONFIG,
        0x00
    );

    if (err != ESP_OK)
        return err;

    // --------------------------------------------------
    // Acelerómetro ±2 g
    // --------------------------------------------------

    err = mpu6050_write_register(
        REG_ACCEL_CONFIG,
        0x00
    );

    if (err != ESP_OK)
        return err;

    printf("MPU6050 configurado correctamente\n");

    return ESP_OK;
}

// --------------------------------------------------
// Leer acelerómetro + temperatura + giroscopio
// --------------------------------------------------

esp_err_t mpu6050_read(mpu6050_data_t *data)
{
    uint8_t buffer[14];

    esp_err_t err = mpu6050_read_registers(
        REG_ACCEL_XOUT_H,
        buffer,
        14
    );

    if (err != ESP_OK)
        return err;

    // --------------------------------------------------
    // Acelerómetro
    // --------------------------------------------------

    data->accel_x =
        ((int16_t)buffer[0] << 8) |
        buffer[1];

    data->accel_y =
        ((int16_t)buffer[2] << 8) |
        buffer[3];

    data->accel_z =
        ((int16_t)buffer[4] << 8) |
        buffer[5];

    // --------------------------------------------------
    // Temperatura
    // --------------------------------------------------

    data->temperature =
        ((int16_t)buffer[6] << 8) |
        buffer[7];

    // --------------------------------------------------
    // Giroscopio
    // --------------------------------------------------

    data->gyro_x =
        ((int16_t)buffer[8] << 8) |
        buffer[9];

    data->gyro_y =
        ((int16_t)buffer[10] << 8) |
        buffer[11];

    data->gyro_z =
        ((int16_t)buffer[12] << 8) |
        buffer[13];

    return ESP_OK;
}

// --------------------------------------------------
// Task de prueba
// --------------------------------------------------