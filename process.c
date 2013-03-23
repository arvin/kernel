#include <LPC17xx.h>
#include "uart.h"
#include "process.h"
#include "userproc.h"
#include "rtx.h"
#include "lib.h"
#include "timer.h"

#ifdef DEBUG_0
#include <stdio.h>
#endif  /* DEBUG_0 */

#define STACK_SIZE_MULTIPLIER 2

ProcessQueue* readyQueue;
ProcessQueue** blockedQueues;
ProcessQueue* blockedMsgQueues;
ProcessNode* curProcess;
ProcessNode* nullProcessNode;

pcb_t* procArr[14];
ProcessNode* systemProcesses[4];
MessageQueue* msgDelayQueue;
int newProcessId = 0;				/* Must be unique */

void* print_process_memory;
void* print_process_data;
void* print_process_temp;

void null_process(void) {
	while(1) {
		release_processor();
	}
}


void keyboard_proc(void){
    Message *msg;
		char* msg_data;
	  int sender_id = get_system_pid(KCD); //random id;
		command_entry* cmd_table = request_memory_block();
		int index = 0;
		int i = 0;
	  int messageDelivered = 0;
		
		char* buffer;
		int read_command = FALSE;
		int command_index = 0;
		
		Message *temp;
		Message *crtMsg;
		char *crtData;
	
		
	
	  while(1){
			msg = (Message *)receive_message(&sender_id);
			msg_data = (char*) msg->data;

			//Check the contents of the message
			if(msg->type == COMMAND_REG){
				if(contains_prefix(msg_data, "%") && index < 16){
					for(i = 0; i < 2; i++)
						cmd_table[index].cmd[i] = msg_data[i];
					cmd_table[index].cmd[2] = '\0';
					cmd_table[index].pid = msg->sender_pid;
					index++;
				}
				release_memory_block(msg_data);
				release_memory_block(msg);
			}else if(msg->type == COMMAND) {
				messageDelivered = FALSE;
				for (i = 0; i < index; ++i) {
					if (contains_prefix(msg_data, cmd_table[i].cmd)) {
						msg->sender_pid = get_system_pid(KCD);
						msg->dest_pid = cmd_table[i].pid;
						send_message(cmd_table[i].pid, msg);
						messageDelivered = TRUE;
					}
				}
				if(!messageDelivered) {
					release_memory_block(msg_data);
					release_memory_block(msg);
				}
			}else if(msg->type == KEYBOARD_INPUT){
				if(*msg_data == '%' && !read_command){
					if (has_free_memory()) {
						read_command = TRUE;
						command_index = 1;
						
						buffer = request_memory_block();
						*buffer = *msg_data;
					}
				}else if(read_command == TRUE){
					if(command_index <BUFSIZE-1 && has_free_memory()){
						
						if(*msg_data == (char)13){ //Pressed enter, so parse command
							temp = (Message*)request_memory_block();
							for (i = 0; i < index; ++i) {
								if (contains_prefix(buffer, cmd_table[i].cmd)) {
									if(temp != NULL){
									
											//Create a new message and send it to the handler
											temp->type = COMMAND;
											temp->sender_pid = get_system_pid(KCD);
											temp->dest_pid = cmd_table[i].pid;
											temp->data = buffer; //REMEMBER TO DOUBLE CHECK ABOUT THE REF
											*(buffer + command_index) = '\0';
											send_message(cmd_table[i].pid, temp);

									}
									

								}
							}
							
							//Create a message to CRT to print a new line
							if (has_free_memory()) {
								crtMsg = (Message*)request_memory_block();
								if (has_free_memory()) {
									crtData = (char*)request_memory_block();
									*crtData = '\r';
									*(crtData+1) = '\n';
									*(crtData+2) = '\0';
									crtMsg->data = crtData;
									crtMsg->type = CRT_DISPLAY;
									crtMsg->dest_pid = get_system_pid(CRT);
									crtMsg->sender_pid = get_system_pid(KCD);
									//send_msg(get_system_pid(CRT), crtMsg, 0);
									send_message(get_system_pid(CRT), crtMsg);
									
									read_command = FALSE;
									command_index = 0;
								}
								else
									release_memory_block(crtMsg);
							}
							
						}else{
							//Add the new char to buffer
							*(buffer + command_index) = *msg_data;
							command_index++;
						}
					}

				}else{ //HOTKEY!
					if(string_equals(msg_data, "!")){
						release_memory_block(msg_data);
						release_memory_block(msg);
						print_process();
					}
				}
				
				//Print to CRT if we're putting together a command
			if (read_command == TRUE && has_free_memory()) {
				crtMsg = (Message*)request_memory_block();
				if (has_free_memory()) {
					crtData = (char*)request_memory_block();
					*crtData = *(buffer+command_index-1);
					*(crtData + 1) = '\0';
					crtMsg->data = crtData;

					crtMsg->type = CRT_DISPLAY;
					crtMsg->dest_pid = get_system_pid(CRT);
					crtMsg->sender_pid = get_system_pid(KCD);
					//send_msg(get_system_pid(CRT), crtMsg, 0);
					send_message(get_system_pid(CRT), crtMsg);
				}
				else
					release_memory_block(crtMsg);
				}
	
				
				release_memory_block(msg_data);
				release_memory_block(msg);
				
			}
		}
}

