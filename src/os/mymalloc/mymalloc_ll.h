#ifndef _MYMALLOC_LL_H
#define _MYMALLOC_LL_H

struct mem_node {
    struct mem_region* mem_region;
    struct mem_node* child;
};

struct mem_node* GetNewNode(char* mem_array, int size);

struct mem_node* InsertAtTail(struct mem_node* head, char* mem_array, int size);

// void PrintForwards(struct mem_node* head);

struct mem_node* DeleteAllNodes(struct mem_node* head);

#endif