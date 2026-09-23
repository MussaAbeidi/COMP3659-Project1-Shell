#include "myheap.h"


#define HEAP_SIZE 10000     /* adjust as necessary */

static char heap[HEAP_SIZE];
static char *freep = heap;


char *alloc(unsigned int size)
{
  if (size > HEAP_SIZE) {
    return -1;
  }
  int i = 0;
  char *assignedStart[i]; 
  
  for (i = 0; i < HEAP_SIZE; i++) {
    
  }
  return 0;     /* TO DO */
}


void free_all()
{
  /* TO DO */
}