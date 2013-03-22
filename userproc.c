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

typedef struct MsgNode{
    Message* message;
    struct MsgNode* next;
}MsgNode;


typedef struct MsgQueue{
    MsgNode* first;
    MsgNode* last;
    int size;
}MsgQueue;


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
	
	/*msg = (Message*)receive_message(& sender_id);
	msgNum =  (msg->data);
	
	if (procStage == 1 && *msgNum == 9999) {
		sendCRTMsg("Test 1: PASSED\n\r", 1);
	}
	else
		sendCRTMsg("Test 1: FAILED\n\r", 1);
	
	release_memory_block(msg->data);
	release_memory_block(msg);*/
	
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

int is_valid_priority_cmd(char* a){
	char *b = a;
	int i = 0;

	//Check for correct length
	while(*b != '\0'){
		b++;
		i++;
	}
	
	if(i > 8) //All commands of format '%C # #\0', '%C ## #\0', '%C ###\0', '%C ## ##\0'
		return FALSE;

	b = a+2; //Skip %C, was already checked by the prefix function

	if(*b !=' ')
		return FALSE;
	b++;
	
	//*b MUST be a number
	for(i=0; i<2; i++){
		if(!(*b>='0' && *b<='9'))
			return FALSE;
		b++;
		if(*b ==' ') //Can now be a space or a number
			break;
	}

	if(*b !=' ')
		return FALSE;

	b++;
	for(i=0; i<2; i++){
		if(!(*b>='0' && *b<='9'))
			return FALSE;
		b++;
		if(*b =='\0') //Must now be the end line character
			break;
	}

	if(*b!='\0')
		return FALSE;

	return TRUE;
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
				 if(is_valid_priority_cmd(received_msg_data)){
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
			
		}
		
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
	
	new_Message = (Message*)request_memory_block();
	new_Message->sender_pid = 6;
	new_Message->dest_pid = 6;
	new_Message->type = DISPLAY_TIME;
	new_Message->data = NULL;
	
	delayed_send(6, new_Message, 1000);
	
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
			release_memory_block(msg->data);
			release_memory_block(msg);
		}
		else if(msg->type == DISPLAY_TIME){
			if(displayTime)
				display_time();
			
			delayed_send(6, msg, 1000);
		}
	}

}


void proc7(void){ //Process A
    Message* new_Message;
    int sender_id;
    char* message_data;
    int* num;
    
    new_Message = (Message*)request_memory_block();
		message_data = (char*)request_memory_block();
		string_copy(message_data, "%Z");
 	
		new_Message->data = (void*) message_data;
		new_Message->sender_pid = 7;
		new_Message->dest_pid = get_system_pid(KCD);
		new_Message->type = COMMAND_REG;
    
		send_message(get_system_pid(KCD), (void*) new_Message);  //register me
    
    while(1){
        new_Message = (Message*)receive_message(&sender_id);
        message_data = new_Message->data;
        if(string_equals(message_data, "%Z")){
            release_memory_block(new_Message->data);
            release_memory_block(new_Message);
            break;
        }
				else{
					  release_memory_block(new_Message->data);
            release_memory_block(new_Message);
				}
    }
    
    num = (int*) request_memory_block();
    *num = 0;
    while (1) {
        //Their pseudocode asks us to use this as a message envelope
        new_Message = (Message*)request_memory_block();  //should we be using MessageNode here?
        new_Message->type = COUNT_REPORT;
				new_Message->data = request_memory_block();
        *((int*)(new_Message->data)) =  *num;
        send_message(8, new_Message);
        *num = *num +1;
        release_processor();
    }
}


void proc8(void){ //Process B
    Message* new_Message;
    int sender_id;
    
    while(1){
        new_Message = (Message*)receive_message(&sender_id);
        send_message(9, new_Message);
    }
}

void proc9(void){ //Process C
    Message* p;
    Message* q;
		MsgNode* temp;
    char* message_data;
    int sender_id;
    int* msgNum;
    
    //Queue related data structures --were made global.
    
    MsgQueue* queue = (MsgQueue*) request_memory_block();
    queue->first = NULL;
    queue->last = NULL;
    queue->size = 0;
    
    
    while(1){
        if(queue->size == 0)
            p = (Message*)receive_message(&sender_id);
        else{
						temp = queue->first;
					  queue->first = temp->next;
					  temp->next = NULL;
						if(queue->last == temp)
							queue->last = NULL;
						queue->size--;
						p = temp->message;
        }
        
        if(p->type == COUNT_REPORT){
            msgNum = (int*)(p->data);
            if((*msgNum) % 20 ==0){
                
                string_copy(p->data, "PROCESS C");
                p->sender_pid = 9;
                p->dest_pid = get_system_pid(CRT);
                p->type = CRT_DISPLAY;
								send_message( get_system_pid(CRT), p);
                
                //hibernate for 10 seconds
                q = (Message*)request_memory_block();
                q->sender_pid = 9;
								q->dest_pid = 9;
                q->type = WAKEUP10;
                delayed_send(9, (void*)q, 1000);
                while(1){
                    p = (Message*) receive_message(&sender_id);
                    if(p->type==WAKEUP10)
                        break;
                    else{
                        //Add to local message queue
												temp = (MsgNode*)request_memory_block();
											  temp->message = p;
												temp->next = NULL;
                        if(queue->last)
													queue->last->next = temp;
												queue->last = temp;
												if(queue->first == 	NULL)
													queue->first = temp;
												queue->size++;
                    }
                }
                
            }
        }
        release_memory_block(p->data);
        release_memory_block(p);
        release_processor();
    }
}


int ProcessCount = 9;
void* ProcessTable[] = {
	&proc1,
	&proc2,
	&proc3,
	&proc4,
	&proc5,
	&proc6,
	&proc7,
	&proc8,
	&proc9
};
