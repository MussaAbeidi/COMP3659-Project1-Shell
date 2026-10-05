#ifndef MY_STRING_H
#define MY_STRING_H

unsigned int my_strlen(const char *string_char);
int my_strcmp(const char *string1, const char *string2);
char *my_strcpy(const char *src_string, char *dest_string);
char *my_strcat(char *dest, const char *src);
int has_slash(const char *str);

#endif