void crt_proc(){
	Message* msg;
	int sender_id;
	unsigned char* msgData;
	while(1) {
		msg = (Message*)receive_message(& sender_id);
		if(msg->type == CRT_DISPLAY){
			//send_message(get_system_pid(UART), msg);
			//k_uart_i_process();
			msgData =  (msg->data);
			uart_put_string(msgData);
		}
		release_memory_block(msg->data);
		release_memory_block(msg);
	}
}


void k_display_time(){
		int count = get_timer() / 1000;
		char* data = (char*)k_request_memory_block();
		Message* crt_message = (Message*)k_request_memory_block();
		crt_message->type = CRT_DISPLAY;
		crt_message->dest_pid = k_get_system_pid(CRT);
		crt_message->sender_pid = get_current_process_id();
		crt_message->data = (void*)data;
	
		*(data++) = '\r';
		*(data++) = (count/3600) / 10 + '0';
		*(data++) = (count/3600) % 10 + '0';
		*(data++) = ':';
		*(data++) = ((count/60)%60) / 10 + '0';
		*(data++) = ((count/60)%60) % 10 + '0';
		*(data++) = ':';
		*(data++) = (count%60) / 10 + '0';
		*(data++) = (count%60) % 10 + '0';
		*(data++) = '\r';
		*(data++) = '\0';
	
		send_msg(k_get_system_pid(CRT), crt_message, 0, NULL);

}


void process_init() {
  int i;
	// Ready queue initialization
	readyQueue = (ProcessQueue*)k_request_memory_block(	);
	readyQueue->first = NULL;
	readyQueue->last = NULL;
	readyQueue->size = 0;
	
	// Blocked queues initialization
	blockedQueues = (ProcessQueue**)k_request_memory_block();
	for (i = 0; i < PRIORITY_COUNT; ++i) {
		blockedQueues[i] = (ProcessQueue*)k_request_memory_block();
		blockedQueues[i]->first = NULL;
		blockedQueues[i]->last = NULL;
		blockedQueues[i]->size = 0;
	}

	// Initialization of blocked queues for messaging
	blockedMsgQueues = (ProcessQueue*)k_request_memory_block();
	blockedMsgQueues->first = NULL;
	blockedMsgQueues->last = NULL;
	blockedMsgQueues->size = 0;
	
	//Initialization of delayed message queue
	msgDelayQueue = (MessageQueue*)k_request_memory_block();
	msgDelayQueue->first = NULL;
	msgDelayQueue->last = NULL;
	msgDelayQueue->size = 0;
	
	// Null process initialization
	curProcess = NULL;
	nullProcessNode = (ProcessNode*)k_request_memory_block();
	init_pcb(&null_process, nullProcessNode, 4, TRUE);
	
	
	// User processes initialization
	for (i = 0; i < ProcessCount; ++i)
		add_new_prioritized_process(ProcessTable[i], 2);
		
	//Interrupt process initialization
	systemProcesses[TIMER] = (ProcessNode*)k_request_memory_block();
	init_pcb(&timer_i_process, systemProcesses[TIMER], 3, FALSE);
	systemProcesses[TIMER]->pcb.m_state = WAIT_FOR_INTERRUPT;
	
	systemProcesses[UART] = (ProcessNode*)k_request_memory_block();
	init_pcb(&uart_i_process, systemProcesses[UART], 3, FALSE);
	systemProcesses[UART]->pcb.m_state = WAIT_FOR_INTERRUPT;
	
	// System process initialization
	systemProcesses[KCD] = (ProcessNode*)k_request_memory_block();
	init_pcb(&keyboard_proc, systemProcesses[KCD], 0, TRUE);
	push_process(readyQueue, systemProcesses[KCD]);
	
	systemProcesses[CRT] = (ProcessNode*)k_request_memory_block();
	init_pcb(&crt_proc, systemProcesses[CRT], 0, TRUE);
	push_process(readyQueue, systemProcesses[CRT]);
	
	// Print process initialization
	print_process_memory = multisize_request_memory_block(1, TRUE);
	print_process_data = multisize_request_memory_block(6, TRUE);
	print_process_temp = multisize_request_memory_block(1, TRUE);
}


