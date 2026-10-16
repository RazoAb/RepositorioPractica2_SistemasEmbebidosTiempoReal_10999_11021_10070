#include "data_types.h"
#include "buttons.h"
#include "freertos/queue.h"
#include "queues.h"

void button_init(gpio_num_t pin)
{
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(pin, GPIO_INTR_POSEDGE);
}

#define ESP_INTR_FLAG_DEFAULT 0

// 1. ISR optimizada (Permanecerá en IRAM)
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t)(uintptr_t) arg;
    command_t command;
    
    switch (gpio_num)
    {
        case BUTTON_FREEZE:
            command = CMD_FREEZE;
            break;
        case BUTTON_MODE:
            command = CMD_CHANGE_MODE;
            break;
        case BUTTON_LOAD:
            command = CMD_OVERLOAD;
            break;
        
        default:
            break;
    }
    xQueueSendFromISR(g_commandQueue, &command, NULL);
}



void button_config(void)
{
    // Configuración inicial del GPIO utilizando tu estructura gpio_config_t
   //gpio_config(&io_conf);

    button_init(BUTTON_FREEZE);
    button_init(BUTTON_LOAD);
    button_init(BUTTON_MODE);


    // Inicializar cola y hilos
    //gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //xTaskCreate(button_task, "button_task", 3072, NULL, 10, NULL);

    // Registrar servicio e instalar el manejador específico
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(BUTTON_FREEZE, gpio_isr_handler, (void*)(uintptr_t) BUTTON_FREEZE);
    gpio_isr_handler_add(BUTTON_LOAD, gpio_isr_handler, (void*)(uintptr_t) BUTTON_LOAD);
    gpio_isr_handler_add(BUTTON_MODE, gpio_isr_handler, (void*)(uintptr_t) BUTTON_MODE);

    //ESP_LOGI(TAG, "Sistema listo. Presiona el botón conectado al GPIO4.");

    // [Opcional] Descomenta la línea de abajo si quieres probar cómo encender/apagar el pin en caliente
    // modificar_interrupcion_dinamicamente();
}


/*void vTaskButtons(void *pvParameters)
{
    button_config();
    //hace falta queuesInit()
    command_t command;
    while(1)
    {
        if(xQueueReceiveFromISR(g_commandQueue, &command, NULL) == pdTRUE)
        {
            switch (command)
            {
            case CMD_FREEZE:
                printf("FREEZE\n");
                break;
            case CMD_CHANGE_MODE:
                printf("MODE\n");
                break;
            case CMD_OVERLOAD:
                printf("LOAD\n");
                break;
            
            default:
                break;
            }
            xQueueSend();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}*/