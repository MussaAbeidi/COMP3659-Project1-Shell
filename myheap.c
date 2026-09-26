#include "myheap.h"
#include <stddef.h>




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