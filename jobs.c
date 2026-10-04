#include <unistd.h>
#include "jobs.h"
#include "parser.h"
#include "myheap.h"
#include "mystring.h"
#include <sys/wait.h>
#include <fcntl.h>

const char error_message[] = "Error: Command line exceeds maximum length (255) allowed.\n";
char buffer[BUFFER_SIZE];
ssize_t bytes_read;
char *tokens[MAX_TOKENS];
int num_tokens;
// int pos; parser has its own

void reset_bytes_read()
{
    bytes_read = 0;
}

void print_prompt()
{   
    const char *prompt = "$ ";
    write(1, prompt, my_strlen(prompt));
}

// Subtracting 1 from the size to ensure we don't account for the terminating character
void read_input()
{
    bytes_read = read(0, buffer, BUFFER_SIZE - 1);
}

void print_error(const char *message, int length)
{
    write(2, message, length);
}

void flush_trailing_chars(){
    char flush_char;
    while (read(0,&flush_char,1) > 0 && flush_char != '\n')
    {
    }
}

void echo_bytes(ssize_t bytes_read)
{
    write(1, buffer, bytes_read);
    write(1, "\n", 1);
}

void echo_error()
{
    write(1, error_message, my_strlen(error_message));
}

int check_read_error()
{
    // Terminating the program read system call failure if bytes_read euqal to -1 so the shell will not crash
    if (bytes_read == -1)
    {   //printf("Bytes read: %i\n", bytes_read);
        return 1;
    }
    return 0;
}

int check_exit_condition()
{
    if (my_strcmp(buffer, "exit") == 0)
    {
        return 1;
    }    
    return 0;
}

int buffer_overflow(){
    if (bytes_read == BUFFER_SIZE - 1 && buffer[bytes_read - 1] != '\n')
    {
        return 1;
    }
    return 0;
}

void format_string(){
    if (bytes_read > 0 && buffer[bytes_read - 1] == '\n')
    {
        buffer[bytes_read - 1] = '\0';
        bytes_read--;
    } 
    else if (bytes_read > 0)
    {
        buffer[bytes_read] = '\0';
    }
}

// Safety measure for when there is a buffer overflow when the user typed more than what was allowewd
int buffer_overflow_detected()
{
    if (buffer_overflow())
    {
        // Let the user know too many characters inputted
        echo_error();
        // Get rid of trailing characters overflowing from buffer
        flush_trailing_chars();
        return 1;
    }    
    return 0;
}

int tokenize_string()                                  /* CHANGED */
{
    char *start = buffer;
    char *end;
    num_tokens = 0;                                    /* CHANGED */

    // Skip over leading whitespace
    while(*start && (*start == ' ' || *start == '\t'))
    {   
        start++;
    }
    // Empty string inputted
    if (!*start) {
        num_tokens = 0;                                /* CHANGED */
        return 0;
    }
    while(*start && num_tokens < MAX_TOKENS)           /* CHANGED */
    {
        end = start;
        while (*end && (*end != ' ' && *end != '\t')) 
        {
            end++;
        }
        int token_length = end - start;
        char *token = alloc(token_length + 1);
        // Trying to exceed heap limit
        if (token == NULL)
        {
            return HEAP_EXCEEDED;
        }
        char temp = *end;
        *end = '\0';

        my_strcpy(start, token);
        *end = temp;
        start = end;
        tokens[num_tokens] = token;                    /* CHANGED */
        num_tokens++;                                  /* CHANGED */
        while (*start && (*start == ' ' || *start == '\t')) 
        {
            start++;
        }

    }
    return 0;                                          /* removed the argv NULL line */
}

/* Runs in the child: builds the program's path and execs it */
void run_child_process(struct Command *command)
{
    char path[PATH_LEN];

    if (has_slash(command->argv[0]))
    {
        my_strcpy(command->argv[0], path);          /* already a path: use it as-is */
    }
    else
    {
        my_strcpy("/usr/bin/", path);               /* "ls" becomes "/usr/bin/ls" */
        my_strcat(path, command->argv[0]);
    }

    execve(path, command->argv, NULL);

    /* only reached if execve failed */
    const char exec_error[] = "Error: Command execution failed.\n";
    print_error(exec_error, my_strlen(exec_error));
    _exit(1);
}


