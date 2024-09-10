#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* Library includes. */
#include <stdio.h>
#include <pico/stdio.h>
#include <pico/cyw43_arch.h>
#include <pico/lwip_freertos.h>
#include <pico/async_context_freertos.h>

#include <lwip/raw.h>
#include <lwip/tcp.h>

static void sample_task(void *) {
    // Start wifi
    printf("Starting wifi...\n");
    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        while(true);
    }
    printf("Enabling station mode...\n");
    cyw43_arch_enable_sta_mode();
    printf("Connecting to %s...\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        exit(1);
    } else {
        printf("Connected.\n");
    }

    int i=0;
    while(true) {
        uint32_t ip = netif_ip4_addr(netif_default)->addr;
        printf("%d: %d.%d.%d.%d\n",
            i++,
            ip % 256,
            ip / 256 % 256,
            ip / 256 / 256 % 256,
            ip / 256 / 256 / 256 % 256
        );

        vTaskDelay(configTICK_RATE_HZ);
    }
}

static void glados_task(void *) {
    TickType_t timer = 0;
    while(true) {
        vTaskDelayUntil(&timer, 10*configTICK_RATE_HZ);
        printf("Still alive!\n");
    }
}

int main( void ) {
    stdio_init_all();

    for (int i=0; i<10; ++i) {
        printf("Booting in %d...\n", 10-i);
        sleep_ms(1000);
    }

    printf("Creating tasks...\n");
    xTaskCreate(sample_task, "sample", 4096, NULL, 10, NULL);
    xTaskCreate(glados_task, "glados", 512, NULL, 1, NULL);

    printf("Starting FreeRTOS-SMP scheduler...\n");
    vTaskStartScheduler();
    return 0;
}
/*-----------------------------------------------------------*/
