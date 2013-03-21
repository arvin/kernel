#include <LPC17xx.h>
#include "rtx.h"
#include "uart.h"
#include "atomic.h"
#include "process.h"

volatile uint8_t g_UART0_TX_empty=1;
volatile uint8_t g_UART0_buffer[BUFSIZE];
volatile uint32_t g_UART0_count = 0;
volatile int read_command = FALSE;
volatile uint8_t command_buffer[BUFSIZE];
volatile uint32_t command_index = 0;
volatile char* data_buff;
volatile void* timer_reserved_memory[3];
/**
 * @brief: initialize the n_uart
 * NOTES: only fully supports uart0 so far, but can be easily extended 
 *        to other uarts.
 *        The step number in the comments matches the item number 
 *        in Section 14.1 on pg 298 of LPC17xx_UM
 */
int uart_init(int n_uart) {

	LPC_UART_TypeDef *pUart;
	
	timer_reserved_memory[0] = request_system_memory_block();
	timer_reserved_memory[1] = request_system_memory_block();
	timer_reserved_memory[2] = request_system_memory_block();

	if (n_uart ==0 ) {
		/*
		Steps 1 & 2: system control configuration.
		Under CMSIS, system_LPC17xx.c does these two steps
		 
		-----------------------------------------------------
		Step 1: Power control configuration. 
		        See table 46 pg63 in LPC17xx_UM
		-----------------------------------------------------
		Enable UART0 power, this is the default setting
		done in system_LPC17xx.c under CMSIS.
		Enclose the code for your refrence
		//LPC_SC->PCONP |= BIT(3);
	
		-----------------------------------------------------
		Step2: Select the clock source. 
		       Default PCLK=CCLK/4 , where CCLK = 100MHZ.
		       See tables 40 & 42 on pg56-57 in LPC17xx_UM.
		-----------------------------------------------------
		Check the PLL0 configuration to see how XTAL=12.0MHZ 
		gets to CCLK=100MHZin system_LPC17xx.c file.
		PCLK = CCLK/4, default setting after reset.
		Enclose the code for your reference
		//LPC_SC->PCLKSEL0 &= ~(BIT(7)|BIT(6));	
			
		-----------------------------------------------------
		Step 5: Pin Ctrl Block configuration for TXD and RXD
		        See Table 79 on pg108 in LPC17xx_UM.
		-----------------------------------------------------
		Note this is done before Steps3-4 for coding purpose.
		*/
		
		/* Pin P0.2 used as TXD0 (Com0) */
		LPC_PINCON->PINSEL0 |= (1 << 4);  
		
		/* Pin P0.3 used as RXD0 (Com0) */
		LPC_PINCON->PINSEL0 |= (1 << 6);  

		pUart = (LPC_UART_TypeDef *) LPC_UART0;	 
		
	} else if (n_uart == 1) {
	    
		/* see Table 79 on pg108 in LPC17xx_UM */ 
		/* Pin P2.0 used as TXD1 (Com1) */
		LPC_PINCON->PINSEL0 |= (2 << 0);

		/* Pin P2.1 used as RXD1 (Com1) */
		LPC_PINCON->PINSEL0 |= (2 << 2);	      

		pUart = (LPC_UART_TypeDef *) LPC_UART1;
		
	} else {
		return 1; /* not supported yet */
	} 
	
	/*
	-----------------------------------------------------
	Step 3: Transmission Configuration.
	        See section 14.4.12.1 pg313-315 in LPC17xx_UM 
	        for baud rate calculation.
	-----------------------------------------------------
        */
	
	/* Step 3a: DLAB=1, 8N1 */
	pUart->LCR = UART_8N1; /* see uart.h file */ 

	/* Step 3b: 115200 baud rate @ 25.0 MHZ PCLK */
	pUart->DLM = 0; /* see table 274, pg302 in LPC17xx_UM */
	pUart->DLL = 9;	/* see table 273, pg302 in LPC17xx_UM */
	
	/* FR = 1.507 ~ 1/2, DivAddVal = 1, MulVal = 2
	   FR = 1.507 = 25MHZ/(16*9*115200)
	   see table 285 on pg312 in LPC_17xxUM
	*/
	pUart->FDR = 0x21;       
	
 

	/*
	----------------------------------------------------- 
	Step 4: FIFO setup.
	       see table 278 on pg305 in LPC17xx_UM
	-----------------------------------------------------
        enable Rx and Tx FIFOs, clear Rx and Tx FIFOs
	Trigger level 0 (1 char per interrupt)
	*/
	
	pUart->FCR = 0x07;

	/* Step 5 was done between step 2 and step 4 a few lines above */

	/*
	----------------------------------------------------- 
	Step 6 Interrupt setting and enabling
	-----------------------------------------------------
	*/
	/* Step 6a: 
	   Enable interrupt bit(s) wihtin the specific peripheral register.
           Interrupt Sources Setting: RBR, THRE or RX Line Stats
	   See Table 50 on pg73 in LPC17xx_UM for all possible UART0 interrupt sources
	   See Table 275 on pg 302 in LPC17xx_UM for IER setting 
	*/
	/* disable the Divisior Latch Access Bit DLAB=0 */
	pUart->LCR &= ~(BIT(7)); 
	
	pUart->IER = IER_RBR | IER_THRE | IER_RLS; 

	/* Step 6b: enable the UART interrupt from the system level */
	NVIC_EnableIRQ(UART0_IRQn); /* CMSIS function */
	

	return 0;
}

