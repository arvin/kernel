#include <LPC17xx.h>
#include "memory.h"
#include "process.h"
#include "uart.h"
#include "atomic.h"
#include "rtx.h"

extern void* Image$$RW_IRAM1$$ZI$$Limit;

#define MemoryStart &Image$$RW_IRAM1$$ZI$$Limit
#define MemoryEnd 0x10007FFF

LinkedList* MemoryList;
LinkedList* FreeMemoryList;
LinkedList a;				// MemoryList
LinkedList b;				// FreeMemoryList


void memory_init() {
	MemoryList = &a;
	FreeMemoryList = &b;
	MemoryList->first = NULL;
	MemoryList->last = NULL;
	MemoryList->newStartingAddress = (uint32_t)MemoryStart;
	FreeMemoryList->first = NULL;
	FreeMemoryList->last = NULL;
	FreeMemoryList->newStartingAddress = (uint32_t)MemoryStart;
}

void setNewStartingAddress(int multiplier) {
	// Notice that total size of a block is 128 bytes plus the size of a node structure
	MemoryList->newStartingAddress = MemoryList->newStartingAddress + MEMORY_BLOCK_SIZE * (uint32_t)sizeof(uint32_t) * multiplier + (uint32_t)sizeof(Node);
}

Node* getBlockFromFreeLinkedList() {
	if(FreeMemoryList->first != NULL ){
		Node* temp = FreeMemoryList->first;
		FreeMemoryList->first = temp->next;
		if(FreeMemoryList->first != NULL){
				FreeMemoryList->first->prev = NULL;
		}else{
			FreeMemoryList->last = NULL;
		}
		return temp;
	}
	return NULL;
}

// Insert memory block to specified memory list
void insertToList(LinkedList *list, Node* currentNode) {
	currentNode->prev = list->last;
	currentNode->next = NULL;
	if (list->last != NULL)
		list->last->next = currentNode;
	if (list->first == NULL)
		list->first = currentNode;
	list->last = currentNode;
}

// Remove memory block from the specified memory list
void removeFromList(LinkedList *list, Node* temp) {
			if(temp == list->first){
				list->first=temp->next;
			}
			if(temp == list->last){
				list->last = temp->prev;
			}
			
			if(temp->prev)
				temp->prev->next = temp->next;
			if(temp->next)
				temp->next->prev = temp->prev;
			temp->next = NULL;
			temp->prev = NULL;

}

int k_release_memory_block(void* memory_block){
		Node* temp;
		atomic(0);
		if(!memory_block){
			atomic(1);
			return -1; //Error
		}
		temp = MemoryList->first;
		while(temp!= NULL){
				if((uint32_t)memory_block == (uint32_t)temp + (uint32_t)sizeof(Node) ){
					if (temp->is_system_reserved) {
						atomic(1);
						return 0;
					}
					removeFromList(MemoryList, temp);
					insertToList(FreeMemoryList, temp);
					// Unblock any processes if possible
					unblock_process();
					atomic(1);
					return 0; //Success
				}
				temp = temp->next;
		}
		atomic(1);
		return -1; //Error
}

// An API call for memory request
void *k_persistent_request_memory_block() {
	void* block = k_request_memory_block();
	while (block == NULL) {
		// Process will be blocked if there isn't any sufficient memory
		k_release_processor(INSUFFICIENT_MEMORY);
		// Retry memory block request
		block = k_request_memory_block();
	}
	return block;
}

void* k_request_memory_block() {
	return multisize_request_memory_block(1, FALSE);
}

// An internal API call for memory request
void* multisize_request_memory_block(int size_multiplier, int is_system_reserved) {

	Node* currentNode;
	uint32_t block;
	atomic(0);
	// Check if any memory has been freed
	if(FreeMemoryList->first != NULL && size_multiplier == 1) {
		currentNode = getBlockFromFreeLinkedList();
		block = (uint32_t)currentNode + (uint32_t)(sizeof(Node));
	}		
	//search for memory block in pool
	else if (hasUnusedMemory(size_multiplier)){
		// No more memory
		atomic(1);
		return NULL;
	}
	else {
		// Expand MemoryList
		currentNode = (Node*) MemoryList->newStartingAddress;
		block = MemoryList->newStartingAddress + (uint32_t)(sizeof(Node));
		setNewStartingAddress(size_multiplier);
	}
	currentNode->is_system_reserved = is_system_reserved;
	insertToList(MemoryList, currentNode);
	atomic(1);
	return (void*)block;
}

int k_has_free_memory(int multiplier) {
	return FreeMemoryList->first != NULL || hasUnusedMemory(multiplier);
}

// Determine if MemoryList can be expanded or not
int hasUnusedMemory(int multiplier) {
	return (MemoryList->newStartingAddress + MEMORY_BLOCK_SIZE * (uint32_t)sizeof(uint32_t) * multiplier + (uint32_t)sizeof(Node) >= MemoryEnd);
}
