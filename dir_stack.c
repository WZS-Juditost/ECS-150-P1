#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dir_stack.h"

struct dir_Stack *newStack(void)  {
    struct dir_Stack *stack = malloc(sizeof *stack);
    if (!stack) {
        return stack;
    }
    stack->head = NULL;
    stack->stackSize = 0;
    return stack;
}

char *copyStr(char *str) {
    char *tmp = malloc(strlen(str) + 1);
    if (tmp)
        strcpy(tmp, str);
    return tmp;
}

void push(struct dir_Stack *dir_S, char *new_address) {
    struct dir_node *new_node = malloc(sizeof *new_node); 
    if (!new_node) {
        return;
    }
    new_node->data = copyStr(new_address);
    new_node->next = dir_S->head;
    dir_S->head = new_node;
    dir_S->stackSize++;
}


char *top(struct dir_Stack *dir_S) {
    if(!(dir_S && dir_S->head)) {
        return NULL;
    }    
    return dir_S->head->data;
}

void pop(struct dir_Stack *dir_S) {
    if (dir_S->head != NULL) {
        struct dir_node *temp = dir_S->head;
        dir_S->head = dir_S->head->next;
        free(temp->data);
        free(temp);
        dir_S->stackSize--;
    }
}

void display(struct dir_Stack *stack) {
    for(struct dir_node *curr = stack->head; curr; curr = curr->next) {
        printf("%s\n", curr->data);
    }
}