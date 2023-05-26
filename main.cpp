#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* Library includes. */
#include <stdio.h>
#include <pico/stdio.h>

static void sample_task(void *) {
    while(true) {
        asm(" nop");
        vTaskDelay(5*configTICK_RATE_HZ);
    }
}

int main( void ) {
    stdio_init_all();
    printf("FreeRTOS-SMP started.\n");

    xTaskCreate(sample_task, "sample", 128, NULL, 10, NULL);

    vTaskStartScheduler();
    return 0;
}
/*-----------------------------------------------------------*/
