
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
#include "stdio.h"
#include "xscutimer.h"


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


/* flush the L1 data and instruction cache*/
void flushL1Cache(){
	Xil_L1DCacheFlush();
	Xil_L1ICacheInvalidate();
}

/* flush the L2 cache*/
void flushL2Cache(){
	Xil_L2CacheFlush();
}

static XScuTimer my_Timer;//timer

int main( void )
{


	//init clock
	initPMU();
	XScuTimer_Config *Timer_Config;
	Timer_Config = XScuTimer_LookupConfig(XPAR_PS7_SCUTIMER_0_DEVICE_ID);
	XScuTimer_CfgInitialize(&my_Timer, Timer_Config, Timer_Config->BaseAddr);
	XScuTimer_LoadTimer(&my_Timer, 0xFFFFFFFF);


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


/*
 *		Private Timer
 */
int measureAccessTimeSCU(int flushL1, int flushL2){


	uint8_t array[256]={0};
	uint8_t tmp = 0;

	for (int i = 0; i< 256;i+= 1){
		tmp = array[i];
	}

	if ( flushL1 ) flushL1Cache();
	if ( flushL2 ) flushL2Cache();

	uint64_t startT = ((*(volatile unsigned long *) (0xF8F00600 +0x00000200U+ 0x04U)) << 32U) | *(volatile unsigned long *)(my_Timer.Config.BaseAddr + 0x04U);

	asm volatile ("DSB");
	tmp = array[86];
	asm volatile ("DSB");
	uint64_t endT = ((*(volatile unsigned long *) (0xF8F00600 +0x00000200U+ 0x04U)) << 32U) | *(volatile unsigned long *)(my_Timer.Config.BaseAddr + 0x04U);

	return ( startT-endT ) ;
}

/*
 *		Global Timer
 */
int measureAccessTimeGT(int flushL1, int flushL2){
	uint8_t array[256]={0};
	uint8_t tmp = 0;

	for (int i = 0; i< 256;i+= 1){
		tmp = array[i];
	}

	if ( flushL1 ) flushL1Cache();
	if ( flushL2 ) flushL2Cache();

	uint64_t startT = ((*(volatile unsigned long *) (0xF8F00000U + 0x00000200U + 0x04U)) << 32U) | *(volatile unsigned long *)(0xF8F00000U + 0x00000200U + 0x00U);

	asm volatile ("DSB");
	tmp = array[86];
	asm volatile ("DSB");
	uint64_t endT = ((*(volatile unsigned long *) (0xF8F00000U + 0x00000200U + 0x04U)) << 32U) | *(volatile unsigned long *)(0xF8F00000U + 0x00000200U + 0x00U);

	return ( endT-startT ) ;
}

/*
 *		PMU Timer
 */
int measureAccessTimePMU(int flushL1, int flushL2){
	uint8_t array[256]={0};
	uint8_t tmp = 0;

	for (int i = 0; i< 256;i+= 1){
		tmp = array[i];
	}

	if ( flushL1 ) flushL1Cache();
	if ( flushL2 ) flushL2Cache();

	uint64_t startT;
	uint64_t endT;

	asm volatile ("MRC p15, 0, %0, c9, c13, 0" : "=r" (startT));

	asm volatile ("DSB");
	tmp = array[86];
	asm volatile ("DSB");
	asm volatile ("MRC p15, 0, %0, c9, c13, 0" : "=r" (endT));
	return ( endT-startT ) ;
}

/*
 *		RTOS Timer
 */
int measureAccessTimeRTOS(int flushL1, int flushL2){
	uint8_t array[256]={0};
	uint8_t tmp = 0;

	for (int i = 0; i< 256;i+= 1){
		tmp = array[i];
	}

	if ( flushL1 ) flushL1Cache();
	if ( flushL2 ) flushL2Cache();

	uint64_t startT = xTaskGetTickCount();

	asm volatile ("DSB");
	tmp = array[86];
	asm volatile ("DSB");
	uint64_t endT = xTaskGetTickCount();

	return ( endT-startT ) ;
}

void measureAllTimer(){
	/*
	 *		Private Timer
	 */

	int sumTime = 0;
	for (int j = 0; j< 100;j+= 1){
		sumTime += measureAccessTimeSCU(0,0);
	}
	printf("SCU hit L1 %d \n",sumTime/100);


	sumTime = 0;
	for (int j = 0; j< 100;j+= 1){
		sumTime += measureAccessTimeSCU(1,0);
	}
	printf("SCU hit L2 %d \n",sumTime/100);


	sumTime = 0;
	for (int j = 0; j< 100;j+= 1){
		sumTime += measureAccessTimeSCU(1,1);
	}
	printf("SCU miss %d \n\n",sumTime/100);


	/*
	 *		Global Timer
	 */

	sumTime = 0;
	for (int j = 0; j< 100;j+= 1){
		sumTime += measureAccessTimeGT(0,0);
	}
	printf("GT hit L1 %d \n",sumTime/100);


	sumTime = 0;
	for (int j = 0; j< 100;j+= 1){
		sumTime += measureAccessTimeGT(1,0);
	}
	printf("GT hit L2 %d \n",sumTime/100);


	sumTime = 0;
	for (int j = 0; j< 100;j+= 1){
		sumTime += measureAccessTimeGT(1,1);
	}
	printf("GT miss %d \n\n",sumTime/100);


	/*
	 *		PMU Timer
	 */

	sumTime = 0;
	for (int j = 0; j< 100;j+= 1){
		sumTime += measureAccessTimePMU(0,0);
	}
	printf("GT hit L1 %d \n",sumTime/100);


	sumTime = 0;
	for (int j = 0; j< 100;j+= 1){
		sumTime += measureAccessTimePMU(1,0);
	}
	printf("GT hit L2 %d \n",sumTime/100);


	sumTime = 0;
	for (int j = 0; j< 100;j+= 1){
		sumTime += measureAccessTimePMU(1,1);
	}
	printf("GT miss %d \n\n",sumTime/100);

	/*
	 *		RTOS Timer
	 */

	sumTime = 0;
	for (int j = 0; j< 100;j+= 1){
		sumTime += measureAccessTimeRTOS(0,0);
	}
	printf("GT hit L1 %d \n",sumTime/100);


	sumTime = 0;
	for (int j = 0; j< 100;j+= 1){
		sumTime += measureAccessTimeRTOS(1,0);
	}
	printf("GT hit L2 %d \n",sumTime/100);


	sumTime = 0;
	for (int j = 0; j< 100;j+= 1){
		sumTime += measureAccessTimeRTOS(1,1);
	}
	printf("GT miss %d \n\n",sumTime/100);


}

/*-----------------------------------------------------------*/
static void prvAttackerTask( void *pvParameters )
{

	measureAllTimer();
	for( ;; )
	{
		printf("attacker...");

		vTaskDelay( pdMS_TO_TICKS( 1000UL ) );
	}
}

/*-----------------------------------------------------------*/
static void prvVictimTask( void *pvParameters )
{
	int i = 0;

	for( ;; )
	{
		printf("victim...");


		printf("\n");
		vTaskDelay( pdMS_TO_TICKS( 1000UL ) );

	}
}

/*-----------------------------------------------------------*/

