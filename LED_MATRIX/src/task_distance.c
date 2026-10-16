#include "tasks.h"
#include "distance.h"
#include "app_config.h"
#include "esp_log.h"

static const char *TAG = "T_DIST";

//Periodo de muestreo según el modo: 5 muestras/s NORMAL, 20 muestras/s HIGH LOAD 
#define PERIOD_NORMAL_MS   200
#define PERIOD_LOAD_MS     50

void vTaskDistance(void *pvParameters)
{
    sensor_task_params_t *params = (sensor_task_params_t *)pvParameters;

    if (params == NULL || params->sensor_queue == NULL || params->acq_mode == NULL) {
        ESP_LOGE(TAG, "Parámetros inválidos, tarea detenida");
        vTaskDelete(NULL);
    }

    dis_sensor_t sensor;
    if (dis_init(&sensor, DISTANCE_TRIG_PIN, DISTANCE_ECHO_PIN) != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo inicializar el HC-SR04, tarea detenida");
        vTaskDelete(NULL);
    }

    TickType_t last_wake = xTaskGetTickCount();
    bool sensor_ok = true;      // para avisar por UART solo cuando cambia el estado

    while (1)
    {
        // Se lee el sensor 
        uint16_t cm = DISTANCE_MAX_CM;
        esp_err_t err = dis_read_cm(&sensor, &cm);

        if (err == ESP_ERR_TIMEOUT) {       //sin objeto en rango 
            cm  = DISTANCE_MAX_CM;
            err = ESP_OK;
        }

        if (err == ESP_OK) {
            if (!sensor_ok) {
                sensor_ok = true;
                ESP_LOGI(TAG, "HC-SR04 recuperado");
            }

            //  Se arma el mensaje y envia a SensorQueue
            sensor_message_t msg = {0};
            msg.type                      = SENSOR_TYPE_DISTANCE;
            msg.data.distance.distance_cm = cm;
            msg.timestamp                 = xTaskGetTickCount() * portTICK_PERIOD_MS;

            xQueueSend(params->sensor_queue, &msg, 0);
        }
        else if (sensor_ok) {
            // Falla real (no responde / ruido / Echo atorado)
            sensor_ok = false;
            ESP_LOGW(TAG, "HC-SR04 con falla: %s", esp_err_to_name(err));
        }

        // se espera al siguiente ciclo
        uint32_t period_ms = (*params->acq_mode == ACQ_MODE_NORMAL) ? PERIOD_NORMAL_MS
                                                                    : PERIOD_LOAD_MS;
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(period_ms));
    }
}