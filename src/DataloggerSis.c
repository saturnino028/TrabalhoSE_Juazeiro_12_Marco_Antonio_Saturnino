/**
 * @brief arquivo de implementação das funções do sistema
 * 
 **/

 #include "DataloggerSis.h"
 #include "figuras_ws2812.h"
 #include "prefixos_uteis.h"
 #include "figuras_ssd1306.h"

/********************* Variaveis Globais *********************/
 //Funções da interface do usuário
ssd1306_t ssd; volatile uint8_t slice_b;
volatile uint16_t top_wrap = 1000; //Valor máximo do pwm
volatile float volume_buzzer = 2.0;

volatile uint32_t passado = 0; //Usada para implementar o debouncing
bool flag_grav_dados = 0;

def_canais_pwm dados;

dados_sensor dados_imp;

char filename[32];

/************************** Funções *************************/

/**
 * @brief inicialização do sistema
 */
void  InitSistema()
{
    bool cor = true;
    
    set_sys_clock_khz(1250000,false); //Cofigura o clock

    stdio_init_all();

    config_pins_gpio(); //Inicia os pinos GPIO

    init_matriz(); //Inicia a matriz de LEDs 5x5 WS2812

    desenhar_fig(open, 10);

    slice_b = config_pwm(buz_B, 1*KHz); //Configura um slice para 1KHz

    config_i2c_display(&ssd);

    ssd1306_fill(&ssd, !cor); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
    ssd1306_draw_string(&ssd, "  EMBARCATECH", 5, 15); // Desenha uma string
    ssd1306_draw_string(&ssd, "RESTIC 37", 26, 29); // Desenha uma string  
    ssd1306_draw_string(&ssd, "  FASE 2", 26, 43); // Desenha uma string      
    ssd1306_send_data(&ssd); // Atualiza o display

    sleep_ms(1500);
    ssd1306_fill(&ssd, !cor); // Limpa o display
    ssd1306_draw_image(&ssd, fig_principal);     
    ssd1306_send_data(&ssd); // Atualiza o display

    uint8_t flag_led = 1;
    for(uint8_t etapa = 0; etapa<6; etapa++)
    {
        gpio_put(LED_R, (flag_led & 0b00000001));
        gpio_put(LED_G, (flag_led & 0b00000010));
        gpio_put(LED_B, (flag_led & 0b00000100));
        for(int i = 0; i<500; i++)
            sleep_ms(1);
        flag_led = flag_led*2;
    }

    gpio_put(LED_B,0);
    gpio_put(LED_G,1);
    gpio_put(LED_R,1);

    ssd1306_fill(&ssd, !cor); // Limpa o display
    ssd1306_draw_string(&ssd, " Iniciando...", 8, 29); // Desenha uma string       
    ssd1306_send_data(&ssd); // Atualiza o display

    sleep_ms(1000);

    gpio_set_irq_enabled_with_callback(bot_A, GPIO_IRQ_EDGE_FALL, true, &botoes_callback);
    gpio_set_irq_enabled_with_callback(bot_B, GPIO_IRQ_EDGE_FALL, true, &botoes_callback);

    mpu6050_init(); //Inicializa o MPU6050
    verificar_cartao(); //Verifica se a montagem do cartão é bem sucedida

    desenhar_fig(apagado, 10);

    campainha(volume_buzzer, 1000,slice_b, buz_B);

    sleep_ms(1000);
}

/**
 * @brief inicia os pinos de GPIO
 */
void config_pins_gpio()
{
    //Configuração do botao A
    gpio_init(bot_A);
    gpio_pull_up(bot_A);
    gpio_set_dir(bot_A, GPIO_IN);

    //Configuração do botao B
    gpio_init(bot_B);
    gpio_pull_up(bot_B);
    gpio_set_dir(bot_B, GPIO_IN);

    //Configuração do LED vermelho
    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);

    //Configuração do LED verde
    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);

    //Configuração do LED azul
    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);
}

///Funções PWM
/**
 * @brief Configura os pinos PWM
 */
uint config_pwm(uint8_t _pin, uint16_t _freq_Hz)
{
    uint slice; float Fpwm;
    top_wrap = 1000000/_freq_Hz;
    gpio_set_function(_pin, GPIO_FUNC_PWM); //Habilita a função PWM
    slice = pwm_gpio_to_slice_num(_pin);//Obter o valor do slice correspondente ao pino
    pwm_set_clkdiv(slice, 125.0); //Define o divisor de clock
    pwm_set_wrap(slice, top_wrap); //Define valor do wrap
    Fpwm = 125000000/(125.0*top_wrap);
    printf("PWM definido para %.2f Hz\n", Fpwm);
    return slice; //Retorna o slice correspondente
}

/**
 * @brief ajusta o duty cicle
 */
