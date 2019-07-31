// ->plain_standalone

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
    uint8_t arrayRec[20*256*32];


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
	for(int cache_way = 5; cache_way <9; cache_way++){	// fill the set
		  asm volatile ("DSB");
		  measureAccessTimePMU(&arrayRec[(cache_way*cach_set_max+cache_set_nr)*cache_line_size]);
	}
}

int main()
{
    init_platform();
    __asm__ volatile ("MCR p15, 0, %0, c9, c12, 0\n\t" :: "r" (0x00000017));

    int sumMisses = 0;
    int rounds = 10000;

    for(int r = 1; r < 31; r++){

    int misscounter = 0;
    for(int i = 0; i < rounds; i++){
		Xil_L1DCacheFlush();
		Xil_L2CacheDisable();
		Xil_L1ICacheInvalidate();

		asm volatile ("DSB");
		//Group A #4
		measureAccessTimePMU(&arrayRec[(0*256+3)*32]);
		measureAccessTimePMU(&arrayRec[(5*256+3)*32]);
		measureAccessTimePMU(&arrayRec[(6*256+3)*32]);
		measureAccessTimePMU(&arrayRec[(7*256+3)*32]);
		/*Group B #3
		measureAccessTimePMU(&arrayRec[(0*256+3)*32]);
		measureAccessTimePMU(&arrayRec[(1*256+3)*32]);
		measureAccessTimePMU(&arrayRec[(6*256+3)*32]);
		measureAccessTimePMU(&arrayRec[(7*256+3)*32]);
		*/
		/*Group C #2
		measureAccessTimePMU(&arrayRec[(0*256+3)*32]);
		measureAccessTimePMU(&arrayRec[(1*256+3)*32]);
		measureAccessTimePMU(&arrayRec[(2*256+3)*32]);
		measureAccessTimePMU(&arrayRec[(6*256+3)*32]);
		*/
		/*Group D #1
		measureAccessTimePMU(&arrayRec[(0*256+3)*32]);
		measureAccessTimePMU(&arrayRec[(1*256+3)*32]);
		measureAccessTimePMU(&arrayRec[(2*256+3)*32]);
		measureAccessTimePMU(&arrayRec[(3*256+3)*32]);
		*/
		asm volatile ("DSB");
		for (int rr = 0; rr < r; rr++)
			flush_cache_set(256,32,3);
		asm volatile ("DSB");
		if(measureAccessTimePMU(&arrayRec[(0*256+3)*32]) > 50){
			misscounter++;
		}
    }
	sumMisses+=misscounter;

    printf("N %d, misses %d from %d \n", r,sumMisses,rounds);
    	sumMisses = 0;
    }
    cleanup_platform();
    return 0;
}
