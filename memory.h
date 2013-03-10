#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <LPC17xx.h>

typedef struct Node Node;
typedef struct LinkedList LinkedList;

struct Node;
struct LinkedList;

struct Node
{
	Node* next;
	Node* prev;
};

struct LinkedList
{
	Node* first;
	Node* last;
	uint32_t newStartingAddress;
};

void memory_init(void);

void setNewStartingAddress(int multiplier);

Node* getBlockFromFreeLinkedList(void);

void insertToList(LinkedList *list, Node* currentNode);
void removeFromList(LinkedList *list, Node* temp);

int k_release_memory_block(void* memory_block);

void* k_persistent_request_memory_block(void);
void* k_request_memory_block(void);
void* multisize_request_memory_block(int multiplier);

int hasFreeMemory(int multiplier);
int hasUnusedMemory(int multiplier); // Determine if FreeMemory has any unused memory

#endif
