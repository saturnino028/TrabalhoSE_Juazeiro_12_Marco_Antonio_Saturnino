/**
 * @brief arquivo de cabeçalho das funções de memória - SSDCARD
 */

 #ifndef Cartao_FatFS_SPI_H
 #define Cartao_FatFS_SPI_H

/********************* Includes *********************/
 #include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hardware/rtc.h"
#include "pico/stdlib.h"

#include "ff.h"
#include "diskio.h"
#include "f_util.h"
#include "hw_config.h"
#include "my_debug.h"
#include "rtc.h"
#include "sd_card.h"

/********************* Defines *********************/

/********************* Variaveis Globais *********************/

/********************* Prototipo de Funções *********************/
sd_card_t *sd_get_by_name(const char *const name);
FATFS *sd_get_fs_by_name(const char *name);
void run_setrtc();
void run_format();
void run_mount();
void run_unmount();
void run_getfree();
void run_ls();
void run_cat();
void read_file(const char *filename);
void run_help();

void process_stdio(int cRxedChar);

void listar_cartoes_disponiveis();
bool cartao_pronto();
void verificar_cartao();

 #endif //Cartao_FatFS_SPI_H