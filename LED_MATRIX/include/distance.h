#ifndef DISTANCE_H
#define DISTANCE_H

#include <stdint.h>
#include "esp_err.h"

/// rango
#define DISTANCE_MIN_CM     2
#define DISTANCE_MAX_CM     400

typedef struct dis_sensor {
    int trigger_pin;
    int echo_pin;
} dis_sensor_t;

// Configura los GPIO del sensor (Trigger = salida, Echo = entrada) 
esp_err_t dis_init(dis_sensor_t *sensor, int trigger_pin, int echo_pin);

/* Dispara una medición y devuelve el ancho del pulso Echo en microsegundos.
 *
 * ESP_OK                    medición válida
 * ESP_ERR_INVALID_STATE     Echo ya estaba en alto y no bajó (cable/corto/pin equivocado)
 * ESP_ERR_NOT_FOUND         Echo nunca subió: el sensor no responde (alimentación a 5 V, cableado, pines)
 * ESP_ERR_TIMEOUT           Echo se quedó en alto: no hay objeto dentro del rango
 * ESP_ERR_INVALID_RESPONSE  pulso demasiado corto (ruido / falso contacto)
 */
esp_err_t dis_read_pulse_us(const dis_sensor_t *sensor, uint32_t *pulse_us);

//convierte a cm
esp_err_t dis_read_cm(const dis_sensor_t *sensor, uint16_t *distance_cm);

#endif