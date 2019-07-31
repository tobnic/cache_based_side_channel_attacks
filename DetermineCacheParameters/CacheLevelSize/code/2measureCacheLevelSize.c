
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
	uint8_t test = 0;
	uint8_t array1024[1*1024] = {0};
	uint32_t avg_access = 0;
	for(int j =0; j < 1024 ; j +=2){
		avg_access = 0;
		for(int i =0; i < j*1024 ; i += 32){
			test = array1024[i] *3;
			test++;
		}

		for(int i =0; i < 1*1024 ; i += 1){
			avg_access += measureAccessTimePMU(&array1024[rand() % (j*1024)]);
		}

		printf("%d, %d\n",j,(int)(avg_access/(1*1024)));

	}

	for( ;; )
	{
		printf("attacker...");

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

