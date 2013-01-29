#include "rtx.h"
#include "uart_polling.h"
#include "userproc.h"
#ifdef DEBUG_0
#include <stdio.h>
#endif  /* DEBUG_0 */



void proc1(void) {
	int errorFlag = 1;
	uart1_put_string("Test 1: Deallocate non-existing blocks.\n\r");
	
	if (s_release_memory_block(0))
		uart1_put_string("PASSED\n\r");
	else
		uart1_put_string("FAILED\n\r");
	
	release_processor();
}

void proc2(void){
	uart1_put_string("Test 2: Release processor (passed if no error messages are shown)\n\r");
  release_processor();
	uart1_put_string("FAILED\n\r");
	
}

void proc3(void){
	int i = 0;
	int errorFlag = 0;
	
	uart1_put_string("Test 3: Allocate and deallocate memory 50 times.\n\r");
	
	for(i; i < 50; i++){
		errorFlag |= s_release_memory_block(s_requestion_memory_block());
	}
	
	if (errorFlag)	
		uart1_put_string("FAILED\n\r");
	else
		uart1_put_string("PASSED\n\r");
	release_processor();
	
}

void proc4(void){
	//Allocate to max and then deallocate all
	void** arr;
	int i = 0;
	int errorFlag = 0;
	int reachMax = -1;
	
	uart1_put_string("Test 4a: Allocate 32 blocks.\n\r");
	arr = s_requestion_memory_block();
	
	while(reachMax != 0 && i < 31) {
		arr[i] = (void *)s_requestion_memory_block();
		reachMax = (uint32_t)arr[i];
		uart1_put_hex(arr[i]);
		i++;
	}
	if (i == 31) 
		uart1_put_string("PASSED\n\r");
	else
		uart1_put_string("FAILED\n\r");
	
	uart1_put_string("Test 4b: Deallocate 32 blocks.\n\r");
	i--;
	
	for(i; i >=0; i--) {
		errorFlag |= s_release_memory_block(arr[i]);
	}
	errorFlag |= s_release_memory_block(arr);
	
	if (errorFlag)
		uart1_put_string("FAILED\n\r");
	else
		uart1_put_string("PASSED\n\r");
	
	release_processor();
}

void proc5(void) {
	int result = set_process_priority(1, 2);
	uart1_put_string("Test 5: Set priority to a valid number.\n\r");
	if(result == 0)
		uart1_put_string("PASSED\n\r");
	else
		uart1_put_string("FAILED\n\r");
	
	release_processor();
}

void proc6(void) {
	int result = set_process_priority(1, 9000);
	
	uart1_put_string("Test 6a: Set priority to an invalid number.\n\r");
	if(result == -1)
		uart1_put_string("PASSED.\n\r");
	else
		uart1_put_string("FAILED.\n\r");
	
	uart1_put_string("Test 6b: Setting priority of null process should be prohibited.\n\r");
	set_process_priority(0, 1);
	if(result == -1)
		uart1_put_string("PASSED\n\r");
	else
		uart1_put_string("FAILED\n\r");
	
}

void assign_processes() {
  add_new_process(&proc1);
	add_new_process(&proc2);
	add_new_process(&proc3);
	add_new_process(&proc4);
	add_new_process(&proc5);
	add_new_process(&proc6);
}