/*
 * Mapeamento I2C do display OLED:
 * - SCK: GPIO 3
 * - SDA: GPIO 2
 */

#include <cmath>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <hardware/adc.h>
#include <hardware/i2c.h>
#include <hardware/irq.h>
#include <pico/async_context_freertos.h>
#include <pico/cyw43_arch.h>
#include <pico/lwip_freertos.h>
#include <pico/stdlib.h>

#include <lwip/raw.h>
#include <lwip/tcp.h>

#include "BMSPA_font.h"
#include "acme_5_outlines_font.h"
#include "bubblesstandard_font.h"
#include "crackers_font.h"
#include "image.h"
#include "ssd1306.h"

// Constantes de hardware e buffers.
static const uint8_t kI2cSdaPin = 2;
static const uint8_t kI2cSclPin = 3;
static const uint32_t kI2cFrequencyHz = 400000;
static const int kWaveSamples = 100;
static const int kPoolSize = 6;

// Tabelas de fonte mantidas para compatibilidade com a renderizacao atual.
const uint8_t num_chars_per_disp[] = {7, 7, 7, 5};
const uint8_t *fonts[4] = {acme_font, bubblesstandard_font, crackers_font, BMSPA_font};

struct ean_waveform_s {
    int16_t V[kWaveSamples];
    int16_t I[kWaveSamples];
    uint32_t ID;
};

struct ean_rms_s {
    uint32_t id;
    float V, I, P, Q, S;
};

struct wave2rms_task_arg_t {
    QueueHandle_t QWF, QWE, QRF, QRE;
};

struct oled_task_arg_t {
    QueueHandle_t QRF, QRE;
};

QueueHandle_t ean_wave_full_queue;
QueueHandle_t ean_wave_empty_queue;

ean_waveform_s ean_waveform[kPoolSize];
ean_rms_s ean_rms[kPoolSize];

void setup_gpios(void) {
    // Inicializa I2C1 para o display SSD1306.
    i2c_init(i2c1, kI2cFrequencyHz);
    gpio_set_function(kI2cSdaPin, GPIO_FUNC_I2C);
    gpio_set_function(kI2cSclPin, GPIO_FUNC_I2C);
    gpio_pull_up(kI2cSdaPin);
    gpio_pull_up(kI2cSclPin);
}

void adc_irq_handler() {
    static int sample_index = 0;
    static ean_waveform_s *buffer = 0;
    static uint32_t ID = 0;
    BaseType_t wake = 0;

    // Cada iteracao consome um par V/I do FIFO e preenche 1 amostra.
    while (adc_fifo_get_level() >= 2) {
        uint16_t result_V = adc_fifo_get();
        uint16_t result_I = adc_fifo_get();
        ID++;

        if (!buffer) {
            int r = xQueueReceiveFromISR(ean_wave_empty_queue, &buffer, &wake);
            if (!r) {
                buffer = 0;
            } else {
                buffer->ID = ID;
                sample_index = 0;
            }
        }

        if (buffer) {
            if (sample_index < kWaveSamples) {
                buffer->V[sample_index] = result_V;
                buffer->I[sample_index] = result_I;
                sample_index += 1;
            }
            if (sample_index == kWaveSamples) {
                int r = xQueueSendFromISR(ean_wave_full_queue, &buffer, &wake);
                if (r) {
                    buffer = 0;
                    sample_index = 0;
                }
            }
        }
    }

    if (wake != pdFALSE) {
        taskYIELD();
    }
}

static void task_OLED(oled_task_arg_t *arg) {
    ssd1306_t disp;
    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, i2c1);
    ssd1306_clear(&disp);

    char buffer_text[8];
    ean_rms_s *rms;

    while (true) {
        // Aguarda um buffer RMS pronto para exibicao.
        while (!xQueueReceive(arg->QRF, &rms, portMAX_DELAY))
            ;

        ssd1306_clear(&disp);

        snprintf(buffer_text, sizeof(buffer_text), "%.2f", rms->V);
        ssd1306_draw_string_with_font(&disp, 8, 24, 2, fonts[3], buffer_text);

        snprintf(buffer_text, sizeof(buffer_text), "%.2f", rms->I);
        ssd1306_draw_string_with_font(&disp, 8, 48, 2, fonts[3], buffer_text);

        vTaskDelay(200);
        ssd1306_show(&disp);

        // Devolve o buffer para a fila de RMS vazios.
        while (!xQueueSend(arg->QRE, &rms, portMAX_DELAY))
            ;
    }
}