void duty_cicle(float _percent, uint _slice, uint8_t _pin)
{
    pwm_set_enabled(_slice, false); //Desabilita PWM
    uint16_t valor_pwm = (_percent/100)*top_wrap; //Configura DutyCicle
    pwm_set_gpio_level(_pin, valor_pwm); //Configura DutyCicle
    pwm_set_enabled(_slice, true); //Habilitar PWM
}

/**
 * @brief função de callback para desativar a campainha
 */
int64_t fim_campainha(alarm_id_t id, void *user_data)
{
    def_canais_pwm *data = (def_canais_pwm *)user_data;
    uint _slice = data -> _slice;
    uint8_t _pin = data -> _pin;
    duty_cicle(0.0, _slice, _pin);
    return 0;
}

/**
 * @brief função para som no buzzer
 */
void campainha(float _dc, uint32_t _duracao_ms, uint _slice, uint8_t _pin)
{
    duty_cicle(_dc, _slice, _pin);
    dados._slice = _slice;
    dados._pin = _pin;
    add_alarm_in_ms(_duracao_ms, fim_campainha, &dados, false);
}

/**
 * @brief Coloca o Pico no modo gravação
 */
void modo_gravacao()
{    
    printf("Entrando no modo de gravacao...\n");
    reset_usb_boot(0, 0); 
}

/**
 * @brief trata a interrupção gerada pelos botões A e B da BitDog
 * @param gpio recebe o pino que gerou a interrupção
 * @param events recebe o evento que causou a interrupção
 */
void botoes_callback(uint gpio, uint32_t events)
{
    printf("Interrupcao");
    // Obtém o tempo atual em microssegundos
    uint32_t agora = to_us_since_boot(get_absolute_time());
    // Verifica se passou tempo suficiente desde o último evento
    if (agora - passado > 500000) // 500 ms de debouncing
    {
        if(gpio == bot_A)
            flag_grav_dados = 1;
        else if(gpio == bot_B)
            modo_gravacao();
        passado  = agora;
    }
}

/**
 * @brief verificar se existe algum cartão pronto para uso
 */
void verificar_cartao()
{
    run_mount();

    if(cartao_pronto())
    {
        printf("Cartão pronto para uso.\n");
        run_unmount();
    }
    else
    {
        bool flag_led = 0;

        printf("Não foi possível montar o cartão. Verifique e tente novamente.\n");
        gpio_put(LED_B, flag_led);
        gpio_put(LED_G, flag_led);

        ssd1306_fill(&ssd, false); // Limpa o display
        ssd1306_draw_string(&ssd, "Erro: Cartao", 10, 29); // Desenha uma string       
        ssd1306_send_data(&ssd); // Atualiza o display

        while (1)
        {
            flag_led = !flag_led;
            gpio_put(LED_R, flag_led);
            sleep_ms(200);
        }
    }
}

/**
 * @brief função para dar inicio a gravação de dados
 */
void gravar_dados()
{
    bool cor = true;

    gpio_put(LED_B,0);
    gpio_put(LED_G,0);
    gpio_put(LED_R,1);

    run_mount();

    if(gerar_proximo_nome_csv(filename, sizeof(filename)))
    {
        printf("Próximo arquivo: %s\n", filename);

        ssd1306_fill(&ssd, !cor); // Limpa o display
        ssd1306_draw_string(&ssd, "Gravando Dados", 8, 29); // Desenha uma string      
        ssd1306_send_data(&ssd); // Atualiza o display

        criar_csv_dados_sensor(filename);
    }
    else
    {
        bool flag_led = 0;
        ssd1306_fill(&ssd, !cor); // Limpa o display
        ssd1306_draw_string(&ssd, "Erro Gravação", 8, 29); // Desenha uma string      
        ssd1306_send_data(&ssd); // Atualiza o display
        while (1)
        {
            flag_led = !flag_led;
            gpio_put(LED_R, flag_led);
            sleep_ms(200);
        }
    }
    run_unmount();
}

