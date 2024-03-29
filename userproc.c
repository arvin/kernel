#include "rtx.h"
#include "uart.h"
#include "userproc.h"
#ifdef DEBUG_0
#include <stdio.h>
#endif  /* DEBUG_0 */

#define true 1

int passCount = 0;
int proc4Stage = 0;


//Original, old process 1
/*
void proc1(void) {
	uart_put_string("-- RTX Test Start --\n\r");
	uart_put_string("Test 1: Deallocate non-existing blocks.\n\r");
	
	
	if (release_memory_block(0)) {
		passCount++;
		uart_put_string("PASSED\n\r");
	}
	else
		uart_put_string("FAILED\n\r");
	
	do
		release_processor();
	while (true);
}
*/

//New process 1
void proc1(void) {
	
	Message* msg;
	int sender_id = 2;
	int* msgNum;
	uart_put_string("-- RTX Test Start --\n\r");
	uart_put_string("Test 1: Deallocate non-existing blocks.\n\r");

	if (release_memory_block(0)) {
		passCount++;
		uart_put_string("PASSED\n\r");
	}
	else
		uart_put_string("FAILED\n\r");
	
	msg = (Message*)receive_message(& sender_id);

	msgNum =  (msg->data);
	uart_put_int(*msgNum);
	uart_put_string("\n\r\n\r");
	
	
	do
		release_processor();
	while (true);
}


//Old process 2
/*
void proc2(void){
	uart_put_string("Test 2: Release processor\n\r");
	
  do
		release_processor();
	while (true);
}
*/



void proc2(void){
	Message* new_Message;
	int* message_data;
	uart_put_string("Test 2: Release processor\n\r");
	
	new_Message = (Message*) request_memory_block();
	message_data = (int*)request_memory_block();
	*message_data = 9999;
	
	new_Message->data = (void*) message_data;
	new_Message->sender_pid = 2;
	new_Message->dest_pid = 1;
	new_Message->type = 1;

//	send_message(1, (void*) new_Message);
	delayed_send(1, (void*) new_Message, 5);
  do
		release_processor();
	while (true);
}


void proc3(void){
	int i;
	int errorFlag = 0;
	
	passCount++;
	uart_put_string("PASSED\n\r");
	
	uart_put_string("Test 3: Allocate and deallocate memory 50 times.\n\r");
	
	for(i = 0; i < 50; i++){
		errorFlag |= release_memory_block(request_memory_block());
	}
	
	if (errorFlag)
		uart_put_string("FAILED\n\r");
	else {
		passCount++;
		uart_put_string("PASSED\n\r");
	}
	
	do
		release_processor();
	while (true);
	
}

void proc4(void){
	//Allocate to max and then deallocate all
	void** arr[3];
	int i = 0;
	int errorFlag = 0;
	
	uart_put_string("Test 4 (Step 1): Allocate 95 blocks.\n\r");
	for (i = 0; i < 3; ++i)
		arr[i] = request_memory_block();
	
	i = 0;
	while(i < 92) {
		arr[i/32][i%32] = (void *)request_memory_block();
		i++;
	}
	
	proc4Stage++;
	release_processor();
	
	uart_put_string("Test 4 (Step 2): Deallocate 95 blocks.\n\r");
	i--;
	
	while(i >= 0) {
		errorFlag |= release_memory_block(arr[i/32][i%32]);
		i--;
	}
	for (i = 0; i < 3; ++i)
		errorFlag |= release_memory_block(arr[i]);
	
	if (errorFlag)
		uart_put_string("FAILED\n\r");
	else {
		passCount++;
		uart_put_string("PASSED\n\r");
	}
	
	proc4Stage++;
	do
		release_processor();
	while (true);
}

void proc5(void) {
	void* block;
	block = request_memory_block();
	release_memory_block(block);
	uart_put_string("Test 5: Request for memory block while all memory blocks are used.\n\r");
	if (proc4Stage == 2) {
		passCount++;
		uart_put_string("PASSED\n\r");
	}
	else
		uart_put_string("FAILED\n\r");
	
	do
		release_processor();
	while (true);
}

void proc6(void) {
	int result;
	
	release_processor();
	
	result = set_process_priority(1, 2);
	uart_put_string("Test 6a: Set priority to a valid number.\n\r");
	if(result == 0) {
		passCount++;
		uart_put_string("PASSED\n\r");
	}
	else
		uart_put_string("FAILED\n\r");
	
	result = set_process_priority(1, 9000);
	
	uart_put_string("Test 6b: Set priority to an invalid number.\n\r");
	if(result == -1) {
		passCount++;
		uart_put_string("PASSED\n\r");
	}
	else
		uart_put_string("FAILED\n\r");
	
	uart_put_string("Test 6c: Setting priority of null process should be prohibited.\n\r");
	set_process_priority(0, 1);
	if(result == -1) {
		passCount++;
		uart_put_string("PASSED\n\r");
	}
	else
		uart_put_string("FAILED\n\r");
	
	uart_put_string("Total number of pass count:\n\r");
	uart_put_int(passCount);
	uart_put_string(" out of 8 passed in total.\n\r");
	uart_put_string("-- RTX Test End --\n\r");
	do
		release_processor();
	while (true);
}

int ProcessCount = 6;
void* ProcessTable[] = {
	&proc1,
	&proc2,
	&proc3,
	&proc4,
	&proc5,
	&proc6
};
