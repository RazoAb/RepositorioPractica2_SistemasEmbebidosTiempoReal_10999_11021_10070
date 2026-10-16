#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "queues.h"
#include "app_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "data_types.h"
#include "driver/spi_master.h"

#include "max7219.h"

// --------------------------------------------------
// Rango de distancia que se muestra en la matriz (cm)
// Cerca de DIST_MAP_MIN_CM -> 8 filas encendidas
// Lejos  de DIST_MAP_MAX_CM -> 1 fila encendida
// (ajusta estos valores a tu aplicación)
// --------------------------------------------------
#define DIST_MAP_MIN_CM   2
#define DIST_MAP_MAX_CM   20

// Rango para el mapeo a dígito 0..9 (9 = cerca, 0 = lejos)
#define DIGIT_MAP_MIN_CM  2
#define DIGIT_MAP_MAX_CM  20

// --------------------------------------------------
// Fuente 5x7 para números 0-9
// --------------------------------------------------

static const uint8_t DIGIT[10][8] = {

    // 0
    {0x70, 0x88, 0x98, 0xA8,
     0xC8, 0x88, 0x70, 0x00},

    // 1
    {0x20, 0x60, 0x20, 0x20,
     0x20, 0x20, 0x70, 0x00},

    // 2
    {0x70, 0x88, 0x08, 0x10,
     0x20, 0x40, 0xF8, 0x00},

    // 3
    {0xF0, 0x08, 0x08, 0x70,
     0x08, 0x08, 0xF0, 0x00},

    // 4
    {0x10, 0x30, 0x50, 0x90,
     0xF8, 0x10, 0x10, 0x00},

    // 5
    {0xF8, 0x80, 0x80, 0xF0,
     0x08, 0x08, 0xF0, 0x00},

    // 6
    {0x70, 0x80, 0x80, 0xF0,
     0x88, 0x88, 0x70, 0x00},

    // 7
    {0xF8, 0x08, 0x10, 0x20,
     0x40, 0x40, 0x40, 0x00},

    // 8
    {0x70, 0x88, 0x88, 0x70,
     0x88, 0x88, 0x70, 0x00},

    // 9
    {0x70, 0x88, 0x88, 0x78,
     0x08, 0x08, 0x70, 0x00}
};

static void draw_digit(uint8_t matrix[8], int digit)
{
    if (digit < 0 || digit > 9) {
        for (int row = 0; row < 8; row++) {
            matrix[row] = 0x00;
        }
        return;
    }

    for (int row = 0; row < 8; row++) {
        matrix[row] = DIGIT[digit][row];
    }
}


// MODO 1: Horizonte artificial con roll y pitch
static void draw_horizon(uint8_t matrix[8], float roll, float pitch)
{
    // Limitar roll entre -45 y +45 grados
    if (roll > 45.0f)
        roll = 45.0f;

    if (roll < -45.0f)
        roll = -45.0f;

    // Limitar pitch entre -45 y +45 grados
    if (pitch > 45.0f)
        pitch = 45.0f;

    if (pitch < -45.0f)
        pitch = -45.0f;

    // Convertir roll a radianes
    float angle = roll * M_PI / 180.0f;

    // Centro vertical de la matriz
    const float center = 3.5f;

    // Limpiar matriz
    for (int i = 0; i < 8; i++)
        matrix[i] = 0x00;

    // Calcular inclinación y desplazamiento
    float slope = tanf(angle);
    float pitch_offset = -(pitch / 45.0f) * 3.5f;

    for (int x = 0; x < 8; x++)
    {
        float y = center
                  + pitch_offset
                  + slope * (x - center);

        int row = (int)roundf(y);

        // Evitar dibujar fuera de la matriz
        if (row >= 0 && row < 8)
        {
            matrix[row] |= (1 << (7 - x));
        }
    }
}
// TERMINA MODO 1

//MODO 2: PITCH BARRA BIPOLAR

static void draw_pitch_bar(uint8_t matrix[8], float pitch)
{
    // Limitar el pitch al rango de -45 a +45 grados
    if (pitch > 45.0f)  pitch = 45.0f;
    if (pitch < -45.0f) pitch = -45.0f;

    // Apagar todos los LED inicialmente
    for (int i = 0; i < 8; i++) {
        matrix[i] = 0x00;
    }

    // Línea horizontal de referencia en el centro
    matrix[3] = 0xFF;

    // Calcular cuántas filas adicionales encender
    int levels = (int)roundf(fabsf(pitch) / 45.0f * 4.0f);

    if (pitch > 0.0f) {
        // Pitch positivo: encender filas hacia arriba
        for (int row = 2; row >= 3 - levels; row--) {
            if (row >= 0) {
                matrix[row] = 0xFF;
            }
        }
    }
    else if (pitch < 0.0f) {
        // Pitch negativo: encender filas hacia abajo
        for (int row = 4; row <= 3 + levels; row++) {
            if (row < 8) {
                matrix[row] = 0xFF;
            }
        }
    }
}