int k_set_process_priority(int process_ID, int priority) {
	ProcessNode* node;
	int highest_priority = 4;
	
	if(process_ID == 0 && process_ID > ProcessCount){
		return -1;
	}
		
	if (priority < 0 || priority > 3)
		return -1;
	
	//Check if the process is in blocked Queue, move it to the correct blocked queue
	node = remove_process(blockedQueues[priority], process_ID);
	if(node != NULL){
			push_process(blockedQueues[priority], node);
	}else{
			//Check if node is in ready queue and get the highest priority in ready queue
			node = readyQueue->first;
			while(node != NULL){
				if(node->pcb.priority < highest_priority && node->pcb.m_pid != process_ID)
					highest_priority = node->pcb.priority;
				node = node->next;
			}
			if(priority < highest_priority){
				node = remove_process(readyQueue, process_ID);
				if(node != NULL){
					push_process_to_front(readyQueue, node);
				}
			}
		
	}
	procArr[process_ID]->priority = priority;
	return 0;
}


int k_get_process_priority(int process_ID) {
	
	int i = 0;
	ProcessNode* node = readyQueue->first;
	
	while(node != NULL) {
		if (node->pcb.m_pid == process_ID) {
			return node->pcb.priority;
		}
		node = node->next;
	}
	
	for (i = 0; i < PRIORITY_COUNT; ++i) {
		node = blockedQueues[i]->first;
		
		while(node != NULL) {
			if (node->pcb.m_pid == process_ID) {
				return node->pcb.priority;
			}
			node = node->next;
		}
	}

	return -1;
}


// Return the process that should be executed
ProcessNode* scheduler(void){
		if (curProcess == NULL) {
			// First time executing
			curProcess = poll_process(readyQueue);
			return curProcess;
		}
		else if (readyQueue->first != NULL) {
			return poll_process(readyQueue);
		}
		return nullProcessNode;
}

void init_pcb(void* process, ProcessNode* node, int priority, int isStackRequired) {
	node->next = NULL;
	
	// PCB initialization
	procArr[newProcessId] = &(node->pcb);
	node->pcb.m_pid = newProcessId++;
	
	node->pcb.priority = priority;
	node->pcb.m_state = NEW;
	
	node->pcb.msgQueue.first = NULL;
	node->pcb.msgQueue.last = NULL;
	node->pcb.msgQueue.size = 0;
	
	if (isStackRequired)
		init_proc_stack(process, node);
	else {
		node->pcb.mp_sp = NULL;
		node->pcb.stack_boundary = NULL;
	}
}


void init_proc_stack(void* process, ProcessNode* node) {
	int i;
	uint32_t* stackBlockStart = (uint32_t*)multisize_request_memory_block(STACK_SIZE_MULTIPLIER, FALSE);
	
	// Stack initialization
	node->pcb.stack_boundary = stackBlockStart;
	node->pcb.mp_sp = stackBlockStart + MEMORY_BLOCK_SIZE * STACK_SIZE_MULTIPLIER;
	node->pcb.mp_sp--;
	/* 8 bytes alignment adjustment to exception stack frame */
	if (!(((uint32_t)node->pcb.mp_sp) & 0x04)) {
			node->pcb.mp_sp--;
	}
	
	*(--(node->pcb.mp_sp))  = INITIAL_xPSR;      /* user process initial xPSR */ 
	*(--(node->pcb.mp_sp))  = (uint32_t) process;  /* PC contains the entry point of the process */

	for (i = 0; i < 6; i++) { /* R0-R3, R12 are cleared with 0 */
		*(--(node->pcb.mp_sp)) = 0x0;
	}
}


