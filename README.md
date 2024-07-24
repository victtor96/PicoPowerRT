# raspberry_pico_freertos

This repository contains a clean starting point for using Raspberry Pi Pico W with FreeRTOS.

The following environment variables must be defined:

* `PICO_SDK_PATH`: Full path to the pico-SDK.
* `FREERTOS_KERNEL_PATH`: Full path to the FreeRTOS kernel.

# Setting up
Start by installing arm-none-eabi-gcc, make, cmake, and git. Then:

```
sudo mkdir /opt/pico
cd /opt/pico
sudo git clone --depth 1 https://github.com/FreeRTOS/FreeRTOS-Kernel
sudo git clone --depth 1 --branch master https://github.com/raspberrypi/pico-sdk
cd /opt/pico/pick-sdk
sudo git submodule update --init --depth 1
```

Then, for building you should:

```
export PICO_BOARD=pico_w
export PICO_SDK_PATH=/opt/pico/pico-sdk
export FREERTOS_KERNEL_PATH=/opt/pico/FreeRTOS-Kernel
cd /tmp
git clone https://gitlab.com/lucas.hartmann/raspberry_pico_freertos
mkdir raspberry_pico_freertos/build
cd raspberry_pico_freertos/build
cmake ..
make
```
