/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
/* Xilinx includes. */
#include "xil_printf.h"
#include "xparameters.h"
#include "xil_cache_l.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*-----------------------------------------------------------*/

/* The attacker and victim tasks*/
static void prvSenderTask( void *pvParameters );
static void prvReceiverTask( void *pvParameters );

static TaskHandle_t xSenderTask;
static TaskHandle_t xReceiverTask;

/*-----------------------------------------------------------*/

void initPMU(void) {
  __asm__ volatile ("MCR p15, 0, %0, c9, c12, 0\n\t" :: "r" (0x00000017));
}

int main( void )
{

	initPMU();
	xil_printf( "Hello from Freertos example main\r\n" );

	xTaskCreate( prvSenderTask, 					/* The function that implements the task. */
					( const char * ) "Attacker", 		/* Text name for the task, provided to assist debugging only. */
					configMINIMAL_STACK_SIZE, 	/* The stack allocated to the task. */
					NULL, 						/* The task parameter is not used, so set to NULL. */
					tskIDLE_PRIORITY+3,			/* The task runs at the idle priority. */
					&xSenderTask );

	xTaskCreate( prvReceiverTask,
				 ( const char * ) "Victim",
				 configMINIMAL_STACK_SIZE,
				 NULL,
				 tskIDLE_PRIORITY + 2,
				 &xReceiverTask );


	vTaskStartScheduler();

	for( ;; );
}


// Shared array between the Tasks
uint8_t sharedArray[5*1024 * 1024];

//Measures the access time for a given memory address
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

void flush_cache_set(int cach_set_max,int cache_line_size,int cache_set_nr){
	for(int cache_way = 1; cache_way <=20; cache_way++){	// fill the set
		measureAccessTimePMU(&sharedArray[(cache_way*cach_set_max+cache_set_nr)*cache_line_size]);
	}
}



static void prvSenderTask( void *pvParameters )
{

	char messageSend[32] = "freeRTOS_Covert_Channel 1234567\n";
	uint8_t bitStream[256]={0};
	int bitPointer = 0;

	//Fill Cache
	for (int i = 0; i< 256;i++){
		measureAccessTimePMU(&sharedArray[i*32]);
	}
	for( ;; )
	{
		// Convert string to single bits
		int index = 0;
	    for(size_t i = 0; i < strlen(messageSend); ++i) {
	        char ch = messageSend[i];
	        for(int j = 7; j >= 0; --j){
	            if(ch & (1 << j)) {
	            	bitStream[index++] = 1;
	            } else {
	            	bitStream[index++] = 0;
	            }
	        }
	    }

	    // Map Binary to Cache
			// if a bit is 0 then flush the corresponding cache set
			if(bitStream[(bitPointer++)%256]){
				for(int i = 0; i<256;i++){
					flush_cache_set(256,32,i);
				}
			}
		vTaskDelay( 1 );
	}
}

static void prvReceiverTask( void *pvParameters )
{
	char messageRec[32] = "";
	char bitStream[256]={0};
	int RxtaskCntr = 0;

	for( ;; )
	{
		//message counter for statistics only
		int bitOneCounter = 0;
		int bitZeroCounter = 0;
		//measure access time for each 32nd array element
		for (int i = 0; i< 256;i++){
			if(	measureAccessTimePMU(&sharedArray[i*32])>50){
				//cache miss
				bitOneCounter++;
			}
			else{
				// cache hit
				bitZeroCounter++;
			}
		}

		if(bitOneCounter>bitZeroCounter)
			bitStream[RxtaskCntr]='1';
		else
			bitStream[RxtaskCntr]='0';

		printf("#%d 1:%d 0:%d -> %c \n",RxtaskCntr, bitOneCounter, bitZeroCounter,bitStream[RxtaskCntr]);
		RxtaskCntr = (RxtaskCntr+1)%256;

		if(RxtaskCntr%8==0){
		// Convert single bits to ASCII chars
		   char subbuff[9];
		   unsigned char c;
		   int index = 0;

		   for(int k = 0; k < RxtaskCntr; k += 8) {
			   memcpy(subbuff, &bitStream[k], 8);
			   subbuff[8] = '\0';
			   c = (unsigned char)strtol(subbuff, 0, 2);
			   messageRec[index++] = c;
			   messageRec[index] = '\0';
		   }
		   printf("		Message: %s \n",messageRec);

		}

		//Fill Cache
		for (int i = 0; i< 256;i++){
			measureAccessTimePMU(&sharedArray[i*32]);
		}



		vTaskDelay(1);

	}
}
