#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "mymalloc_ll.h"

struct mem_node* GetNewNode(char* thisWord) {
	//assigns memory space
	struct mem_node* newNode = (struct mem_node*)malloc(sizeof(struct mem_node));
	newNode->mem_region = NULL;
	newNode->child = NULL;
	return newNode;
}

struct mem_node* InsertAtTail(struct mem_node* head, char* thisWord) {
	//inserts unique nodes
	struct mem_node* temp = head;
	struct mem_node* newNode = GetNewNode(thisWord);
	if(head == NULL) {
		head = newNode;
		return head;
	}
	while(temp->child != NULL){
		temp = temp->child; // Go To last Node
	}
	temp->child = newNode;
	return head;
}

// void PrintForwards(struct mem_node* head) {
// 	//prints all nodes from head down
// 	struct mem_node* temp = head;
// 	while(temp != NULL) {
// 		printf("%i %s \n", temp->count, temp->node_word);
// 		temp = temp->child;
// 	}
// 	printf("\n");
// 	return;
// }

struct mem_node* DeleteAllNodes(struct mem_node* head) {
	//deletes nodes and frees memory
	struct mem_node* current = head;
	struct mem_node* child;
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