// This is an API call for creating new user processes
int k_add_new_process(void* process) {
	return add_new_prioritized_process(process, 3);
}


// This is an internal call for creating new processes
int add_new_prioritized_process(void* process, int priority) {
	ProcessNode* node = (ProcessNode*)k_request_memory_block();
	push_process(readyQueue, node);
	init_pcb(process, node, priority, TRUE);
	return 0;
}


// This is an API call for releasing current user process
int k_voluntarily_release_processor() {
	return k_release_processor(RDY);
}

__asm void switch_proc()
{
	PRESERVE8
	IMPORT switch_process
	PUSH{r0-r11, lr}
	BL switch_process
	POP{r0-r11, pc}
	BX lr
} 

// This is an internal API call for releasing current process with a new specified state
int k_release_processor(proc_state_t newState) {
	if (curProcess != NULL)
	curProcess->pcb.m_state = newState;

	switch (newState) {
		case RDY:
			if (curProcess != NULL)
				push_process(readyQueue, curProcess);
			break;
		case INSUFFICIENT_MEMORY:
			push_process(blockedQueues[curProcess->pcb.priority], curProcess);
			break;
		case MSG_WAIT:
			push_process(blockedMsgQueues, curProcess);
			break;
	}
	
	switch_proc();
	return 0;
}



// Remove first process from the specified queue
ProcessNode* poll_process(ProcessQueue* queue) {
	ProcessNode* node = queue->first;
	if (node == NULL) {
		return NULL;
	}
	queue->first = node->next;
	if (queue->first == NULL) {
		queue->last = NULL;
	}
	node->next = NULL;
	queue->size--;
	return node;
}


// Remove process from the specified queue with specified process ID
ProcessNode* remove_process(ProcessQueue* queue, int pid) {
	ProcessNode* node = queue->first;
	ProcessNode* prev = NULL;
	while (node != NULL) {
		if (node->pcb.m_pid == pid) {
			if (prev != NULL)
				prev->next = node->next;
			if (queue->first == node)
				queue->first = node->next;
			if (queue->last == node)
				queue->last = prev;
			
			queue->size--;
			return node;
		}
		prev = node;
		node = node->next;
	}
	return NULL;
}

void shift_ready_process(int pid) {
	ProcessNode* node = remove_process(readyQueue,  pid);
	if(node != NULL){
		push_process(readyQueue, node);
	}
}


// Add process to the back of the specified queue
void push_process(ProcessQueue* queue, ProcessNode* node) {
	if (queue->first == NULL) {
		queue->first = node;
	}
	else {
		queue->last->next = node;
	}
	queue->last = node;
	node->next = NULL;
	queue->size++;
}


// Add process to the front of the specified queue
void push_process_to_front(ProcessQueue* queue, ProcessNode* node) {
	if (queue->first == NULL) {		
		queue->last = node;
	}
	node->next = queue->first;
	queue->first = node;
	queue->size++;
}


