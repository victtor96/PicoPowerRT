#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* Library includes. */
#include <stdio.h>
#include <pico/stdio.h>

void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );

int main( void ) {
    stdio_init_all();
    printf("FreeRTOS-SMP started.\n");

    vTaskStartScheduler();
    return 0;
}
/*-----------------------------------------------------------*/
