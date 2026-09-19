#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 256

int main(void) {
    char buffer[BUFFER_SIZE];
    const char *prompt = "$ ";
    
    while(1) {
        write(1, prompt, sizeof("$ ") - 1);

        // We subtract 1 from the size to ensure we don't account for the terminating character
        ssize_t bytes_read = read(0,buffer, sizeof(buffer) - 1);

        // 1. Handles system read failure so the shell will not crash
        if (bytes_read == -1) {
            return 1;
        }

        // 2. Handles EOF to allow user to exit the shell
        if (bytes_read == 0) {
            return 0;
        }

        // 3. Handles buffer overflow (user types more than our buffer can handle)
        if (bytes_read == BUFFER_SIZE - 1 && buffer[bytes_read - 1] != '\n') {
            const char error_message[] = "Error: Command line exceeds maximum length (255) allowed.\n";
            write(1,error_message,sizeof(error_message)-1);
            
            // Vacuuming up the remaining characters in the OS input stream so they don't get executed as the next command by accident
            char flush_char;
            while (read(0,&flush_char,1) > 0 && flush_char != '\n') {
                // Discarding characters one at a time until we hit a newline
            }
            // We skip this loop and start fresh for the user
            continue;
        }


        if (bytes_read > 0 && buffer[bytes_read - 1] == '\n') {
            buffer[bytes_read - 1] = '\0';
            bytes_read--;
        } else if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
        }

        // Checking for "exit" command to terminate the program
        if (strcmp(buffer, "exit") == 0) {
            return 0;
        }

        // Echoing the command the user inputted back to the terminal 
        write(1, buffer, bytes_read);
        write(1, "\n", 1);
    }


    return 0;
}