// Internal call for releasing processor
	int switch_process(void) {
	volatile int i;
	volatile proc_state_t state;
	ProcessNode *oldProcess = NULL;
	ProcessNode *newProcess = NULL;

	newProcess = scheduler();
	if (curProcess == NULL) {
	 return -1;
	}
	
	oldProcess = curProcess;
	curProcess = newProcess;

	state = curProcess->pcb.m_state;

	if (curProcess->pcb.mp_sp == NULL) {
		curProcess = oldProcess;
		uart_put_string("Kernel Error: Attempt to switch to a process without a stack.\n\rProcess ID:\n\r");
		uart_put_hex(curProcess->pcb.m_pid);
		return -1;
	} else if (state == NEW) {
		if (oldProcess->pcb.m_state != NEW) {
			oldProcess->pcb.mp_sp = (uint32_t *) __get_MSP();
		}
		curProcess->pcb.m_state = RUN;
		__set_MSP((uint32_t) curProcess->pcb.mp_sp);
		__rte();  /* pop exception stack frame from the stack for a new process */
	} else if (state == RDY) {   
		oldProcess->pcb.mp_sp = (uint32_t *) __get_MSP(); /* save the old process's sp */
		curProcess->pcb.m_state = RUN;
		__set_MSP((uint32_t) curProcess->pcb.mp_sp); /* switch to the new proc's stack */		
	} else {
		curProcess = oldProcess; /* revert back to the old proc on error */
		uart_put_string("Kernel Error: Failed to switch process.\n\rProcess ID:\n\r");
		uart_put_hex(curProcess->pcb.m_pid);
		return -1;
	}
	return 0;
}


// If memory is available, unblock the process with the highest priority
void unblock_process() {
	int i;
	ProcessNode* node;
	if (!k_has_free_memory(1))
		return;
	
	for (i = 0; i < PRIORITY_COUNT; ++i) {
		node = poll_process(blockedQueues[i]);
		if (node != NULL) {
			node->pcb.m_state = RDY;
			push_process_to_front(readyQueue, node);
			
			//Preemption
			if(curProcess->pcb.priority > node->pcb.priority) {
				k_voluntarily_release_processor();
			}
			
			return;
		}
	}
}


int k_send_message(int process_ID, void *messageEnvelope) {
	return send_msg(process_ID, messageEnvelope, 1, NULL);
}


int send_msg(int process_ID, void *messageEnvelope, int allowPreempt, void* system_reserved_block) {
	ProcessNode* node;
	pcb_t* target = procArr[process_ID];
	Message* msg = (Message*)messageEnvelope;
	addMessage(&(target->msgQueue), msg, 0, system_reserved_block);
	
	node = remove_process(blockedMsgQueues, process_ID);
	if (node != NULL) {
		// Receiver is in blocked queue
		node->pcb.m_state = RDY;
		push_process_to_front(readyQueue, node);
		
		//Preemption
		if(allowPreempt && curProcess->pcb.priority > node->pcb.priority) {
			k_voluntarily_release_processor();
		}
	}
	return 0;
}



int k_delayed_send(int process_ID, void *MessageEnvelope, int delay){
	Message* msg = (Message*)MessageEnvelope;
	addMessage(msgDelayQueue, msg, delay, NULL);
	return 0;
}


void* k_receive_message(int* sender_id) {
    return removeMessage(&(curProcess->pcb.msgQueue), sender_id, 1);
}

void* system_proc_receive_message(system_proc_type type){
		int sender_id;
    return removeMessage(&(systemProcesses[type]->pcb.msgQueue), &sender_id, 0);
}


int k_dec_delay_msg_time(){
	MessageNode* temp;
	MessageNode* node = msgDelayQueue->first;
	MessageNode* 	prev = msgDelayQueue->first;
	Message* msg ;
	int preemptPid = 0;
	int highPriority = curProcess->pcb.priority;
	
	while(node!=NULL){
		node->delay--;
		if(node->delay <=0){
			
			msg = node->message;
			if(procArr[msg->dest_pid]->priority < highPriority){
				highPriority = procArr[msg->dest_pid]->priority;
				preemptPid = msg->dest_pid;
			}
			
			temp = node;
			if((node == msgDelayQueue->first) || (node == msgDelayQueue->last)){
				if(msgDelayQueue->first == msgDelayQueue->last){
					msgDelayQueue->first = NULL;
					msgDelayQueue->last = NULL;
				}else{
					if(node == msgDelayQueue->first){
						msgDelayQueue->first = node->next;
						prev = node->next;
					}
					if(node == msgDelayQueue->last){
						msgDelayQueue->last = prev;
						prev->next = NULL;
					}
				}
			}
			else{
				prev->next = node->next;
			}
			node = node->next;
			msgDelayQueue->size--;
			k_release_memory_block(temp);
			send_msg(msg->dest_pid, (void*)msg, 0, NULL);
		}
		else{
			prev = node;
			node = node->next;
		}
	}
	
	return preemptPid;
}