//TERMINA MODO 2


// --------------------------------------------------
// MAPEO DE DISTANCIA
// --------------------------------------------------

// Mapeo lineal x: [in_min, in_max] -> [out_min, out_max], saturado.
// out_min puede ser mayor que out_max (mapeo invertido).
static int map_range(int x, int in_min, int in_max, int out_min, int out_max)
{
    if (x < in_min) x = in_min;
    if (x > in_max) x = in_max;

    return out_min + (int)(((int32_t)(x - in_min) * (out_max - out_min))
                           / (in_max - in_min));
}

// Convierte la distancia (cm) en número de filas encendidas (1..8).
// Cerca -> 8 filas, lejos -> 1 fila (nunca queda la matriz en blanco).
static int map_distance_to_levels(uint16_t distance_cm)
{
    return map_range((int)distance_cm,
                     DIST_MAP_MIN_CM, DIST_MAP_MAX_CM,
                     8, 1);
}

// Convierte la distancia (cm) en un número 0..9.
// 9 = cerca (2 cm), 0 = lejos (20 cm o más). Fuera del rango se satura.
int map_distance_to_digit(uint16_t distance_cm)
{
    return map_range((int)distance_cm,
                     DIGIT_MAP_MIN_CM, DIGIT_MAP_MAX_CM,
                     9, 0);
}

// Barra de proximidad: se llena de abajo hacia arriba (fila 7 = abajo).
static void draw_distance_bar(uint8_t matrix[8], uint16_t distance_cm)
{
    int levels = map_distance_to_levels(distance_cm);

    for (int i = 0; i < 8; i++) {
        matrix[i] = 0x00;
    }

    for (int row = 7; row > 7 - levels; row--) {
        matrix[row] = 0xFF;
    }
}


// --------------------------------------------------
// Objeto del display
// --------------------------------------------------

static max7219_t display;


// --------------------------------------------------
// Inicialización del display
// --------------------------------------------------

void display_init(void)
{
    max7219_init(
        &display,
        SPI2_HOST,
        PIN_MOSI,
        PIN_CLK,
        PIN_CS,
        4
    );

    printf("MAX7219 inicializado\n");
}


// --------------------------------------------------
// Tarea del display
// --------------------------------------------------

// Tarea del display
void display_task(void *pvParameters)
{
    display_data_t state;

    while (1)
    {
        // Recibir el estado completo desde TaskDataManager
        if (xQueueReceive(g_displayQueue,
                          &state,
                          portMAX_DELAY) == pdTRUE)
        {
            // Si la pantalla está congelada, no actualizar
            if (state.freeze)
            {
                continue;
            }

            // Preparar las cuatro matrices
            uint8_t imu_display[8] = {0};
            uint8_t temp_tens[8] = {0};
            uint8_t temp_units[8] = {0};
            uint8_t distance_display[8] = {0};

            // Obtener temperatura y limitarla a dos dígitos
            int temperature = (int)state.temperature;

            if (temperature < 0)
                temperature = 0;

            if (temperature > 99)
                temperature = 99;

            // Matrices 2 y 3: temperatura
            draw_digit(temp_tens, temperature / 10);
            draw_digit(temp_units, temperature % 10);

            // Matriz 4: seleccionar representación del MPU6050
            if (state.display_mode == DISPLAY_MODE_GRAPHIC)
            {
                // Modo 1: horizonte artificial con roll y pitch
                draw_horizon(imu_display, state.roll, state.pitch);
                draw_distance_bar(distance_display, state.distance_cm);
            }
            else
            {
                // Modo 2: barra bipolar de pitch
                draw_pitch_bar(imu_display, state.pitch);
                draw_digit(distance_display, map_distance_to_digit(state.distance_cm));
            }
            

            // Actualizar las cuatro matrices MAX7219
            max7219_write_rows(&display,
                               imu_display,
                               temp_units,
                               temp_tens,
                               distance_display);
        }
    }
}