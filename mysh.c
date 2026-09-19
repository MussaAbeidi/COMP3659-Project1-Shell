#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 256

void print_prompt(void) {
    const char *prompt = "$ ";
    write(1, prompt, sizeof("$ ") - 1);
}

void print_error(const char *message, int length) {
    write(1, message, length);
}

void flush_input_stream(void) {
    // Removing the remaining characters in the OS input stream so that they don't get executed as the next command accidently
    char flush_char;
    while (read(0,&flush_char,1) > 0 && flush_char != '\n') {
        // Removing the characters from the input stream one at a time
    }
}

int main(void) {
    char buffer[BUFFER_SIZE];
    
    while(1) {
        print_prompt();

        // Subtracting 1 from the size to ensure we don't account for the terminating character
        ssize_t bytes_read = read(0,buffer, sizeof(buffer) - 1);

        // Terminating the program read system call failure if bytes_read euqal to -1 so the shell will not crash
        if (bytes_read == -1) {
            return 1;
        }

        // Handling the read system call EOF so that the user can exit the shell perfectly
        if (bytes_read == 0) {
            return 0;
        }

        // Safety measure for when there is a buffer overflow when the user typed more than what was allowewd
        if (bytes_read == BUFFER_SIZE - 1 && buffer[bytes_read - 1] != '\n') {
            const char error_message[] = "Error: Command line exceeds maximum length (255) allowed.\n";
            print_error(error_message, sizeof(error_message)-1);
            
            flush_input_stream();
            
            // We skip this loop and start fresh for the user
            continue;
        }

        // Checking to see if the bytes that have been read and adding terminator "\0" to make sure it is ready to be compared via strcmp, and adding a safety measure incase it is missing a "\n" 
        if (bytes_read > 0 && buffer[bytes_read - 1] == '\n') {
            buffer[bytes_read - 1] = '\0';
            bytes_read--;
        } else if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
        }

        // Checking if the user typed the "exit" command to terminate the program
        if (strcmp(buffer, "exit") == 0) {
            return 0;
        }

        // Echoing the command typed by the user back to the terminal
        write(1, buffer, bytes_read);
        write(1, "\n", 1);
    }

    return 0;
}