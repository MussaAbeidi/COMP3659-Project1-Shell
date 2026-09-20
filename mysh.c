#include <unistd.h>
#include <string.h>
#include "mysh.h"

int main() {
    
    // Poll for input from the user until 'exit' is executed
    while(1) {

        print_prompt();
        read_input();
        check_read_error();

        // Safety measure for when there is a buffer overflow when the user typed more than what was allowewd
        if (bytes_read == BUFFER_SIZE - 1 && buffer[bytes_read - 1] != '\n') {
            write_error();
            flush_trailing_chars();
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
        check_exit_condition();

        // Echoing the command typed by the user back to the terminal
        echo_bytes(bytes_read);
    }

    return 0;
}