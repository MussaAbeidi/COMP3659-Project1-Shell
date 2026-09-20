#include <unistd.h>
#include <string.h>
#include "mysh.h"

int main() {
    
    // Poll for input from the user until 'exit' is executed
    while(1) {

        // Prefix "$ " for input
        print_prompt();
        // Get keyboard input from user
        read_input();
        check_read_error();
        if(buffer_overflow_detected()){continue;}
        // Checking to see if the bytes that have been read and adding terminator "\0" to make sure it is ready to be compared via strcmp, and adding a safety measure incase it is missing a "\n" 
        format_string();
        // Checking if the user typed the "exit" command to terminate the program
        check_exit_condition();
        // Echoing the command typed by the user back to the terminal
        echo_bytes(bytes_read);
    }

    return 0;
}