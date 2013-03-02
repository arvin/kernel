#ifndef MESSAGE_H
#define MESSAGE_H

#include "rtx.h"

typedef struct MessageNode {
	Message* message;
	struct MessageNode* next;
	struct MessageNode* prev;
} MessageNode;

typedef struct MessageQueue {
	MessageNode* first;
	MessageNode* last;
	int size;
} MessageQueue;

void addMessage(MessageQueue* queue, Message* msg);
Message* removeMessage(MessageQueue* queue, int sender_id);
MessageNode* removeMessageFromQueue(MessageQueue* queue, int sender_id);

#endif
