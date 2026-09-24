#include "myheap.h"


#define HEAP_SIZE 10000     /* adjust as necessary */

static char heap[HEAP_SIZE];
static char *freep = heap;


char *alloc(unsigned int size)
{
  if (freep + size > heap + HEAP_SIZE) {
    return NULL;
  }

  char *ptr = freep;
  freep += size;
  return ptr;
}


void free_all()
{
  freep = heap;
}