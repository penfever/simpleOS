#ifndef _LINKED_LIST_H
#define _LINKED_LIST_H

//TODO: move the function definitions to .c file

struct Node{
  char* node_word;
  int count;
  struct Node* parent;
  struct Node* child;
};

struct Node* GetNewNode(char* thisWord);

struct Node* InsertAtTail(struct Node* head, char* thisWord);

void PrintForwards(struct Node* head);

struct Node* DeleteAllNodes(struct Node* head);

#endif