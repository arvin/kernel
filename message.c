#include "message.h"
#include "process.h"

void addMessage(MessageQueue* queue, Message* msg){
	MessageNode* node = (MessageNode*)k_request_memory_block();
	__disable_irq();
	node->message = msg;
	
	if (queue->first == NULL) {
		queue->first = node;
		node->prev = NULL;
	}
	else {
		node->prev = queue->last;
		queue->last->next = node;
	}
	queue->last = node;
	node->next = NULL;
	queue->size++;
	__enable_irq();
}

MessageNode* removeMessageFromQueue(MessageQueue* queue, int sender_id){
	MessageNode *node = queue->first;
		__disable_irq();
		while(node != NULL){
			if(node->message->sender_pid == sender_id){
				if(node->prev != NULL){
					node->prev->next = node->next;
				}
				
				if(node->next != NULL){
					node->next->prev = node->prev;
				}
				
				if(node == queue->first){
					queue->first = NULL;
				}
				if(node == queue->last){
					queue->last = NULL;
				}
				queue->size--;
				
				__enable_irq();
				return node;
			}
			node = node->next;
		}
		
		__enable_irq();
		return NULL;
}

// Add process to the back of the specified queue
Message* removeMessage(MessageQueue* queue, int sender_id) {
	while(1){
		MessageNode* node = removeMessageFromQueue(queue, sender_id);
		if(node != NULL) {
			Message* message = node->message;
			k_release_memory_block((void*)node);
			return message;
		}
		k_release_processor(MSG_WAIT);
	}
}

