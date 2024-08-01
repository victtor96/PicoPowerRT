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
    cyw43_arch_enable_sta_mode();
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

int main( void ) {
    stdio_init_all();

    printf("Creating tasks...\n");
    xTaskCreate(sample_task, "sample", 2048, NULL, 10, NULL);

    printf("Starting FreeRTOS-SMP scheduler...\n");
    vTaskStartScheduler();
    return 0;
}
/*-----------------------------------------------------------*/
