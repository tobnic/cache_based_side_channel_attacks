
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
#include "xtime_l.h"
#include "stdio.h"
#include "xscutimer.h"

#include "xscugic.h"
#include "xil_exception.h"
#include "xttcps.h"



/* The Tx and Rx tasks as described at the top of this file. */
static void prvVictimTask( void *pvParameters );
static TaskHandle_t xVictimTask;


// Global variables
int mycountarray[4000] ={0};
int interruptCounter = 0;


#define TTC_DEVICE_ID	    XPAR_XTTCPS_0_DEVICE_ID
#define TTC_INTR_ID		    XPAR_XTTCPS_0_INTR
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID

typedef struct {
	u32 OutputHz;	/* Output frequency */
	u16 Interval;	/* Interval value */
	u8 Prescaler;	/* Prescaler value */
	u16 Options;	/* Option settings */
} TmrCntrSetup;

static TmrCntrSetup SettingsTable[1] = {
	{175000*10, 0, 0, 0},	/* Ticker timer counter initial setup*/
};

extern XScuGic xInterruptController;

static void SetupInterruptSystem(XScuGic *GicInstancePtr, XTtcPs *TtcPsInt);
static void TickHandler(void *CallBackRef);

XTtcPs Timer;

void initPMU(void) {
  __asm__ volatile ("MCR p15, 0, %0, c9, c12, 0\n\t" :: "r" (0x00000017));
}
int main( void )
{
	initPMU();
	XTtcPs_Config *Config;
	TmrCntrSetup *TimerSetup;

	TimerSetup = &SettingsTable[TTC_DEVICE_ID];

	//Timer = &(TtcPsInst[TTC_DEVICE_ID]);
	XTtcPs_Stop(&Timer);

	//initialise the timer
	Config = XTtcPs_LookupConfig(TTC_DEVICE_ID);
	XTtcPs_CfgInitialize(&Timer, Config, Config->BaseAddress);

	TimerSetup->Options |= (XTTCPS_OPTION_INTERVAL_MODE |
							  XTTCPS_OPTION_WAVE_DISABLE);

	XTtcPs_SetOptions(&Timer, TimerSetup->Options);
	XTtcPs_CalcIntervalFromFreq(&Timer, TimerSetup->OutputHz,&(TimerSetup->Interval), &(TimerSetup->Prescaler));

	XTtcPs_SetInterval(&Timer, TimerSetup->Interval);
	XTtcPs_SetPrescaler(&Timer, TimerSetup->Prescaler);

	SetupInterruptSystem(&xInterruptController, &Timer);



	XTtcPs_Stop(&Timer);

	xTaskCreate( 	prvVictimTask, 					/* The function that implements the task. */
					( const char * ) "Attacker", 		/* Text name for the task, provided to assist debugging only. */
					configMINIMAL_STACK_SIZE, 	/* The stack allocated to the task. */
					NULL, 						/* The task parameter is not used, so set to NULL. */
					tskIDLE_PRIORITY+1,			/* The task runs at the idle priority. */
					&xVictimTask );


	vTaskStartScheduler();

	for( ;; );
}


static void SetupInterruptSystem(XScuGic *GicInstancePtr, XTtcPs *TtcPsInt)
{


		XScuGic_Config *IntcConfig; //GIC config
		Xil_ExceptionInit();

		//initialise the GIC
		IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);

		XScuGic_CfgInitialize(GicInstancePtr, IntcConfig,
						IntcConfig->CpuBaseAddress);

	    //connect to the hardware
		Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
					(Xil_ExceptionHandler)XScuGic_InterruptHandler,
					GicInstancePtr);

		XScuGic_Connect(GicInstancePtr, TTC_INTR_ID,
				(Xil_ExceptionHandler)TickHandler, (void *)TtcPsInt);


		XScuGic_Enable(GicInstancePtr, TTC_INTR_ID);
		XTtcPs_EnableInterrupts(TtcPsInt, XTTCPS_IXR_INTERVAL_MASK);

		XTtcPs_Start(TtcPsInt);

		// Enable interrupts in the Processor.
		Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);
	}



int globalCounter = 0;

static void TickHandler(void *CallBackRef)
{
	interruptCounter++;

	mycountarray[interruptCounter] = globalCounter;


	if(interruptCounter > 100){
		XTtcPs_Stop(&Timer);
	}
	XTtcPs_GetInterruptStatus((XTtcPs *)CallBackRef);
}

/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/


static void prvVictimTask( void *pvParameters )
{

	for( ;; )
	{
		XTtcPs_Start(&Timer);

		for (globalCounter = 0; globalCounter < 10000; globalCounter++){
			int j = globalCounter;
			j++;
		}

		for(int i = 0; i < 20; i++){
			printf("%d \n",mycountarray[i]);
		}

		printf("---\n");
		vTaskDelay( pdMS_TO_TICKS( 1000 ) );

	}

}



