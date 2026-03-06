# PicoPowerRT

Sistema embarcado em tempo real com Raspberry Pi Pico W e FreeRTOS para aquisicao de sinais de tensao/corrente via ADC, calculo RMS em pipeline com filas e exibicao dos resultados em OLED SSD1306 via I2C.

## Descricao do Projeto 

Este projeto demonstra um pipeline embarcado em tempo real focado em desempenho, previsibilidade e modularidade.

Objetivo tecnico:
- Capturar tensao e corrente com ADC em interrupcao.
- Processar RMS em task dedicada (FreeRTOS).
- Exibir os valores em display SSD1306 via I2C.
- Manter fluxo continuo usando pool de buffers e filas (sem malloc em tempo de execucao do pipeline principal).

## Visao de Engenharia

Este firmware foi estruturado como uma arquitetura de producer/consumer:
- `adc_irq_handler()` produz blocos de amostras `V/I`.
- `task_Calc_wave2rms()` consome os blocos e gera medidas RMS.
- `task_OLED()` consome as medidas e atualiza a interface do usuario.

A decisao de usar filas de ponteiros para buffers pre-alocados evita copia pesada de dados e reduz jitter no processamento.

## Arquitetura do Sistema

Fluxo principal:

1. ADC gera amostras em round-robin (canal 0 e 1).
2. ISR monta uma janela fixa de 100 amostras (`kWaveSamples`).
3. Janela pronta vai para a fila de waveforms cheias (`ean_wave_full_queue`).
4. Task de calculo retira waveform, calcula RMS e publica em `QRF`.
5. Task do OLED exibe os valores e devolve o buffer RMS para `QRE`.

Pools de memoria:
- `ean_waveform[6]`: pool de buffers de amostra.
- `ean_rms[6]`: pool de buffers de resultado.

## Hardware e Interfaces

Mapeamento usado:
- OLED SSD1306 (I2C1), SDA em GPIO 2, SCL em GPIO 3, endereco `0x3C`.
- ADC em round-robin nos canais 0 (tensao) e 1 (corrente).

## Funcao de Cada Bloco (main.cpp)

### Estruturas
- `ean_waveform_s`: bloco de aquisicao com 100 amostras de tensao/corrente + ID.
- `ean_rms_s`: bloco de resultado com campos RMS e espaco para grandezas eletricas adicionais.
- `wave2rms_task_arg_t`: agrega as quatro filas da task de calculo.
- `oled_task_arg_t`: agrega as duas filas usadas pela task de exibicao.

### Funcoes
1. `setup_gpios()`: inicializa I2C1 em 400 kHz, configura GPIO2/GPIO3 como I2C e ativa pull-up.
2. `adc_irq_handler()`: roda em interrupcao, le pares `V/I` do FIFO, monta janela de 100 amostras, publica em `ean_wave_full_queue` e cede CPU quando necessario.
3. `wave2rms(const ean_waveform_s* wave, ean_rms_s* rms)`: remove componente DC, calcula RMS de tensao/corrente e aplica calibracao (`kv` e `kvi`).
4. `task_Calc_wave2rms(wave2rms_task_arg_t* fila)`: consome waveform pronta, executa `wave2rms(...)`, devolve buffer de onda e publica resultado RMS.
5. `task_OLED(oled_task_arg_t* arg)`: inicializa SSD1306, recebe RMS, renderiza `V/I` no display e devolve buffer RMS ao pool.
6. `main()`: inicializa perifericos, configura ADC/IRQ, cria filas, prepara pools, cria tasks e inicia o scheduler.

## Driver do Display (ssd1306.c / ssd1306.h)

API publica principal:
- `ssd1306_init` / `ssd1306_deinit`: ciclo de vida do display.
- `ssd1306_poweron` / `ssd1306_poweroff`: controle de energia logica.
- `ssd1306_contrast` / `ssd1306_invert`: ajuste visual.
- `ssd1306_clear` / `ssd1306_show`: limpeza e flush do framebuffer.
- `ssd1306_draw_pixel`, `ssd1306_draw_line`, `ssd1306_draw_square`, `ssd1306_draw_empty_square`: primitivas graficas.
- `ssd1306_draw_char_with_font`, `ssd1306_draw_string_with_font`, `ssd1306_draw_char`, `ssd1306_draw_string`: renderizacao de texto.
- `ssd1306_bmp_show_image` / `ssd1306_bmp_show_image_with_offset`: exibicao de bitmap monocromatico.

## Hooks do FreeRTOS (hooks.cpp)

- `vApplicationMallocFailedHook`: assert em falha de alocacao.
- `vApplicationStackOverflowHook`: assert em overflow de pilha.
- `vApplicationIdleHook`: hook de ociosidade (atualmente vazio).
- `vApplicationTickHook`: hook de tick (atualmente vazio).

## Diferenciais Tecnicos 

- Pipeline orientado a tempo real com separacao clara entre aquisicao, processamento e exibicao.
- Uso de pool fixo de memoria para reduzir fragmentacao e latencia imprevisivel.
- Baixo acoplamento entre tasks via filas.
- Codigo pronto para extensao de metricas eletricas (`P`, `Q`, `S`) sem quebrar a arquitetura.

## Como Compilar e Rodar

### 1) Dependencias

```sh
sudo apt install -y cmake git \
  gcc-arm-none-eabi \
  libnewlib-arm-none-eabi \
  libstdc++-arm-none-eabi-newlib

sudo mkdir -p /opt/pico
cd /opt/pico
sudo git clone --depth 1 https://github.com/FreeRTOS/FreeRTOS-Kernel
sudo git clone --depth 1 https://github.com/raspberrypi/pico-sdk
cd /opt/pico/pico-sdk
sudo git submodule update --init --depth 1
```

### 2) Variaveis de ambiente

```sh
export PICO_BOARD=pico_w
export PICO_SDK_PATH=/opt/pico/pico-sdk
export FREERTOS_KERNEL_PATH=/opt/pico/FreeRTOS-Kernel
export WIFI_SSID=your_wifi_name
export WIFI_PASSWORD=public_secret
```

Observacao:
- Neste firmware, Wi-Fi esta linkado no projeto mas nao ha task de rede ativa no `main.cpp` atual.

### 3) Build

```sh
cd <raiz-do-projeto>
cmake -S . -B build
cmake --build build -j
```

Artefatos principais:
- `build/main.uf2`
- `build/main.elf`

### 4) Flash

Opcao manual:
1. Entrar em modo BOOTSEL no Pico W.
2. Copiar `main.uf2` para a unidade `RPI-RP2`.

Opcao com script:
```sh
./helper.sh build/main.uf2
```

## Resultado Esperado

- Terminal serial com logs de `RMS V` e `RMS I`.
- OLED com RMS de tensao na linha superior e RMS de corrente na linha inferior.