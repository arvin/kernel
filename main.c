#include <LPC17xx.h>
#include "uart_polling.h"
#include "userproc.h"
#include "rtx.h"
#include "memory.h"
#include "process.h"

int main() {
	SystemInit();
	__disable_irq();
	
	uart1_init();
	memory_init();
	process_init();
	
	__enable_irq();
	
	uart1_put_string("======================================\n\r");
	uart1_put_string("RTX has been initialized successfully.\n\r");
	uart1_put_string("======================================\n\r\n\r");

	__set_CONTROL(__get_CONTROL() | BIT(0));
	release_processor();
	
	return 0;
}
