#ifndef MESSAGE_H
#define MESSAGE_H

#include "rtx.h"

typedef struct MessageNode {
	Message* message;
	struct MessageNode* next;
	int delay;
} MessageNode;

typedef struct MessageQueue {
	MessageNode* first;
	MessageNode* last;
	int size;
} MessageQueue;

int addMessage(MessageQueue* queue, Message* msg, int delay, void* system_reserved_block);
Message* removeMessage(MessageQueue* queue, int* sender_id, int blocking);
MessageNode* pollMessageQueue(MessageQueue* queue);
__asm void save_release_processor(void);
void wait_on_message(void);

#endif
