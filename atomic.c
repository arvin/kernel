#include <LPC17xx.h>


void atomic(int on){
	if(on){
		
		NVIC_EnableIRQ(TIMER0_IRQn);
	__enable_irq();
	}else{
		NVIC_DisableIRQ(TIMER0_IRQn);
	__disable_irq();
		
	}
}
