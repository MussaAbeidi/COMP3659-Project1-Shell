#include <stdio.h> // REMOVE
#include <unistd.h>
#include "jobs.h"
#include "myheap.h"
#include "mystring.h"

const char error_message[] = "Error: Command line exceeds maximum length (255) allowed.\n";
char buffer[BUFFER_SIZE];
ssize_t bytes_read;

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
    write(1, message, length);
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
    if (strcmp(buffer, "exit") == 0)
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

int check_trailing_ampersand()
{
    if(buffer[bytes_read - 1] == '&')
    {
        return 1;
    }
    return 0;
}

void remove_ampersand()
{
    buffer[bytes_read - 1] = '\0';
}

int tokenize_string(struct Command *command)
{
    char *start = buffer;
    char *end;
    command->argc = 0;

    // Skip over leading whitespace
    while(*start && (*start == ' ' || *start == '\t'))
    {   
        start++;
    }
    // Empty string inputted
    if (!*start) {
        command->argc = 0;
        return 0;
    }
    while(*start && (command->argc) < MAX_ARGS)
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
        
        command->argv[command->argc] = token;
        command->argc++;
        while (*start && (*start == ' ' || *start == '\t')) 
        {
            start++;
        }

    }
    
    command->argv[command->argc] = NULL;
    return 0;
}

int get_command(struct Command *command)
{
    free_all(); 
    reset_bytes_read();
    print_prompt();
    read_input();
    if (check_read_error())
    {
        return READ_ERROR;
    }
    if(buffer_overflow_detected())
    {
        return BUFFER_OVERFLOW;
    }
    format_string();
    if (check_exit_condition())
    {
        return EXIT;
    }
    if (check_trailing_ampersand())
    {
        command->background_status = 1;
        remove_ampersand();
    }
    tokenize_string(command);

    return 0;
}