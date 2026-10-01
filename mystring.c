#include "mystring.h"
#include <string.h>       /* TO DO: initial cheat! Remove this line and all library dependency evetually */
#include <stdio.h>

unsigned int my_strlen(const char *string_char)
{
    int count = 0;
    while (*string_char != '\0') 
    {
        string_char++;
        count++;
    }
    return count;   /* TO DO: replace string library wrappers with non-library code */
}


int my_strcmp(const char *string_1, const char *string_2)
{
    while (*string_1 && (*string_1 == *string_2)) 
    {
        string_1++;
        string_2++;
    }
        if (*string_1 == *string_2) 
        {
            return 0;
        }
    return 1;
}

char *my_strcpy(const char *src_string, char *dest_string)
{
    char *start_address = dest_string;
    unsigned int src_string_len = my_strlen(src_string);
    
    while (*src_string) 
    {
        *dest_string = *src_string;
        src_string++;
        dest_string++;
    }
    *dest_string = '\0';

    return start_address;
}

char *my_strcat(char *src, const char *dest)
{
  char *d = dest;

  while (*d != '\0')
    d++;

  while (*src != '\0')
    {
      *d = *src;
      d++;
      src++;
    }

  *d = '\0';

  return dest;
}