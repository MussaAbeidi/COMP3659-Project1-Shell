#include <unistd.h>

#ifndef MYSH
#define MYSH
#define BUFFER_SIZE 257

const char error_message[] = "Error: Command line exceeds maximum length (255) allowed.\n";
char buffer[BUFFER_SIZE];
ssize_t bytes_read;

void print_prompt() {
    const char *prompt = "$ ";
    write(1, prompt, sizeof("$ ") - 1);
}

// Subtracting 1 from the size to ensure we don't account for the terminating character
void read_input(){
    bytes_read = read(0,buffer, sizeof(buffer) - 1);

}

void print_error(const char *message, int length) {
    write(1, message, length);
}

void flush_trailing_chars() {
    char flush_char;
    while (read(0,&flush_char,1) > 0 && flush_char != '\n') {
    }
}

void echo_bytes(ssize_t bytes_read){
    write(1, buffer, bytes_read);
    write(1, "\n", 1);
}

void write_error(){
    write(1, error_message, sizeof(error_message)-1);
}

void check_read_error(){

    // Terminating the program read system call failure if bytes_read euqal to -1 so the shell will not crash
    if (bytes_read == -1) {
        exit(1);
    }
    // Handling the read system call EOF so that the user can exit the shell perfectly
    else if (bytes_read == 0) {
        exit(0);
    }
}

void check_exit_condition(){
    if (strcmp(buffer, "exit") == 0) {
        exit(0);
    }    

}

#endif