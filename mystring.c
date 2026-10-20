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

char *my_strcat(char *dest, const char *src)
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

int has_slash(const char *str)
{
    while (*str) 
    {
        if (*str == '/') 
        {
            return 1;  /* a slash was found*/
        }
        str++;
    }
    return 0;  /* there was no slash found */
}

/* check if the string starts with the given prefix */
int starts_with(const char *str, const char *prefix)
{
    while (*prefix)
    {
        if (*str != *prefix)
        {
            return 0;
        }
        str++;
        prefix++;
    }
    return 1;
}

/* getting the environment variable value */
char *my_getenv(const char *name, char *envp[])
{
    if (!envp || my_strlen(name) > 62)
    {
        return NULL;
    }

    char prefix[64];
    my_strcpy(name, prefix);
    my_strcat(prefix, "=");

    for (int i = 0; envp[i] != NULL; i++)
    {
        if (starts_with(envp[i], prefix))
        {
            return envp[i] + my_strlen(prefix);
        }
    }
    return NULL;
}