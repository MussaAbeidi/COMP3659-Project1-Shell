#include <unistd.h>
#include "jobs.h"
#include "parser.h"
#include "myheap.h"
#include "mystring.h"
#include <sys/wait.h>

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

void run_child_process(struct Command *command)
{
    // execve is used to execute the command in the child process. returns (-1) only if there is an error
    // execve takes in the path to the command, the arguments, and the environment variables (NULL in this case)
    if (execve(command->argv[0], command->argv, NULL) == -1)
    {
        const char exec_error[] = "Error: Command execution failed.\n";
        print_error(exec_error, my_strlen(exec_error));
        
        // using _exit wrapper to terminate the child process safely
        _exit(1); 
    }
}

void wait_for_foreground_process(pid_t pid, int background) // changed
{
    // if background_status == 0, we wait for the child process to finish before returning to the prompt
    if (background == 0)    // changed
    {
        int status;
        waitpid(pid, &status, 0);
    }
    // if background == 1, we do not wait and return to the prompt immediately
}


void run_command(struct Command *command, int background)   // changed
{
    // if command is empty or the first argument is NULL, we do not execute anything
    if (command->argc == 0 || command->argv[0] == NULL)
    {
        return;
    }
    
    pid_t pid = fork();

    // if pid < 0, then fork failed
    if (pid < 0)
    {
        // printing an error message to the user if the fork failed
        const char fork_error[] = "Error: Failed to spawn child process.\n";
        print_error(fork_error, my_strlen(fork_error));
    }
    // if pid is equal to 0, we are in the child process
    else if (pid == 0)
    {
        run_child_process(command);
    }
    // if pid > 0, we are in the parent process
    else
    {
        wait_for_foreground_process(pid, background);   //changed
    }
}


int get_job(struct Job *job)    // changed
{
    free_all();
    reset_bytes_read();
    print_prompt();
    read_input();

    if (check_read_error()){
        return READ_ERROR;
    }
    if (buffer_overflow_detected()){
        return BUFFER_OVERFLOW;
    }
    format_string();
    if (check_exit_condition()){
        return EXIT;
    }
    if (tokenize_string() != 0){
        return HEAP_EXCEEDED;
    }
    if (num_tokens == 0){
        return EMPTY_LINE;
    }

    return parse_job(job);
}

void run_job(struct Job *job)
{
  run_command(&job->pipeline[0], job->background);  // changed
}