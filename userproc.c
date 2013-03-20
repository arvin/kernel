#include "rtx.h"
#include "uart.h"
#include "userproc.h"
#include "lib.h"
#ifdef DEBUG_0
#include <stdio.h>
#endif  /* DEBUG_0 */

#define true 1

int passCount = 0;
int procStage = 0;
int start_clk;

void sendCRTMsg(char* msg, int sender_pid){
	Message* crt_msg;
	crt_msg = (Message*) request_memory_block();
	crt_msg->type = CRT_DISPLAY;
	crt_msg->dest_pid = get_system_pid(CRT);
	crt_msg->sender_pid = sender_pid;
	crt_msg->data = string_copy(request_memory_block(), msg);
	send_message(get_system_pid(CRT), (void*) crt_msg);
}


//New process 1
void proc1(void) {
	
	Message* msg;
	
	int sender_id = 2;
	int* msgNum;
	
	sendCRTMsg("-- RTX Test Start --\n\rTest 1: Delayed Receive\n\r", 1);
	
	msg = (Message*)receive_message(& sender_id);
	msgNum =  (msg->data);
	
	if (procStage == 1 && *msgNum == 9999) {
		sendCRTMsg("Test 1: PASSED\n\r", 1);
	}
	else
		sendCRTMsg("Test 1: FAILED\n\r", 1);
	
	release_memory_block(msg->data);
	release_memory_block(msg);
	
	do
		release_processor();
	while (true);
}

void proc2(void){
	Message* new_Message;
	int* message_data;
	sendCRTMsg("Test 2: Delayed Send\n\r", 2);
	
	new_Message = (Message*) request_memory_block();
	message_data = (int*)request_memory_block();
	*message_data = 9999;
	
	new_Message->data = (void*) message_data;
	new_Message->sender_pid = 2;
	new_Message->dest_pid = 1;
	new_Message->type = 1;

	delayed_send(1, (void*) new_Message, 5);
	procStage++;
  do
		release_processor();
	while (true);
}


void proc3(void){
	int i;
	int errorFlag = 0;
	
	passCount++;
	sendCRTMsg("Test 3: Allocate and deallocate memory 50 times.\n\r", 3);
	
	for(i = 0; i < 50; i++){
		errorFlag |= release_memory_block(request_memory_block());
	}
	
	if (errorFlag)
		sendCRTMsg("FAILED\n\r", 3);
	else {
		passCount++;
		sendCRTMsg("PASSED\n\r", 3);
	}
	
	do
		release_processor();
	while (true);
	
}

void proc4(void){
	//Allocate to max and then deallocate all
	int result;
	
	result = set_process_priority(1, 2);
	sendCRTMsg("Test 4a: Set priority to a valid number.\n\r", 4);
	if(result == 0) {
		passCount++;
		sendCRTMsg("PASSED\n\r", 4);
	}
	else
		sendCRTMsg("FAILED\n\r", 4);
	
	result = set_process_priority(1, 9000);
	
	sendCRTMsg("Test 4b: Set priority to an invalid number.\n\r", 4);
	if(result == -1) {
		passCount++;
		sendCRTMsg("PASSED\n\r", 4);
	}
	else
		sendCRTMsg("FAILED\n\r", 4);
	
	sendCRTMsg("Test 4c: Setting priority of null process should be prohibited.\n\r", 4);
	set_process_priority(0, 1);
	if(result == -1) {
		passCount++;
		sendCRTMsg("PASSED\n\r", 4);
	}
	else
		sendCRTMsg("FAILED\n\r", 4);
	
	

	do
		release_processor();
	while (true);
}

