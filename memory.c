#include <LPC17xx.h>
#include "memory.h"
#include "uart_polling.h"

extern void* Image$$RW_IRAM1$$ZI$$Limit;

#define NULL 0
#define MemoryStart &Image$$RW_IRAM1$$ZI$$Limit
#define MemoryEnd 0x10007FFF

LinkedList* MemoryList;
LinkedList* FreeMemoryList;
LinkedList a;
LinkedList b;


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

void GetNewStartingAddress() {
	MemoryList->newStartingAddress = MemoryList->newStartingAddress + 32 * (uint32_t)sizeof(uint32_t) + (uint32_t)sizeof(Node);
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

void insertToList(LinkedList *list, Node* currentNode) {
	currentNode->prev = list->last;
	currentNode->next = NULL;
	if (list->last != NULL)
		list->last->next = currentNode;
	if (list->first == NULL)
		list->first = currentNode;
	list->last = currentNode;
}

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

int s_release_memory_block(void* memory_block){
		Node* temp;
	
		if(!memory_block){
			return 1; //Error
		}
		temp = MemoryList->first;
		while(temp!= NULL){
				if((uint32_t)memory_block == (uint32_t)temp + (uint32_t)sizeof(Node) ){
					removeFromList(MemoryList, temp);
					insertToList(FreeMemoryList, temp);
					return 0;
				}
				temp = temp->next;
		}
		return 1;
}

void *s_requestion_memory_block() {
	Node* currentNode;
	uint32_t block;
	
	if(FreeMemoryList->first  != NULL){
		currentNode = getBlockFromFreeLinkedList();
		block = (uint32_t)currentNode + (uint32_t)(sizeof(Node));
	}		
	//search for memory block in pool
	else if (MemoryList->newStartingAddress >= MemoryEnd){
		//free linked list -> check.
		return NULL;
	}else{
		currentNode = (Node*) MemoryList->newStartingAddress;
		block = MemoryList->newStartingAddress + (uint32_t)(sizeof(Node));
		GetNewStartingAddress();
	}
	insertToList(MemoryList, currentNode);
	
	return (void*)block;
}
