#include <stdio.h>
#include "pico/stdlib.h"
#include "ws2818b.pio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/bootrom.h"

//define o LED de saída
#define GPIO_LED 18

// Definição do número de LEDs e pino.
#define LED_COUNT 25
#define LED_PIN 7

//define os pinos do teclado com as portas GPIO
uint columns[4] = {4, 3, 2, 1}; 
uint rows[4] = {8, 7, 6, 5};

//mapa de teclas
char KEY_MAP[16] = {
    '1', '2' , '3', 'A',
    '4', '5' , '6', 'B',
    '7', '8' , '9', 'C',
    '*', '0' , '#', 'D'
};

// Definição de pixel GRB
struct pixel_t {
  uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[LED_COUNT];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;

/**
 * Inicializa a máquina PIO para controle da matriz de LEDs.
 */
void npInit(uint pin) {

  // Cria programa PIO.
  uint offset = pio_add_program(pio0, &ws2818b_program);
  np_pio = pio0;

  // Toma posse de uma máquina PIO.
  sm = pio_claim_unused_sm(np_pio, false);
  if (sm < 0) {
    np_pio = pio1;
    sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
  }

  // Inicia programa na máquina PIO obtida.
  ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

  // Limpa buffer de pixels.
  for (uint i = 0; i < LED_COUNT; ++i) {
    leds[i].R = 0;
    leds[i].G = 0;
    leds[i].B = 0;
  }
}

/**
 * Atribui uma cor RGB a um LED.
 */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
  leds[index].R = r;
  leds[index].G = g;
  leds[index].B = b;
}

/**
 * Limpa o buffer de pixels.
 */
void npClear() {
  for (uint i = 0; i < LED_COUNT; ++i)
    npSetLED(i, 0, 0, 0);
}

/**
 * Escreve os dados do buffer nos LEDs.
 */
void npWrite() {
  // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
  for (uint i = 0; i < LED_COUNT; ++i) {
    pio_sm_put_blocking(np_pio, sm, leds[i].G);
    pio_sm_put_blocking(np_pio, sm, leds[i].R);
    pio_sm_put_blocking(np_pio, sm, leds[i].B);
  }
  sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}



uint _columns[4];
uint _rows[4];
char _matrix_values[16];
uint all_columns_mask = 0x0;
uint column_mask[4];

//imprimir valor binário
void imprimir_binario(int num) {
 int i;
 for (i = 31; i >= 0; i--) {
  (num & (1 << i)) ? printf("1") : printf("0");
 }
}

//inicializa o keypad
void pico_keypad_init(uint columns[4], uint rows[4], char matrix_values[16]) {

    for (int i = 0; i < 16; i++) {
        _matrix_values[i] = matrix_values[i];
    }

    for (int i = 0; i < 4; i++) {

        _columns[i] = columns[i];
        _rows[i] = rows[i];

        gpio_init(_columns[i]);
        gpio_init(_rows[i]);

        gpio_set_dir(_columns[i], GPIO_IN);
        gpio_set_dir(_rows[i], GPIO_OUT);

        gpio_put(_rows[i], 1);

        all_columns_mask = all_columns_mask + (1 << _columns[i]);
        column_mask[i] = 1 << _columns[i];
    }
}

//coleta o caracter pressionado
char pico_keypad_get_key(void) {
    int row;
    uint32_t cols;
    bool pressed = false;

    cols = gpio_get_all();
    cols = cols & all_columns_mask;
    imprimir_binario(cols);
    
    if (cols == 0x0) {
        return 0;
    }

    for (int j = 0; j < 4; j++) {
        gpio_put(_rows[j], 0);
    }

    for (row = 0; row < 4; row++) {

        gpio_put(_rows[row], 1);

        busy_wait_us(10000);

        cols = gpio_get_all();
        gpio_put(_rows[row], 0);
        cols = cols & all_columns_mask;
        if (cols != 0x0) {
            break;
        }   
    }

    for (int i = 0; i < 4; i++) {
        gpio_put(_rows[i], 1);
    }

    if (cols == column_mask[0]) {
        return (char)_matrix_values[row * 4 + 0];
    }
    else if (cols == column_mask[1]) {
        return (char)_matrix_values[row * 4 + 1];
    }
    if (cols == column_mask[2]) {
        return (char)_matrix_values[row * 4 + 2];
    }
    else if (cols == column_mask[3]) {
        return (char)_matrix_values[row * 4 + 3];
    }
    else {
        return 0;
    }
}

// Mapeamento da matriz (5x5)
int getIndex(int x, int y) {
    return (y % 2 == 0) ? y * 5 + x : y * 5 + (4 - x);
}

// Animação do coração
void heartAnimation() {

    int corazon[][2] = {
        {2, 0},  // Base do coração
        {1, 1}, {3, 1},  // Meio inferior
        {0, 2}, {4, 2},  // Laterais
        {0, 3}, {2, 3}, {4, 3},  // Meio superior
        {1, 4}, {3, 4}  // Topo
    };
    
    // Coração aparecendo
    for (int i = 0; i < 10; i++) {
        int idx = getIndex(corazon[i][0], corazon[i][1]);
        npSetLED(idx, 10, 0, 0); // Cor vermelha
        npWrite();
        sleep_ms(100);
    }

    sleep_ms(500); // Mantém o coração aceso por um tempo


    // Apaga o coração gradualmente
    for (int i = 9; i >= 0; i--) {
        int idx = getIndex(corazon[i][0], corazon[i][1]);
        npSetLED(idx, 0, 0, 0);
        npWrite();
        sleep_ms(100);
    }
}


//função principal
int main() {

    // Inicializa matriz de LEDs NeoPixel.
    npInit(LED_PIN);
    npClear();

    // Aqui, você desenha nos LEDs.

    npWrite(); // Escreve os dados nos LEDs.

    stdio_init_all();
    pico_keypad_init(columns, rows, KEY_MAP);
    char caracter_press;
    gpio_init(GPIO_LED);
    gpio_set_dir(GPIO_LED, GPIO_OUT);

    while (true) {
        caracter_press = pico_keypad_get_key();
        printf("\nTecla pressionada: %c\n", caracter_press);

        //Avaliação de caractere para o LED
        if (caracter_press=='B')
        {
            for (uint i=0; i < LED_COUNT; i++){
                npSetLED(i, 0, 0, 255);
                sleep_ms(200);
                npWrite();
            }
        }

        if (caracter_press=='A'){
            npClear();
            npWrite();
        }
        
        if (caracter_press == '*'){
            rom_reset_usb_boot(0, 0);
        }

                
        if (caracter_press == '2'){

            heartAnimation();
            
        }
        
        busy_wait_us(500000);
    }
}