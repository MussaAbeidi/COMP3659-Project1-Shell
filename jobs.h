#include <unistd.h>

#ifndef JOBS_H
#define JOBS_H
#define MAX_ARGS 20
#define BUFFER_SIZE 257
#define PATH_LEN 256
#define BUFFER_OVERFLOW 2
#define READ_ERROR 1
#define EXIT 3
#define HEAP_EXCEEDED 4
#define MAX_PIPELINE_LEN 10
#define MAX_TOKENS 64
#define PARSE_ERROR 10
#define EMPTY_LINE  11

struct Command
{
  char *argv[MAX_ARGS+1];
  unsigned int argc;
  // unsigned int background_status;
};


struct Job
{
  struct Command pipeline[MAX_PIPELINE_LEN];
  unsigned int num_stages;
  char *outfile_path;   /* NULL if no > */
  char *infile_path;    /* NULL if no < */
  int background;       /* 0 = foreground */
};

// int get_command(struct Command *command);
void run_child_process(struct Command *command, char *envp[]);
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
int tokenize_string();
int get_job(struct Job *job);
void run_job(struct Job *job, char *envp[]);
pid_t spawn(struct Command *command, int in_fd, int out_fd, int unused_fd, char *envp[]);
void cleanup_zombies(void);

#endif