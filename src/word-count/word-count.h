#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef struct Node{
  char* node_word;
  struct Node* parent;
  struct Node* child;
};

//assigns memory space
struct Node* GetNewNode(char* thisWord) {
	struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
	newNode->node_word = thisWord;
	newNode->parent = NULL;
	newNode->child = NULL;
	return newNode;
}

//only inserts unique nodes
struct Node* InsertAtTail(struct Node* head, char* thisWord) {
	struct Node* temp = head;
	struct Node* newNode = GetNewNode(thisWord);
	if(head == NULL) {
		head = newNode;
		return head;
	}
	while(temp->child != NULL){
		if (strcmp(temp->node_word, thisWord) == 0){
			return head;
		}
		else{
			temp = temp->child; // Go To last Node
		}
	}
	temp->child = newNode;
	newNode->parent = temp;
	return head;
}

void PrintForwards(struct Node* head) {
	struct Node* temp = head;
	while(temp != NULL) {
		printf("%s ",temp->node_word);
		temp = temp->child;
	}
	printf("\n");
	return;
}

struct Node* DeleteAllNodes(struct Node* head) {
	struct Node* current = head;
	struct Node* child;
	if(current == NULL) {
		return NULL;
	}
	while(current != NULL) {
		child = current->child;
		free(current->node_word);
		free(current);
		current = child;
	}
	return NULL;
}