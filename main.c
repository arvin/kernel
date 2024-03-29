#include <LPC17xx.h>
#include "uart.h"
#include "userproc.h"
#include "rtx.h"
#include "memory.h"
#include "process.h"
#include "timer.h"

int main() {
	SystemInit();
	__disable_irq();
	
	uart_init(0);
	memory_init();
	process_init();
	timer_init(0);

	
	__enable_irq();
	
	uart_put_string("======================================\n\r");
	uart_put_string("RTX has been initialized successfully.\n\r");
	uart_put_string("======================================\n\r\n\r");

	__set_CONTROL(__get_CONTROL() | BIT(0));
	release_processor();
	
	return 0;
}
