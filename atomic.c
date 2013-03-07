#include <LPC17xx.h>

int sem = 0;

void atomic(int on){
	if(on){
		if(--sem == 0){
		NVIC_EnableIRQ(TIMER0_IRQn);
	  __enable_irq();
		}
	}else{
		if(++sem == 1){
			NVIC_DisableIRQ(TIMER0_IRQn);
			__disable_irq();
		}
	}
}
