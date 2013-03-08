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

void addMessage(MessageQueue* queue, Message* msg, int delay);
Message* removeMessage(MessageQueue* queue, int* sender_id);
MessageNode* pollMessageQueue(MessageQueue* queue);
__asm void save_release_processor(void);
void wait_on_message(void);

#endif
