// SE DECALRAN LAS BIBLIOTECAS Y ARCHIVOS REQUERIDOS
// Bibliotecas estándar de C
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
// Librerías específicas del proyecto
#include "clock.h"
#include "console.h"
#include "sdram.h"
#include "lcd-spi.h"
#include "gfx.h"
#include "rcc.h"
#include "adc.h"
#include "dac.h"
#include "gpio.h"
#include "spi.h"
#include "usart.h"
// // Librerías del microcontrolador
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>



// Definiciones relacionadas con el giroscopio
#define GYR_RNW			(1 << 7) /* Permite escribir cuando es cero */
#define GYR_MNS			(1 << 6) /* Habilita lecturas múltiples cuando es 1 */
#define GYR_WHO_AM_I		0x0F    // Registro que identifica el dispositivo
#define GYR_OUT_TEMP		0x26    // Registro de temperatura de salida
#define GYR_STATUS_REG		0x27    // Registro de estado

// Definiciones para configurar el giroscopio
#define GYR_CTRL_REG1		0x20    // Registro de control 1
#define GYR_CTRL_REG1_PD	(1 << 3) // Modo de encendido
#define GYR_CTRL_REG1_XEN	(1 << 1) // Habilitar eje X
#define GYR_CTRL_REG1_YEN	(1 << 0) // Habilitar eje Y
#define GYR_CTRL_REG1_ZEN	(1 << 2) // Habilitar eje Z
#define GYR_CTRL_REG1_BW_SHIFT	4    // Cambio de ancho de banda
#define GYR_CTRL_REG4		0x23    // Registro de control 4
#define GYR_CTRL_REG4_FS_SHIFT	4    // Cambio de escala completa

// Direcciones de registros de datos de giroscopio
#define GYR_OUT_X_L		0x28
#define GYR_OUT_X_H		0x29
#define GYR_OUT_Y_L		0x2A
#define GYR_OUT_Y_H		0x2B
#define GYR_OUT_Z_L		0x2C
#define GYR_OUT_Z_H		0x2D

#define L3GD20_SENSITIVITY_250DPS  (0.00875F)  // Sensibilidad del giroscopio

// Estructura para almacenar las lecturas de los ejes X, Y, y Z del giroscopio
typedef struct Gyro {
  int16_t x;
  int16_t y;
  int16_t z;
} gyro;

// Función para realizar una comunicación SPI y obtener una respuesta
uint8_t spi_communication(uint8_t command) {
    gpio_clear(GPIOC, GPIO1);          // Desactiva el pin GPIO1 de GPIOC (probablemente CS o Chip Select)
    spi_send(SPI5, command);           // Envia el comando por SPI5
    spi_read(SPI5);                    // Lee una respuesta desde SPI5, aunque esta respuesta no se usa
    spi_send(SPI5, 0);                 // Envía un byte en 0 por SPI5
    uint8_t result = spi_read(SPI5);   // Lee el resultado desde SPI5
    gpio_set(GPIOC, GPIO1);            // Activa el pin GPIO1 de GPIOC (termina la comunicación con el dispositivo)
    return result;                     // Devuelve el resultado leído
}

// Función para leer un eje del giroscopio, combinando el byte menos significativo (LSB) y el más significativo (MSB)
int16_t read_axis(uint8_t lsb_command, uint8_t msb_command) {
    int16_t result;
    result = spi_communication(lsb_command);                // Lee el byte menos significativo
    result |= spi_communication(msb_command) << 8;          // Lee el byte más significativo y lo combina con el LSB
    return result;                                          // Devuelve el valor combinado
}

// Función para leer los valores de los ejes X, Y, Z del giroscopio
gyro read_xyz(void) {
    gyro lectura;

    spi_communication(GYR_WHO_AM_I | 0x80);                 // Lee el registro WHO_AM_I
    spi_communication(GYR_STATUS_REG | GYR_RNW);            // Lee el registro STATUS_REG
    spi_communication(GYR_OUT_TEMP | GYR_RNW);              // Lee el registro OUT_TEMP

    // Lee y escala los valores de los ejes
    lectura.x = read_axis(GYR_OUT_X_L | GYR_RNW, GYR_OUT_X_H | GYR_RNW) * L3GD20_SENSITIVITY_250DPS;
    lectura.y = read_axis(GYR_OUT_Y_L | GYR_RNW, GYR_OUT_Y_H | GYR_RNW) * L3GD20_SENSITIVITY_250DPS;
    lectura.z = read_axis(GYR_OUT_Z_L | GYR_RNW, GYR_OUT_Z_H | GYR_RNW) * L3GD20_SENSITIVITY_250DPS;

    return lectura; // Devuelve la lectura de los 3 ejes
}

// Función para configurar algunos pines GPIO
static void gpio_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOG);                     // Habilita el reloj para el puerto GPIOG
    rcc_periph_clock_enable(RCC_GPIOA);                     // Habilita el reloj para el puerto GPIOA

    // Configura el pin GPIO0 de GPIOA como entrada
    gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);
    // Configura el pin GPIO13 de GPIOG como salida
    gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
    // Configura el pin GPIO14 de GPIOG como salida
    gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO14);
}