void proc5(void) {
	
	Message *new_Message;
	Message *msg;
	char* message_data;
	char* s = "%C";
	int sender_id = 0;
	char* received_msg_data;
	char* temp;
	int target_pid;
	int target_priority;
	int index;
	
	sendCRTMsg("Test 5: CHANGE A PROCESS's PRIORITY.\n\r", 5);
	
	new_Message = (Message*)request_memory_block();
	message_data = (char*)request_memory_block();
	message_data = s;
 	
	new_Message->data = (void*) message_data;
	new_Message->sender_pid = 5;
	new_Message->dest_pid = get_system_pid(KCD);
	new_Message->type = COMMAND_REG;

	send_message(get_system_pid(KCD), (void*) new_Message);
	
	while(1){
		msg = (Message*)receive_message(& sender_id);

		received_msg_data =  (msg->data);
		
	 if(msg->type == COMMAND){
			if(contains_prefix(received_msg_data, "%C")){
				//NOTE ADD CHECK TO SEE IF CMD IS VALID
				
				
				//Get the process_id
				target_pid = *(received_msg_data+3) - '0';
				index = 4;
				temp = received_msg_data+index;
				
				//Check if double digit
				if(*temp != ' '){
					target_pid *= 10;
					target_pid += *(received_msg_data+index) - '0';
					index = 6;
				}else{
					
					index = 5;
				}
				
				//Get the priority id
				target_priority = *(received_msg_data+index) - '0';
				index += 1;
				temp = received_msg_data+index;
				if(*temp != '\0'){
					target_priority *= 10;
					target_priority += *(received_msg_data+index) - '0';
				}
				
				set_process_priority(target_pid, target_priority);
				
			}
		}
		
		uart_put_int(target_pid);
		uart_put_string("\n\r");
		uart_put_int(target_priority);
		uart_put_string("\n\r");
		
		release_memory_block(msg->data);
		release_memory_block(msg);
	}
}


int is_valid_time_cmd(char* a){
	char *b = a;
	int i = 0;
	int timeCheck = 0;
	
	//Check for correct length
	while(*b != '\0'){
		b++;
		i++;
	}
	if(i != 12)
	 return FALSE;

	//Check for the colons 
	if(*(a+6) != ':' || *(a+9) != ':')
		 return FALSE;
	
	//Check for the valid numbers
	for(i = 4; i < 12; i++){
	 if(i != 6 && i != 9){
		if(*(a+i) < '0' || *(a+i) > '9')
			 return FALSE;
 		}
	}
	
	//Check is time is valid
	timeCheck = (*(a + 4) - '0')*10 +(*(a + 5) - '0');
	if(timeCheck >= 24){
		return FALSE;
	}
	timeCheck = (*(a + 7) - '0')*10 +(*(a + 8) - '0');
	if(timeCheck >= 60){
		return FALSE;
	}
	timeCheck = (*(a + 10) - '0')*10 +(*(a + 11) - '0');
	if(timeCheck >= 60){
		return FALSE;
	}
	
	return TRUE;

}

void proc6(void){
	//24 hour wall clock - is supposed to ba a user level process
	Message *new_Message;
	Message *msg;
	char* message_data;
	char* s = "%W";
	int sender_id = 2;
	char* received_msg_data;
	int displayTime = 0;
	int time = 0;
	
	sendCRTMsg("Test 6: 24 Hour Wall Clock Display Process\n\r", 6);
	start_clk = FALSE;
	new_Message = (Message*)request_memory_block();
	message_data = (char*)request_memory_block();
	message_data = s;
 	
	new_Message->data = (void*) message_data;
	new_Message->sender_pid = 6;
	new_Message->dest_pid = get_system_pid(KCD);
	new_Message->type = COMMAND_REG;

	send_message(get_system_pid(KCD), (void*) new_Message);
	
	set_process_priority(6, 0);
	
	sendCRTMsg("-- RTX Test End --\n\r", 6);
	
	while(1){
		msg = (Message*)receive_message(& sender_id);

		received_msg_data =  (msg->data);
		
	 if(msg->type == COMMAND){
			if(string_equals(received_msg_data, "%WR")){
				displayTime = 1;
				set_timer_count(0);
				start_clk = TRUE;
			}else if(contains_prefix(received_msg_data, "%WS")){
				if(is_valid_time_cmd(received_msg_data)){	
					time = 0;
					time += (*(received_msg_data + 4) - '0')*3600*10;
					time += (*(received_msg_data + 5) - '0')*3600;
					time += (*(received_msg_data + 7) - '0')*60*10;
					time += (*(received_msg_data + 8) - '0')*60;
					time += (*(received_msg_data + 10) - '0')*10;
					time += (*(received_msg_data + 11) - '0');
					time *= 1000;
					set_timer_count(time); 
					displayTime = 1;
					start_clk = TRUE;
				} else {
					sendCRTMsg("No you don't!\n\r", 6);
				}
			}else if(string_equals(received_msg_data, "%WT")){
				displayTime = 0;
			}
		}
		else if(msg->type == DISPLAY_TIME){
			if(displayTime)
				display_time();
		}
		
		release_memory_block(msg->data);
		release_memory_block(msg);
	}

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
