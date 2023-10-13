#ifndef dir_stack
#define dir_stack

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct dir_node {
    struct dir_node *next;
    char *data;
}dir_node;

typedef struct dir_Stack {
    struct dir_node *head;
    int stackSize;
}dir_Stack;

struct dir_Stack *newStack(void);
char *copyStr(char *str);
void push(struct dir_Stack *dir_S, char *new_address);
char *top(struct dir_Stack *dir_S);
void pop(struct dir_Stack *dir_S);
void display(struct dir_Stack *stack);

#endif