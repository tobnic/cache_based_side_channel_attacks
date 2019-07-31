

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
/* Xilinx includes. */
#include "xil_printf.h"
#include "xparameters.h"
#include "foolib.h"
#include "xil_cache_l.h"
#include <stdlib.h>
#include "stdio.h"

/*-----------------------------------------------------------*/

/* The Tx and Rx tasks as described at the top of this file. */
static void prvAttackerTask( void *pvParameters );
static void prvReceiverTask( void *pvParameters );
/*-----------------------------------------------------------*/

/* The queue used by the Tx and Rx tasks, as described at the top of this
file. */
static TaskHandle_t xAttackerTask;
static TaskHandle_t xVictimTask;



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

void armv7_pmu_init(void) {
  __asm__ volatile ("MCR p15, 0, %0, c9, c12, 0\n\t" :: "r" (0x00000017));
}

void flush_allCaches(){
	Xil_L1DCacheFlush();
	Xil_L2CacheFlush();
	Xil_L1ICacheInvalidate();
}

void measureHitMissTime(void* ptr, int* hit, int* miss){


	uint32_t cycle_counter = 0;
	void (*functionPtr)(void);
	functionPtr = ptr;



	for(int i = 0; i<100; i++){
		//Flush L1 & L2 Cache
		flush_allCaches();
		cycle_counter += measureAccessTimePMU(functionPtr);
	}

	*hit = cycle_counter/(uint32_t)100;
	cycle_counter = 0;

	for(int i = 0; i<100; i++){

		//Flush L1 & L2 Cache
		flush_allCaches();

		(*functionPtr)();

		cycle_counter += measureAccessTimePMU(functionPtr);
	}
	*miss = cycle_counter/(uint32_t)100;
}




int main( void )
{

	//init clock
	armv7_pmu_init();


	xil_printf( "Hello from Freertos example main\r\n" );


	xTaskCreate( 	prvAttackerTask, 					/* The function that implements the task. */
					( const char * ) "Attacker", 		/* Text name for the task, provided to assist debugging only. */
					configMINIMAL_STACK_SIZE, 	/* The stack allocated to the task. */
					NULL, 						/* The task parameter is not used, so set to NULL. */
					tskIDLE_PRIORITY+3,			/* The task runs at the idle priority. */
					&xAttackerTask );

	xTaskCreate( prvReceiverTask,
				 ( const char * ) "Victim",
				 configMINIMAL_STACK_SIZE,
				 NULL,
				 tskIDLE_PRIORITY + 2,
				 &xVictimTask );

//	vTaskAddProtection(&xAttackerTask, &xVictimTask);
//	vTaskAddProtection(&xVictimTask, &xAttackerTask);


	vTaskStartScheduler();

	for( ;; );
}


/*-----------------------------------------------------------*/
static void prvAttackerTask( void *pvParameters )
{

	int cacheHitTimesquareFunc;
	int cacheMissTimesquareFunc;

	int cacheHitTimemultiplyFunc;
	int cacheMissTimemultiplyFunc;

	measureHitMissTime(&squareFunc,&cacheHitTimesquareFunc,&cacheMissTimesquareFunc);
	printf("+++ Miss Time AVG squareFunc: %d \n",cacheHitTimesquareFunc);
	printf("+++ Hit  Time AVG squareFunc: %d \n\n",cacheMissTimesquareFunc);

	measureHitMissTime(&multiplyFunc,&cacheHitTimemultiplyFunc,&cacheMissTimemultiplyFunc);
	printf("+++ Miss Time AVG multiplyFunc: %d \n",cacheHitTimemultiplyFunc);
	printf("+++ Hit  Time AVG multiplyFunc: %d \n\n",cacheMissTimemultiplyFunc);


	for( ;; )
	{

		flush_allCaches();
		uint32_t cycle_counter = 0;

		vTaskDelay( pdMS_TO_TICKS( 1000UL ) );

		printf("[Spy Task]:  \n");

		cycle_counter = measureAccessTimePMU(&squareFunc);

		if(abs(cacheMissTimesquareFunc-cycle_counter)>abs(cacheHitTimesquareFunc-cycle_counter))
			printf("        Cache Miss squareFunc: %d \n",(int)(cycle_counter));
		else
			printf("        Cache Hit squareFunc: %d \n",(int)(cycle_counter));


		cycle_counter = measureAccessTimePMU(&multiplyFunc);

		if(abs(cacheMissTimemultiplyFunc-cycle_counter)>abs(cacheHitTimemultiplyFunc-cycle_counter))
			printf("        Cache Miss multiplyFunc: %d \n",(int)(cycle_counter));
		else
			printf("        Cache Hit multiplyFunc: %d \n",(int)(cycle_counter));


		printf("\n");

	}
}

/*-----------------------------------------------------------*/
static void prvReceiverTask( void *pvParameters )
{
	int i = 0;

	for( ;; )
	{
		printf("\n\n\n\n\n\n");
		printf("[Victim Task]: \n	square or multiply...?\n");

		i++;
		if(i%3 == 0){
			squareFunc();
		}
		if(i%6 == 0){
			multiplyFunc();
		}

		vTaskDelay( pdMS_TO_TICKS( 1000UL ) );

	}
}

/*-----------------------------------------------------------*/