void sistema_livre()
{
    gpio_put(LED_B,0);
    gpio_put(LED_G,1);
    gpio_put(LED_R,0);

    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_draw_string(&ssd, "Aperte A -Grave", 5, 29); // Desenha uma string      
    ssd1306_send_data(&ssd); // Atualiza o display

    int cRxedChar = getchar_timeout_us(0);
    if (PICO_ERROR_TIMEOUT != cRxedChar)
        process_stdio(cRxedChar);
    if (cRxedChar == 'a') // Monta o SD card se pressionar 'a'
    {
        printf("\nMontando o SD...\n");
        run_mount();
        printf("\nEscolha o comando (h = help):  ");
    }
    if (cRxedChar == 'b') // Desmonta o SD card se pressionar 'b'
    {
        printf("\nDesmontando o SD. Aguarde...\n");
        run_unmount();
        printf("\nEscolha o comando (h = help):  ");
    }
    if (cRxedChar == 'c') // Lista diretórios e os arquivos se pressionar 'c'
    {
        printf("\nListagem de arquivos no cartão SD.\n");
        run_ls();
        printf("\nListagem concluída.\n");
        printf("\nEscolha o comando (h = help):  ");
    }
    if (cRxedChar == 'd') // Exibe o conteúdo do arquivo se pressionar 'd'
    {
        read_file(filename);
        printf("Escolha o comando (h = help):  ");
    }
    if (cRxedChar == 'e') // Obtém o espaço livre no SD card se pressionar 'e'
    {
        printf("\nObtendo espaço livre no SD.\n\n");
        run_getfree();
        printf("\nEspaço livre obtido.\n");
        printf("\nEscolha o comando (h = help):  ");
    }
    if (cRxedChar == 'f') // Captura dados do ADC e salva no arquivo se pressionar 'f'
    {
        //capture_adc_data_and_save();
        printf("\nEscolha o comando (h = help):  ");
    }
    if (cRxedChar == 'g') // Formata o SD card se pressionar 'g'
    {
        printf("\nProcesso de formatação do SD iniciado. Aguarde...\n");
        run_format();
        printf("\nFormatação concluída.\n\n");
        printf("\nEscolha o comando (h = help):  ");
    }
    if (cRxedChar == 'h') // Exibe os comandos disponíveis se pressionar 'h'
    {
        run_help();
    }
}

/**
 * @brief gera o nome do próximo arquivo
 */
bool gerar_proximo_nome_csv(char *buffer, size_t tamanho)
{
    DIR dir;
    FILINFO fno;
    FRESULT fr;
    int max_index = -1;

    // Abre o diretório raiz
    fr = f_opendir(&dir, "/");
    if (fr != FR_OK) {
        printf("Erro ao abrir diretório: %d\n", fr);
        return false;
    }

    // Percorre todos os arquivos do diretório
    while (true) {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0)
            break;

        // Verifica se o nome do arquivo começa com "dados" e termina com ".csv"
        if (strncmp(fno.fname, "dados", 5) == 0 && strstr(fno.fname, ".csv")) {
            // Tenta extrair o número X
            int x;
            if (sscanf(fno.fname, "dados%d.csv", &x) == 1) {
                if (x > max_index)
                    max_index = x;
            }
        }
    }

    f_closedir(&dir);

    // Gera o nome com o próximo índice
    snprintf(buffer, tamanho, "dados%d.csv", max_index + 1);
    return true;
}

/**
 * @brief gerar CSV
 */
void criar_csv_dados_sensor(char *_nome_do_arquivo)
{
    FIL file;
    FRESULT fr;
    UINT bytes_written;

    // Nome do arquivo
    const char *filename = "dados.csv";

    // Abre o arquivo para escrita (cria se não existir, sobrescreve se existir)
    fr = f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        printf("Erro ao criar arquivo CSV: %d\n", fr);
        return;
    }

    // Cabeçalho do CSV
    const char *cabecalho = "Index,Acel_X,Acel_Y,Acel_Z,Giro_X,Giro_Y,Giro_Z,Temp\n";

    // Escreve cabeçalho
    fr = f_write(&file, cabecalho, strlen(cabecalho), &bytes_written);
    if (fr != FR_OK || bytes_written != strlen(cabecalho)) {
        printf("Erro ao escrever cabeçalho no arquivo: %d\n", fr);
        f_close(&file);
        return;
    }

    // Exemplo: escreve 100 linhas de dados
    for (int i = 0; i < 100; i++) 
    {
        mpu6050_read_raw(dados_imp.aceleracao, dados_imp.giro, &dados_imp.temp);
        char linha[100];
        float ax = dados_imp.aceleracao[0] / 16384.0f;
        float ay = dados_imp.aceleracao[1] / 16384.0f;
        float az = dados_imp.aceleracao[2] / 16384.0f;
        float gx = dados_imp.giro[0] / 16384.0f;
        float gy = dados_imp.giro[1] / 16384.0f;
        float gz = dados_imp.giro[2] / 16384.0f;
        float temp = (dados_imp.temp/340.0)+36.53;

        // Formata a linha como CSV
        snprintf(linha, sizeof(linha), "%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                 i, ax, ay, az, gx, gy, gz, temp);

        // Escreve a linha no arquivo
        fr = f_write(&file, linha, strlen(linha), &bytes_written);
        if (fr != FR_OK) {
            printf("Erro ao escrever dados no CSV: %d\n", fr);
            break;
        }
        sleep_ms(50);
    }

    f_close(&file);
    printf("Arquivo CSV criado com sucesso: %s\n", filename);
}