#include <unistd.h>

#ifndef JOBS_H
#define JOBS_H
#define MAX_ARGS 20
#define BUFFER_SIZE 257
#define BUFFER_OVERFLOW 2
#define READ_ERROR 1
#define EXIT 3
#define HEAP_EXCEEDED 4

struct Command
{
  char *argv[MAX_ARGS+1];
  unsigned int argc;
  unsigned int background_status;
};

int get_command(struct Command *command);
void run_command(struct Command *command);
void run_child_process(struct Command *command);
void wait_for_foreground_process(pid_t pid, struct Command *command);
void print_prompt();
void read_input();
void print_error(const char *message, int length);
void flush_trailing_chars();
void echo_bytes(ssize_t bytes_read);
void echo_error();
int check_read_error();
int check_exit_condition();
int buffer_overflow();
void format_string();
int buffer_overflow_detected();
int tokenize_string(struct Command *command);


#endif