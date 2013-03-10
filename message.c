#include "message.h"
#include "process.h"
#include "atomic.h"

void addMessage(MessageQueue* queue, Message* msg, int delay){
	MessageNode* node = (MessageNode*)k_request_memory_block();
	atomic(0);
	node->message = msg;
	node->delay = delay;
	if (queue->first == NULL) {
		queue->first = node;
	}
	else {
		queue->last->next = node;
	}
	queue->last = node;
	node->next = NULL;
	queue->size++;
	atomic(1);
}

MessageNode* pollMessageQueue(MessageQueue* queue) {
	MessageNode* node;
	atomic(0);
	node = queue->first;
	
	if (node == NULL) {
		atomic(1);
		return NULL;
	}
	queue->first = node->next;
	if (queue->first == NULL) {
		queue->last = NULL;
	}
	node->next = NULL;
	queue->size--;
	atomic(1);
	return node;
}

// Add process to the back of the specified queue
Message* removeMessage(MessageQueue* queue, int* sender_id, int blocking) {
	volatile MessageQueue* tmp = queue;
	do {
		MessageNode* node = pollMessageQueue(queue);
		if(node != NULL) {
			Message* message = node->message;
			*sender_id = message->sender_pid;
			k_release_memory_block((void*)node);
			return message;
		}
		
		if (blocking)
			save_release_processor();
	} while (blocking);
	
	return NULL;
}

// Preserve register values
// Note: This is required for removeMessage since the value of queue is lost during release processor even if queue is defined to be volatile
__asm void save_release_processor(void)
{
	PRESERVE8
	IMPORT wait_on_message
	PUSH{r4-r11, lr}
	BL wait_on_message
	POP{r4-r11, lr}
	BX lr
}

void wait_on_message() {
	k_release_processor(MSG_WAIT);
}
