#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "mymalloc.h"
#include "mymalloc_ll.h"

struct mem_region* GetNewNode(int size) {
	//assigns memory space
	struct mem_region* newNode = (struct mem_region*)malloc(sizeof(struct mem_region)); //fix this malloc call, instead allocate memory from existing region
	newNode->free = TRUE;
    newNode->size = size; //TODO: allocate memory from existing region
    newNode->pid = 1;
	newNode->data[0] = NULL;
	return newNode;
}

struct mem_region* InsertAtTail(struct mem_region* first, int size) {
	//inserts unique nodes
	struct mem_region* temp = first;
	struct mem_region* newNode = GetNewNode(size);
	// if(first == NULL) {
	// 	first = newNode;
	// 	return first;
	// }
	while(temp->data[0] != NULL){
		temp = temp->data[0]; // Go To last Node
	}
	temp->data[0] = &newNode; // TODO: this isn't correct. the new node is floating somewhere in the mem region
	return head;
}

// void PrintForwards(struct mem_region* head) {
// 	//prints all nodes from head down
// 	struct mem_region* temp = head;
// 	while(temp != NULL) {
// 		printf("%i %s \n", temp->count, temp->node_word);
// 		temp = temp->child;
// 	}
// 	printf("\n");
// 	return;
// }

struct mem_region* DeleteAllNodes(struct mem_region* head) {
	//deletes nodes and frees memory
	struct mem_region* current = head;
	struct mem_region* child;
	if(current == NULL) {
		return NULL;
	}
	while(current != NULL) {
		child = current->child;
		free(current->mem_region);
		free(current);
		current = child;
	}
	return NULL;
}