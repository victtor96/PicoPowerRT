#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* Library includes. */
#include <stdio.h>
#include <pico/stdio.h>

int main( void ) {
    stdio_init_all();
    printf("FreeRTOS-SMP started.\n");

    vTaskStartScheduler();
    return 0;
}
/*-----------------------------------------------------------*/
