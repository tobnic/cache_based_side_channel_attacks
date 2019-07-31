/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

/* Xilinx includes. */
#include "xil_printf.h"
#include "xparameters.h"
#include "xil_cache_l.h"
#include "xtime_l.h"
#include <stdio.h>
#include <stdlib.h>

/* The attacker and victim tasks*/
static void prvAttackerTask( void *pvParameters );
static void prvVictimTask( void *pvParameters );
/*-----------------------------------------------------------*/

/* The queue used by the Tx and Rx tasks, as described at the top of this
file. */
static TaskHandle_t xAttackerTask;
static TaskHandle_t xVictimTask;

/* reset pmu counter value*/

void initPMU(void) {
  __asm__ volatile ("MCR p15, 0, %0, c9, c12, 0\n\t" :: "r" (0x00000017));
}

int main( void )
{

	initPMU();
	xil_printf( "Hello from Freertos example main\r\n" );

	xTaskCreate( 	prvAttackerTask, 					/* The function that implements the task. */
					( const char * ) "Attacker", 		/* Text name for the task, provided to assist debugging only. */
					configMINIMAL_STACK_SIZE, 	/* The stack allocated to the task. */
					NULL, 						/* The task parameter is not used, so set to NULL. */
					tskIDLE_PRIORITY+3,			/* The task runs at the idle priority. */
					&xAttackerTask );

	xTaskCreate( prvVictimTask,
				 ( const char * ) "Victim",
				 configMINIMAL_STACK_SIZE,
				 NULL,
				 tskIDLE_PRIORITY + 2,
				 &xVictimTask );


	vTaskStartScheduler();

	for( ;; );
}


/*-----------------------------------------------------------*/
static void prvAttackerTask( void *pvParameters )
{
	uint8_t array2[1024];

    for(int i = 0; i<1024; i++){
		uint32_t start = 0;
		uint32_t ende = 0;
		volatile uint32_t value;

		asm volatile ("DSB");
		vTaskDelay( pdMS_TO_TICKS( 5UL ) );

		asm volatile ("MRC p15, 0, %0, c9, c13, 0" : "=r" (start));
		asm volatile ("DSB");
		  asm volatile ("LDR %0, [%1]\n\t"
		      : "=r" (value)
		      : "r" (array2[i*1])
		);
		  asm volatile ("DSB");
		asm volatile ("MRC p15, 0, %0, c9, c13, 0" : "=r" (ende));
		 asm volatile ("DSB");
			printf("%d %d\n",i,ende-start);

	}

	for( ;; )
	{
		printf("attacker...\n");

		vTaskDelay( pdMS_TO_TICKS( 1000UL ) );
	}
}

/*-----------------------------------------------------------*/
static void prvVictimTask( void *pvParameters )
{
	for( ;; )
	{
		printf("victim... \n");
		vTaskDelay( pdMS_TO_TICKS( 1000UL ) );

	}
}

/*-----------------------------------------------------------*/
