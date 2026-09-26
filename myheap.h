#ifndef MY_HEAP_H
#define MY_HEAP_H

#define HEAP_SIZE 1026     /* adjust as necessary */
static char heap[HEAP_SIZE];
static char *freep = heap;
char *alloc(unsigned int size);
void free_all();

#endif