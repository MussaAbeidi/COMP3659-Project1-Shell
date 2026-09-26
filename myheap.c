#include "myheap.h"
#include <stddef.h>


#define HEAP_SIZE 1026     /* adjust as necessary */

static char heap[HEAP_SIZE];
static char *freep = heap;


char *alloc(unsigned int size)
{
  if (freep + size > heap + HEAP_SIZE) 
  {
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