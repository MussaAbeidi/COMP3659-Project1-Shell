#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>


#ifndef MYSH
#define MYSH
#define BUFFER_SIZE 257

char buffer[BUFFER_SIZE];
ssize_t bytes_read;

void print_prompt();
void read_input();
void print_error(const char *message, int length);
void flush_trailing_chars();
void echo_bytes(ssize_t bytes_read);
void write_error();
void check_read_error();
void check_exit_condition();
bool buffer_overflow();
void format_string();
bool buffer_overflow_detected();


#endif