void wave2rms(const ean_waveform_s *wave, ean_rms_s *rms) {
    float media_V = 0;
    float media_sub_V = 0;

    float media_I = 0;
    float media_sub_I = 0;

    for (int i = 0; i < kWaveSamples; i++) {
        media_sub_V += wave->V[i];
    }
    media_sub_V = media_sub_V / kWaveSamples;

    for (int i = 0; i < kWaveSamples; i++) {
        media_V += pow(wave->V[i] - media_sub_V, 2);
    }

    // Ganho/calibracao do canal de tensao (manter formula original).
    const float kv = 1 / (1 / 120000.0 * 50 * 10 / 3.3 * 4095);
    rms->V = sqrt(media_V / kWaveSamples) * kv;
    printf("RMS V = %.2f\n", rms->V);

    // Ganho/calibracao do canal de corrente (manter formula original).
    const float kvi = 1 / (1 / 1000.0 * 50.0 * 4.7 / 3.3 * 4095.0);

    for (int i = 0; i < kWaveSamples; i++) {
        media_sub_I += wave->I[i];
    }
    media_sub_I = media_sub_I / kWaveSamples;

    for (int i = 0; i < kWaveSamples; i++) {
        media_I += pow(wave->I[i] - media_sub_I, 2);
    }
    rms->I = sqrt(media_I / kWaveSamples) * kvi;
    printf("RMS I = %.2f\n", rms->I);
}

static void task_Calc_wave2rms(wave2rms_task_arg_t *fila) {
    ean_waveform_s *wave;
    ean_rms_s *rms;

    while (true) {
        printf("w2r: arg %p, QRF %p, QRE %p\n", fila, fila->QRF, fila->QRE);

        // Consome uma forma de onda pronta + um buffer RMS livre.
        while (!xQueueReceive(fila->QWF, &(wave), portMAX_DELAY))
            ;
        while (!xQueueReceive(fila->QRE, &(rms), portMAX_DELAY))
            ;

        wave2rms(wave, rms);

        // Devolve buffer de forma de onda e publica RMS calculado.
        while (!xQueueSend(fila->QWE, &(wave), portMAX_DELAY))
            ;
        while (!xQueueSend(fila->QRF, &(rms), portMAX_DELAY))
            ;
    }
}

int main(void) {
    stdio_init_all();
    sleep_ms(1000);

    setup_gpios();

    // Configuracao do ADC em modo FIFO com leitura round-robin dos canais 0 e 1.
    adc_init();
    adc_fifo_setup(
        true,  // Habilita FIFO
        true,  // Ativa DMA (se necessario)
        2,     // Threshold de solicitacao DMA/IRQ
        false, // Sem flag de erro no FIFO
        false  // Sem shift dos valores
    );
    adc_set_round_robin((1 << 0) | (1 << 1));
    adc_set_clkdiv(4000.0);

    // Instala e habilita o handler de interrupcao do FIFO do ADC.
    irq_set_exclusive_handler(ADC_IRQ_FIFO, adc_irq_handler);
    irq_set_enabled(ADC_IRQ_FIFO, true);
    adc_run(true);

    // Filas de pool para buffers de forma de onda.
    ean_wave_full_queue = xQueueCreate(kPoolSize, sizeof(ean_waveform_s *));
    ean_wave_empty_queue = xQueueCreate(kPoolSize, sizeof(ean_waveform_s *));

    for (int i = 0; i < kPoolSize; i++) {
        ean_waveform_s *p = &ean_waveform[i];
        xQueueSend(ean_wave_empty_queue, &(p), portMAX_DELAY);
    }

    // Filas de pool para buffers RMS.
    QueueHandle_t QRF = xQueueCreate(kPoolSize, sizeof(ean_rms_s *));
    QueueHandle_t QRE = xQueueCreate(kPoolSize, sizeof(ean_rms_s *));

    for (int i = 0; i < kPoolSize; i++) {
        ean_rms_s *p = &ean_rms[i];
        xQueueSend(QRE, &(p), portMAX_DELAY);
    }

    wave2rms_task_arg_t wave2rms_task_arg;
    wave2rms_task_arg.QWE = ean_wave_empty_queue;
    wave2rms_task_arg.QWF = ean_wave_full_queue;
    wave2rms_task_arg.QRE = QRE;
    wave2rms_task_arg.QRF = QRF;

    oled_task_arg_t task_arg_oled;
    task_arg_oled.QRE = QRE;
    task_arg_oled.QRF = QRF;

    // Pipeline:
    // ADC IRQ -> fila de ondas cheias -> task_Calc_wave2rms -> task_OLED.
    xTaskCreate((TaskFunction_t)task_OLED, "sample", 4096, &task_arg_oled, 14, NULL);
    xTaskCreate((TaskFunction_t)task_Calc_wave2rms, "calc", 4096, &wave2rms_task_arg, 15, NULL);

    adc_irq_set_enabled(true);
    vTaskStartScheduler();
    return 0;
}