/**
 * @brief: use CMSIS ISR for UART0 IRQ Handler
 * NOTE: This example shows how to save/restore all registers rather than just
 *       those backed up by the exception stack frame. We add extra
 *       push and pop instructions in the assembly routine. 
 *       The actual c_UART0_IRQHandler does the rest of irq handling
 */
__asm void UART0_IRQHandler(void)
{
	PRESERVE8
	IMPORT c_UART0_IRQHandler
	PUSH{r4-r11, lr}
	BL c_UART0_IRQHandler
	POP{r4-r11, pc}
} 
/**
 * @brief: c UART0 IRQ Handler
 */
void c_UART0_IRQHandler(void)
{
	uint8_t IIR_IntId;      /* Interrupt ID from IIR */		
	uint8_t LSR_Val;        /* LSR Value             */
	uint8_t dummy = dummy;	/* to clear interrupt upon LSR error */
	
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *)LPC_UART0;

	/* Reading IIR automatically acknowledges the interrupt */
	IIR_IntId = (pUart->IIR) >> 1 ; /* skip pending bit in IIR */

	if (IIR_IntId & IIR_RDA) { /* Receive Data Avaialbe */
		/* read UART. Read RBR will clear the interrupt */
		g_UART0_buffer[g_UART0_count++] = pUart->RBR;
		if (g_UART0_count == BUFSIZE) {
			g_UART0_count = 0; /* buffer overflow */
		}
		
		trigger_uart_i_process();
	} else if (IIR_IntId & IIR_THRE) { 
		/* THRE Interrupt, transmit holding register empty*/
		
		LSR_Val = pUart->LSR;
		if(LSR_Val & LSR_THRE) {
			g_UART0_TX_empty = 1; /* ready to transmit */ 
		} else {  
			g_UART0_TX_empty = 0; /* not ready to transmit */
		}
	    
	} else if (IIR_IntId & IIR_RLS) {
		LSR_Val = pUart->LSR;
		if (LSR_Val  & (LSR_OE|LSR_PE|LSR_FE|LSR_RXFE|LSR_BI) ) {
			/* There are errors or break interrupt 
		           Read LSR will clear the interrupt 
			   Dummy read on RX to clear interrupt, then bail out
			*/
			dummy = pUart->RBR; 
			return; /* error occurs, return */
		}
		/* If no error on RLS, normal ready, save into the data buffer.
	           Note: read RBR will clear the interrupt 
		*/
		if (LSR_Val & LSR_RDR) { /* Receive Data Ready */
			/* read from the uart */
			g_UART0_buffer[g_UART0_count++] = pUart->RBR; 
			if ( g_UART0_count == BUFSIZE ) {
				g_UART0_count = 0;  /* buffer overflow */
			}	

			trigger_uart_i_process();
		}	    
	} else { /* IIR_CTI and reserved combination are not implemented */
		return;
	}	
}

