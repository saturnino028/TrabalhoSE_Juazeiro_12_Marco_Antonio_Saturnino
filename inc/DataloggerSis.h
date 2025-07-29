/***
 * @brief cabelho principal
 * 
 */

 #ifndef MAIN_H
 #define MAIN_H

/********************* Includes *********************/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "hardware/clocks.h"

#include "pico/bootrom.h"

#include "pinout.h"
#include "ssd1306.h"
#include "mpu6050.h"
#include "matriz_5x5.h"
#include "Cartao_FatFS_SPI.h"

/********************* Defines e structs *********************/
typedef struct 
{
    uint _slice;
    uint8_t _pin;
} def_canais_pwm;

typedef struct
{
    int16_t aceleracao[3];
    int16_t giro[3];
    int16_t temp;
} dados_sensor;

/********************* Variaveis Globais *********************/
extern bool flag_grav_dados;

extern ssd1306_t ssd;

/************************** Funções *************************/
void  InitSistema();

void config_pins_gpio();
uint config_pwm(uint8_t _pin, uint16_t _freq_Hz);

void campainha(float _dc, uint32_t _duracao_ms, uint _slice, uint8_t _pin);
void duty_cicle(float _percent, uint _slice, uint8_t _pin);
int64_t fim_campainha(alarm_id_t id, void *user_data);

void botoes_callback(uint gpio, uint32_t events);
void modo_gravacao();

void verificar_cartao();
void gravar_dados();
void sistema_livre();
bool gerar_proximo_nome_csv(char *buffer, size_t tamanho);
void criar_csv_dados_sensor(char *_nome_do_arquivo);

 #endif //MAIN_H