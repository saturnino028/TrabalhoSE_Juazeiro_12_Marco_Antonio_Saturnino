/**
 * @brief arquivo de cabeçalho das funções do sensor MPU6050
 */
#ifndef MPU6050_H
#define MPU6050_H

/********************* Includes *********************/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "pinout.h"

/********************* Defines *********************/
#define I2C_PORT0 i2c0

/********************* Variaveis Globais *********************/


/********************* Prototipo de Funções *********************/

void mpu6050_init();
void mpu6050_reset();
void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp);


#endif //MPU6050_H