void trigger_uart_i_process() {
	set_process_state(get_current_process_id(), INTERRUPTED);
	set_process_state(k_get_system_pid(UART), RUN);
	
	uart_i_process();
	
	set_process_state(k_get_system_pid(UART), WAIT_FOR_INTERRUPT);
	set_process_state(get_current_process_id(), RUN);
	
}

void uart_i_process( /*uint8_t *p_buffer, uint32_t len*/ ){
	uint8_t *p_buffer = (uint8_t *)g_UART0_buffer;
	uint32_t len =  g_UART0_count;
	Message *inputMsg;
	Message *cmdMsg;
	Message *receivedMsg;
	unsigned char* msgData;
	char* inputData;
	char* crtData;
			
			/* Re-enable RBR, THRE left as enabled */
  receivedMsg =	(Message*)system_proc_receive_message(UART);
	if(receivedMsg) {
		
		msgData = (unsigned char*) (receivedMsg->data);
		uart_put_string(msgData);
		release_memory_block(receivedMsg);
		release_memory_block(receivedMsg->data);
	}
	else{
		LPC_UART0->IER = IER_THRE | IER_RLS; 
		g_UART0_count = 0;
		while ( len != 0 ) {
			
			inputMsg = (Message*)k_request_memory_block();
			if (inputMsg == NULL)
				inputMsg = (Message*)timer_reserved_memory[0];
			inputData = (char*)k_request_memory_block();
			if (inputData == NULL)
				inputData = (char*)timer_reserved_memory[1];
			
			*inputData = *p_buffer;
			*(inputData + 1) = '\0';
			inputMsg->data = inputData;
			inputMsg->type = KEYBOARD_INPUT;
			inputMsg->dest_pid = k_get_system_pid(KCD);
			inputMsg->sender_pid = k_get_system_pid(UART);
			send_msg(k_get_system_pid(KCD), inputMsg, 0, timer_reserved_memory[2]);
			
			
			
			p_buffer++;
			len--;
		}

		LPC_UART0->IER = IER_THRE | IER_RLS | IER_RBR;
	}
	
}

int uart_put_string(unsigned char *s)
{
  while (*s !=0) {              /* loop through each char in the string */
		while ( !(g_UART0_TX_empty & 0x01) );	
    uart_put_char(*s++);/* print the char, then ptr increments  */
		g_UART0_TX_empty = 0;  // not empty in the THR until it shifts out
  }
  return 0;
}

  
int uart_put_char(unsigned char c)
{

	LPC_UART_TypeDef *pUart;
	pUart = (LPC_UART_TypeDef *)LPC_UART0;
	
	
	pUart->THR = c;

	return 0;
}

void uart_put_hex(int val) {

	int i;
	 for (i = 7; i >= 0; --i) {
		 char c = (0xF & (val >> (i * 4)));
		 uart_put_char(c + (c < 10 ? '0' : ('A' - 10)));
	}
		uart_put_string("\n\r");
}

void uart_put_int(int val) {
	int i;
	int temp;
	int original;
	if (val == 0) {
		uart_put_char('0');
		return;
	}
	if (val < 0) {
		val = -val;
		uart_put_char('-');
	}
	original = val;
	for (i = 1000000000; i > 0; i /= 10) {
		if (i < original) {
			temp = val / i;
			uart_put_char(temp + '0');
		}
		val %= i;
	}
}




