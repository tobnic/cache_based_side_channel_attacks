
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

uint32_t measureAccessTimePMU(void* pointer)
{
	uint32_t start = 0;
	uint32_t ende = 0;
	asm volatile ("DSB");
	asm volatile ("MRC p15, 0, %0, c9, c13, 0" : "=r" (start));
	asm volatile ("DSB");
	volatile uint32_t value;
	  asm volatile ("LDR %0, [%1]\n\t"
	      : "=r" (value)
	      : "r" (pointer)
	);
	  asm volatile ("DSB");
	asm volatile ("MRC p15, 0, %0, c9, c13, 0" : "=r" (ende));
	 asm volatile ("DSB");
  return (ende-start);

}
/*-----------------------------------------------------------*/
static void prvAttackerTask( void *pvParameters )
{
	for( ;; )
	{
		printf("attacker...\n");
		Xil_L1DCacheFlush();
		Xil_L1ICacheInvalidate();	Xil_L2CacheFlush();
		uint8_t array2[1024];

		for (int cl = 0; cl < 512; cl++){ // guess cache_line size
		for(int x = 0; x<30; x++){		// repeat to overcome randomization
			for(int i = 1; i<8; i++){	// fill the set
				measureAccessTimePMU(&array2[(i*cl+5)*32]);
			}
		}
	    	if(measureAccessTimePMU(&array2[5*32]) > 50){
	    		printf("# cache sets: %d \n",cl);
	    		break;
	        }
		}
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

