/*Exercise 8-6. The standard library function calloc(n,size) returns a pointer to n objects of
size size, with the storage initialized to zero. Write calloc, by calling malloc or by
modifying it..*/

#include <stdlib.h>
#include <string.h>

void *my_calloc(size_t nmemb, size_t size);

#include <stdio.h>

int main(void)
{
  int *p = NULL;
  int i = 0;

  p = my_calloc(100, sizeof *p);
  if(NULL == p)
  {
    printf("my_calloc returned NULL.\n");
  }
  else
  {
    for(i = 0; i < 100; i++)
    {
      printf("%08X ", p[i]);
      if(i % 8 == 7)
      {
	printf("\n");
      }
    }
    printf("\n");
    free(p);
  }

  return 0;
}

void *my_calloc(size_t nmemb, size_t size)
{
  void *Result = NULL;

  /* use malloc to get the memory */
  Result = malloc(nmemb * size);

  /* and clear the memory on successful allocation */
  if(NULL != Result)
  {
    memset(Result, 0x00, nmemb * size);
  }

  /* and return the result */
  return Result;
}
