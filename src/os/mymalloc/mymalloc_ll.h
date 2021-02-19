#ifndef _MYMALLOC_LL_H
#define _MYMALLOC_LL_H

struct mem_node {
    struct mem_region* mem_region;
    struct mem_node* child;
};

struct mem_node* GetNewNode(char* thisWord);

struct mem_node* InsertAtTail(struct mem_node* head, char* thisWord);

// void PrintForwards(struct mem_node* head);

struct mem_node* DeleteAllNodes(struct mem_node* head);

#endif