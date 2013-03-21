#include <LPC17xx.h>
#include "uart.h"
#include "userproc.h"
#include "rtx.h"
#include "memory.h"
#include "message.h"
#include "process.h"
#include "timer.h"
#include "atomic.h"

int main() {
	SystemInit();
	atomic(0);
	memory_init();
	message_init();
	uart_init(0);
	process_init();
	timer_init(0);
	atomic(1);
	uart_put_string("======================================\n\r");
	uart_put_string("RTX has been initialized successfully.\n\r");
	
	

	__set_CONTROL(__get_CONTROL() | BIT(0));  //user mode enabled

	uart_put_string("======================================\n\r\n\r");
	release_processor();
	
	return 0;
}
