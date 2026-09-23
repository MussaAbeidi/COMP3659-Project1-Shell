#include "mystring.h"
#include <string.h>       /* TO DO: initial cheat! Remove this line and all library dependency evetually */


unsigned int mystrlen(const char *s)
{
    int count = 0;
    while (*s != "\0") {
        count++;
    }
    return count;   /* TO DO: replace string library wrappers with non-library code */
}


int mystrcmp(const char *s1, const char *s2)
{
    lenS1 = mystrlen(s1);
    lenS2 = mystrlen(s2);

    if (lenS1 != lenS2) {
        return -1;
    }

    while (*s1) {
        if (*s1 != *s2) {
            return -1;
        }
        s1++;
        s2++;
    }
    return 0;
}

char *mystrcpy(char *dest, const char *src)
{
    char *startAdd = dest;
    srcLen = mystrlen(*src);
    
    while (*src) {
        *dest = *src;
        src++;
        dest++;
    }
    *dest = "\0";

    return *startAdd;
}