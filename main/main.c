#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "ssd/ssd1306.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C
#define WIDTH 128
#define HEIGHT 64

#define JOYSTICK_X_PIN 26   // Eixo X do joystick
#define JOYSTICK_Y_PIN 27   // Eixo Y do joystick

#define QUADRADO_SIZE 8     // Tamanho do quadrado (8x8 pixels)
#define DEADZONE 50         // Zona morta para evitar oscilações

#define MIN_X 0
#define MAX_X (WIDTH - QUADRADO_SIZE)
#define MIN_Y 0
#define MAX_Y (HEIGHT - QUADRADO_SIZE)

// Pinos dos LEDs RGB
#define LED_BLUE_PIN 13     // LED Azul (vertical, eixo Y)
#define LED_RED_PIN 12      // LED Vermelho (horizontal, eixo X)

int main() {
    stdio_init_all();

    // Inicialização do I2C (400 kHz) para o display
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicialização do display OLED SSD1306
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
    
    // Limpa o display inicialmente
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Inicialização do ADC para o joystick
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);

    // Configuração do PWM para os LEDs
    gpio_set_function(LED_RED_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_BLUE_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(LED_RED_PIN);
    uint channel_red = pwm_gpio_to_channel(LED_RED_PIN);
    uint channel_blue = pwm_gpio_to_channel(LED_BLUE_PIN);
    pwm_set_wrap(slice_num, 255);
    pwm_set_enabled(slice_num, true);

    // Inicializa a posição do quadrado no centro do display
    int posX = WIDTH / 2 - QUADRADO_SIZE / 2;
    int posY = HEIGHT / 2 - QUADRADO_SIZE / 2;

    while (true) {
        // Leitura do ADC para os eixos X e Y
        adc_select_input(0);
        uint16_t adc_value_x = adc_read();
        adc_select_input(1);
        uint16_t adc_value_y = adc_read();

        // Cálculo do brilho para os LEDs (horizontal para vermelho, vertical para azul)
        int diff_red = abs((int)adc_value_x - 2048);
        int brightness_red = (diff_red * 255) / 2048;
        if (brightness_red > 255) brightness_red = 255;
        if (diff_red < DEADZONE) brightness_red = 0;
        
        int diff_blue = abs((int)adc_value_y - 2048);
        int brightness_blue = (diff_blue * 255) / 2048;
        if (brightness_blue > 255) brightness_blue = 255;
        if (diff_blue < DEADZONE) brightness_blue = 0;

        pwm_set_chan_level(slice_num, channel_red, brightness_red);
        pwm_set_chan_level(slice_num, channel_blue, brightness_blue);

        // Ajuste do movimento do quadrado:
        // O ADC varia de 0 a 4095, então mapeamos para a largura e altura do display.

        // **Correção no eixo X (permitindo ir até o final da tela)**
        posX = (adc_value_x * (WIDTH - QUADRADO_SIZE)) / 4095;
        if (posX < MIN_X) posX = MIN_X;
        if (posX > MAX_X) posX = MAX_X;

        // **Correção no eixo Y (invertendo para que cima seja cima e baixo seja baixo)**
        posY = ((4095 - adc_value_y) * (HEIGHT - QUADRADO_SIZE)) / 4095;
        if (posY < MIN_Y) posY = MIN_Y;
        if (posY > MAX_Y) posY = MAX_Y;

        // Limpa o display antes de desenhar o quadrado
        ssd1306_fill(&ssd, false);

        // Desenha o quadrado na nova posição
        ssd1306_rect(&ssd, posX, posY, QUADRADO_SIZE, QUADRADO_SIZE, 1, true);

        // Atualiza o display com a nova posição do quadrado
        ssd1306_send_data(&ssd);

        sleep_ms(50);
    }

    return 0;
}
