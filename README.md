# raspberry_pico_freertos

This repository contains a clean starting point for using Raspberry Pi Pico W with FreeRTOS.

The following environment variables must be defined:

* `PICO_SDK_PATH`: Full path to the pico-SDK.
* `RTOS_KERNEL_PATH`: Full path to the SMP variant of the FreeRTOS kernel.

# Setting up
On Fedora 37:

```
sudo mkdir /opt/pico
cd /opt/pico
sudo git clone --depth 1 --branch smp https://github.com/FreeRTOS/FreeRTOS-Kernel
sudo git clone --depth 1 --branch master https://github.com/raspberrypi/pico-sdk
cd /opt/pico/pick-sdk
sudo git submodule --init --depth 1
sudo dnf install arm-none-eabi-*
```

If using the above setup you should

```
PICO_SDK_PATH=/opt/pico/pico-sdk
RTOS_KERNEL_PATH=/opt/pico/FreeRTOS-Kernel
```
