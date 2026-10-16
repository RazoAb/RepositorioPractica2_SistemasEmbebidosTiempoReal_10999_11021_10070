#include "distance.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

static const char *TAG = "DISTANCE";

#define TRIGGER_PULSE_US        10      // pulso mínimo de disparo según datasheet          
#define ECHO_IDLE_TIMEOUT_US    250000   // si Echo sigue en alto de una medición anterior   
#define ECHO_START_TIMEOUT_US   5000    // tiempo máx. para que Echo suba tras el disparo   
#define ECHO_PULSE_TIMEOUT_US   30000   // ancho máx. del pulso (~5 m); sin eco ~38 ms      
#define ECHO_MIN_PULSE_US       100     // menos que esto (~1.7 cm) es ruido                
#define US_PER_CM               58      // ida y vuelta: 2 / 0.0343 cm/us ≈ 58 us/cm        

esp_err_t dis_init(dis_sensor_t *sensor, int trigger_pin, int echo_pin)
{
    if (sensor == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    sensor->trigger_pin = trigger_pin;
    sensor->echo_pin    = echo_pin;

    gpio_config_t trig_cfg = {
        .pin_bit_mask = 1ULL << trigger_pin,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&trig_cfg);
    if (err != ESP_OK) {
        return err;
    }

    gpio_config_t echo_cfg = {
        .pin_bit_mask = 1ULL << echo_pin,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,   // evita lecturas flotantes si no está conectado
        .intr_type    = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&echo_cfg);
    if (err != ESP_OK) {
        return err;
    }

    gpio_set_level(trigger_pin, 0);
    ESP_LOGI(TAG, "HC-SR04 listo (TRIG=%d, ECHO=%d)", trigger_pin, echo_pin);
    return ESP_OK;
}

esp_err_t dis_read_pulse_us(const dis_sensor_t *sensor, uint32_t *pulse_us)
{
    if (sensor == NULL || pulse_us == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* 0) Echo debe estar en bajo antes de disparar (si quedó en alto de una
     *    medición anterior, se espera a que baje; si no baja, algo está mal) */
    int64_t t0 = esp_timer_get_time();
    while (gpio_get_level(sensor->echo_pin) == 1) {
        if ((esp_timer_get_time() - t0) > ECHO_IDLE_TIMEOUT_US) {
            return ESP_ERR_INVALID_STATE;
        }
    }

    //Pulso de disparo de 10 us 
    gpio_set_level(sensor->trigger_pin, 0);
    esp_rom_delay_us(2);
    gpio_set_level(sensor->trigger_pin, 1);
    esp_rom_delay_us(TRIGGER_PULSE_US);
    gpio_set_level(sensor->trigger_pin, 0);

    //Esperar flanco de subida de Echo 
    int64_t t_start = esp_timer_get_time();
    while (gpio_get_level(sensor->echo_pin) == 0) {
        if ((esp_timer_get_time() - t_start) > ECHO_START_TIMEOUT_US) {
            return ESP_ERR_NOT_FOUND;       /* el sensor no responde */
        }
    }

    //Medir cuánto dura Echo en alto 
    int64_t t_rise = esp_timer_get_time();
    while (gpio_get_level(sensor->echo_pin) == 1) {
        if ((esp_timer_get_time() - t_rise) > ECHO_PULSE_TIMEOUT_US) {
            return ESP_ERR_TIMEOUT;         /* sin objeto en rango */
        }
    }
    int64_t width = esp_timer_get_time() - t_rise;

    if (width < ECHO_MIN_PULSE_US) {
        return ESP_ERR_INVALID_RESPONSE;    /* pulso demasiado corto: ruido */
    }

    *pulse_us = (uint32_t)width;
    return ESP_OK;
}

esp_err_t dis_read_cm(const dis_sensor_t *sensor, uint16_t *distance_cm)
{
    if (distance_cm == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t pulse_us = 0;
    esp_err_t err = dis_read_pulse_us(sensor, &pulse_us);
    if (err != ESP_OK) {
        return err;
    }

    int32_t cm = (int32_t)(pulse_us / US_PER_CM);
    if (cm < DISTANCE_MIN_CM) cm = DISTANCE_MIN_CM;
    if (cm > DISTANCE_MAX_CM) cm = DISTANCE_MAX_CM;

    *distance_cm = (uint16_t)cm;
    return ESP_OK;
}