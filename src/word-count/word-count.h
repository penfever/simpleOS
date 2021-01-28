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

struct Node* InsertAtTail(struct Node* head, char* thisWord) {
	struct Node* temp = head;
	struct Node* newNode = GetNewNode(thisWord);
	if(head == NULL) {
		head = newNode;
		return head;
	}
	while(temp->child != NULL){
		temp = temp->child; // Go To last Node
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
	struct Node* temp = head;
	if(head == NULL) {
		return head;
	}
	while(temp->child != NULL) {
		free(temp);
		temp = temp->child;
	}
}