int get_job(struct Job *job)    // changed
{
    free_all();
    reset_bytes_read();
    print_prompt();
    read_input();

    if (check_read_error())
    {
        return READ_ERROR;
    }
    if (buffer_overflow_detected())
    {
        return BUFFER_OVERFLOW;
    }

    format_string();

    if (check_exit_condition())
    {
        return EXIT;
    }
    if (tokenize_string() != 0)
    {
        return HEAP_EXCEEDED;
    }
    if (num_tokens == 0)
    {
        return EMPTY_LINE;
    }

    return parse_job(job);
}

void run_job(struct Job *job)
{
    pid_t pids[MAX_PIPELINE_LEN];
    unsigned int started = 0;
    unsigned int i;
    int in_fd = 0;          /* input for the current stage (0 = keyboard) */
    int out_fd;             /* output for the current stage */
    int next_in_fd;         /* pipe read end, saved for the next stage */
    int fds[2];

    /* < : the first stage reads from the input file */
    if (job->infile_path != NULL)
    {
        in_fd = open(job->infile_path, O_RDONLY);
        if (in_fd < 0)
        {
            const char in_error[] = "Error: cannot open input file.\n";
            print_error(in_error, my_strlen(in_error));
            return;
        }
    }

    for (i = 0; i < job->num_stages; i++)
    {
        next_in_fd = -1;

        if (i < job->num_stages - 1)
        {
            /* not the last stage: output goes into a new pipe */
            if (pipe(fds) < 0)
            {
                const char pipe_error[] = "Error: cannot create pipe.\n";
                print_error(pipe_error, my_strlen(pipe_error));
                break;
            }
            out_fd = fds[1];            /* this stage writes into the pipe */
            next_in_fd = fds[0];        /* the next stage will read from it */
        }
        else if (job->outfile_path != NULL)
        {
            /* > : last stage writes to the output file (create it, or empty it) */
            out_fd = open(job->outfile_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out_fd < 0)
            {
                const char out_error[] = "Error: cannot open output file.\n";
                print_error(out_error, my_strlen(out_error));
                break;
            }
        }
        else
        {
            out_fd = 1;                 /* last stage, no > : output to the screen */
        }

        pids[started] = spawn(&job->pipeline[i], in_fd, out_fd, next_in_fd);
        if (pids[started] > 0)
        {
            started++;
        }

        /* the shell doesn't need these any more, so close them */
        if (in_fd != 0)
        {
            close(in_fd);
        }
        if (out_fd != 1)
        {
            close(out_fd);
        }

        in_fd = next_in_fd;             /* next stage reads from the pipe */
    }

    if (in_fd > 0)                      /* left open only if the loop stopped early */
    {
        close(in_fd);
    }

    /* & : only wait for the children if this is a foreground job */
    if (job->background == 0)
    {
        for (i = 0; i < started; i++)
        {
            waitpid(pids[i], NULL, 0);
        }
    }
}

/*
 * Forks a child to run one command.
 *   in_fd:     what the child should use as standard input  (0 = keyboard as normal)
 *   out_fd:    what the child should use as standard output (1 = screen as normal)
 *   unused_fd: a pipe end the child must close, or -1 if none
 * Returns the child's pid, or -1 if fork failed. Does NOT wait.
 */
pid_t spawn(struct Command *command, int in_fd, int out_fd, int unused_fd)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        const char fork_error[] = "Error: Failed to spawn child process.\n";
        print_error(fork_error, my_strlen(fork_error));
        return -1;
    }

    if (pid == 0)                       /* child */
    {
        if (in_fd != 0)
        {
            dup2(in_fd, 0);             /* standard input now comes from in_fd */
            close(in_fd);
        }
        if (out_fd != 1)
        {
            dup2(out_fd, 1);            /* standard output now goes to out_fd */
            close(out_fd);
        }
        if (unused_fd != -1)
        {
            close(unused_fd);           /* close the pipe end this child doesn't use */
        }
        run_child_process(command);
    }

    return pid;                         /* parent */
}