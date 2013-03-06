#include <LPC17xx.h>
#include "uart.h"
#include "userproc.h"
#include "rtx.h"
#include "memory.h"
#include "process.h"
#include "timer.h"
#include "atomic.h"

int main() {
	SystemInit();
	
	//timer_init(0);

	atomic(0);
	uart_init(0);
	memory_init();
	process_init();

	atomic(1);
	k_uart_put_string("======================================\n\r");
	k_uart_put_string("RTX has been initialized successfully.\n\r");
	k_uart_put_string("======================================\n\r\n\r");

	
	
	__set_CONTROL(__get_CONTROL() | BIT(0));  //user mode enabled
	
	release_processor();
	
	return 0;
}
