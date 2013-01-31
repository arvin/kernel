#include <LPC17xx.h>
#include "uart_polling.h"
#include "userproc.h"
#include "rtx.h"
#include "memory.h"
#include "process.h"

int main(){
	SystemInit();
	__disable_irq();
	
	uart1_init();
	memory_init();
	process_init();
	
	__enable_irq();
	// initialize memory
	
	uart1_put_string("Hello World!\n\r");

	__set_CONTROL(__get_CONTROL() | BIT(0));
	manage_processor();
	
	
/*	for(i = 0; i < 32; i++){
		arr[i] = s_requestion_memory_block();	
	}
	
	for(i = 0; i < 32; i++){
		s_release_memory_block(arr[i]);	
	}
	
	
	s_release_memory_block(a);
	s_release_memory_block(a);
	a = s_requestion_memory_block();
	s_release_memory_block(a);
	a = s_requestion_memory_block();
	s_release_memory_block(a);
	a = s_requestion_memory_block();
	//__enable_irq();
	uart1_put_string("Goodbye Cruel World!\n\r");
	*/
	return 0;
}
