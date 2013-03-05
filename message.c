#include "message.h"
#include "process.h"

void addMessage(MessageQueue* queue, Message* msg, int delay){
	MessageNode* node = (MessageNode*)k_request_memory_block();
	__disable_irq();
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
	__enable_irq();
}

MessageNode* pollMessageQueue(MessageQueue* queue) {
	MessageNode* node;
	__disable_irq();
	node = queue->first;
	
	if (node == NULL) {
		__enable_irq();
		return NULL;
	}
	queue->first = node->next;
	if (queue->first == NULL) {
		queue->last = NULL;
	}
	node->next = NULL;
	queue->size--;
	__enable_irq();
	return node;
}

// Add process to the back of the specified queue
Message* removeMessage(MessageQueue* queue, int* sender_id) {
	while(1){
		MessageNode* node = pollMessageQueue(queue);
		if(node != NULL) {
			Message* message = node->message;
			*sender_id = message->sender_pid;
			k_release_memory_block((void*)node);
			return message;
		}
		k_release_processor(MSG_WAIT);
	}
}

