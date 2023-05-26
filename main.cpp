#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* Library includes. */
#include <stdio.h>
#include <pico/stdio.h>
#include <pico/lwip_freertos.h>
#include <pico/async_context_freertos.h>

async_context_freertos_t async_context;
async_context_freertos_config async_config = {
    .task_priority = 9,
    .task_stack_size = 1024
};

static void sample_task(void *) {
    while(true) {
        asm(" nop");
        vTaskDelay(5*configTICK_RATE_HZ);
    }
}

int main( void ) {
    stdio_init_all();
    printf("FreeRTOS-SMP started.\n");

    async_context_freertos_init(&async_context, &async_config);
    lwip_freertos_init((async_context_t*)&async_context);

    xTaskCreate(sample_task, "sample", 128, NULL, 10, NULL);

    vTaskStartScheduler();
    return 0;
}
/*-----------------------------------------------------------*/