int k_get_system_pid(system_proc_type type){
  return systemProcesses[type]->pcb.m_pid;
}

uint32_t get_current_process_id() {
	return curProcess ? curProcess->pcb.m_pid : 0;
}

void set_process_state(uint32_t process_ID, proc_state_t state) {
	procArr[process_ID]->m_state = state;
}

void k_print_process(){
		Message* crt_message = (Message*)print_process_memory;
		char* data = (char*)print_process_data;
		char* str = (char*)print_process_temp;
		char* sendTo = data;
	
		int curCounter = 0;
		int i = 0;
		int anyBlockedQueues = 0;
		int anyBlockedRecieveQueues = 0;
		ProcessNode* curr;
		clearString(str);
		curr = readyQueue->first;
				data = append_to_block((char*)data,(char*)"Ready Queue:\n\r");
		while(curr != NULL){
			data = append_to_block((char*)data, "\r PID: ");
			*str = curr->pcb.m_pid + '0';
			data = append_to_block(data, str); //Note might have to double check in case it's 2 digit
			data = append_to_block((char*)data, "\n\r Priority: ");
			*str = curr->pcb.priority + '0';
			data = append_to_block((char*)data, str);
			data = append_to_block((char*)data, "\n\r");
			data = append_to_block((char*)data, "\n\r");
			curr = curr->next;
			curCounter++;
		}
		clearString(str);
		data = append_to_block((char*)data,(char*)"\n\rBlocked Memory Queue:\n\r");
	for (i = 0; i < PRIORITY_COUNT; ++i){
		curr = blockedQueues[i]->first;
		while(curr!=NULL){
			anyBlockedQueues = 1;
			data = append_to_block((char*)data, "\r PID: ");
			*str = curr->pcb.m_pid + '0';
			data = append_to_block(data, str); //Note might have to double check in case it's 2 digit
			data = append_to_block((char*)data, "\n\r Priority: ");
			*str = curr->pcb.priority + '0';
			data = append_to_block((char*)data, str);
			data = append_to_block((char*)data, "\n\r");
			data = append_to_block((char*)data, "\n\r");
			curr = curr->next;
		}
	}
	
	if(anyBlockedQueues == 0){
		data = append_to_block((char*)data,(char*)"No Blocked Queues!\n\r\n\r");
	}
	
	
	clearString(str);
		data = append_to_block((char*)data,(char*)"Blocked Msg Queue:\n\r\n\r");
		
		curr = blockedMsgQueues->first;
		while(curr!=NULL){
				clearString(str);
				anyBlockedRecieveQueues = 1;
				data = append_to_block((char*)data, "\r PID: ");
				if (curr->pcb.m_pid >= 10){
					*(str) = curr->pcb.m_pid / 10 + '0';
					*(str+1) = curr->pcb.m_pid % 10 + '0';
				}else{
					*str = curr->pcb.m_pid % 10 + '0';
				}
				data = append_to_block(data, str); //Note might have to double check in case it's 2 digit
				data = append_to_block((char*)data, "\n\r\r Priority: ");
				clearString(str);
				*str = curr->pcb.priority + '0';
				data = append_to_block((char*)data, str);
				data = append_to_block((char*)data, "\n\r");
				curr = curr->next;
		}
	
		if(anyBlockedRecieveQueues == 0){
			data = append_to_block((char*)data,(char*)"No Blocked Recieve Queue's!\n\r\n\r");
		}
	
		eos((char*)data);
		crt_message->type = CRT_DISPLAY;
		crt_message->dest_pid = k_get_system_pid(CRT);
		crt_message->sender_pid = k_get_system_pid(KCD);
		crt_message->data = (void*)sendTo;
		send_msg(k_get_system_pid(CRT), crt_message,0, str);
}

char* append_to_block(char* block, char* str){
	int i = 0;
	int sizeOfString = 0;
	char* data = block;
	sizeOfString = string_len(str);
	for(i = 0; i < sizeOfString; i++){
	   *(data++) = *(str+i);
	}
	return data;

}

void eos(char* block){
	*(block++) = '\0';
}

void clearString(char* str){
	int i = 0;
	for(i = 0;i<15;i++){
			*(str+i) = '\0';
	}
	*(str+15) = '\0';
}
