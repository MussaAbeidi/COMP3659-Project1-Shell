#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "jobs.h"
#include "parser.h"
#include "myheap.h"
#include "mystring.h"


const char error_message[] = "Error: Command line exceeds maximum length (255) allowed.\n";
char buffer[BUFFER_SIZE];
ssize_t bytes_read;
char *tokens[MAX_TOKENS];
int num_tokens;
pid_t saved_pgid = 0; /* to store the process group ID */

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

void echo_error()
{
    write(2, error_message, my_strlen(error_message));
}

int check_read_error()
{
    // terminating the program if bytes_read is -1, which indicates an error occurred during the read operation
    if (bytes_read == -1)
    {   
        // if ctrl + C interrupted read, it's not a real crash
        if (errno == EINTR)
        {
            return 2;
        }
        return 1;
    }
    return 0;
}

int check_exit_condition()
{
    if (num_tokens == 1 && my_strcmp(tokens[0], "exit") == 0)
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

/*For handling ctrl + D after an empty input*/
int check_end_of_input()
{
    return bytes_read == 0;
}

int tokenize_string(char *envp[])                                  
{
    char *start = buffer;
    char *end;
    num_tokens = 0;                                  

    // Skip over leading whitespace
    while(*start && (*start == ' ' || *start == '\t'))
    {   
        start++;
    }
    // Empty string inputted
    if (!*start) 
    {
        num_tokens = 0;                                
        return 0;
    }
    while(*start && num_tokens < MAX_TOKENS)     
    {
        end = start;
        while (*end && (*end != ' ' && *end != '\t')) 
        {
            end++;
        }
        
        char temp = *end;
        *end = '\0';

        char *word_to_copy = start;
        /* if token starts with $, look up the environment variable */
        if (start[0] == '$' && start[1] != '\0')
        {
            char *env_val = my_getenv(start + 1, envp);
            if (env_val != NULL)
            {
                word_to_copy = env_val;
            }
            else
            {
                word_to_copy = "";
            }
        }

        int token_length = my_strlen(word_to_copy);
        char *token = alloc(token_length + 1);

        // Trying to exceed heap limit
        if (token == NULL)
        {
            *end = temp;
            const char heap_err[] = "Error: Heap limit exceeded.\n";
            print_error(heap_err, my_strlen(heap_err));
            return HEAP_EXCEEDED;
        }

        my_strcpy(word_to_copy, token);
        *end = temp;
        start = end;
        tokens[num_tokens] = token;                    /* CHANGED */
        num_tokens++;                                  /* CHANGED */
        while (*start && (*start == ' ' || *start == '\t')) 
        {
            start++;
        }
    }
    /* if there are more characters in the buffer, the user has then typed too many tokens*/
    if (*start != '\0')
    {
        const char tok_err[] = "Error: too many tokens.\n";
        print_error(tok_err, my_strlen(tok_err));
        return HEAP_EXCEEDED;
    }
    
    return 0;                                          /* removed the argv NULL line */
}

/* Run a child process */
void run_child_process(struct Command *command, char *envp[])
{
    char path[PATH_LEN];
    /* handling the case where the command contains a slash */
    if (has_slash(command->argv[0]))
    {
        my_strcpy(command->argv[0], path);
        execve(path, command->argv, envp);
    }
    else
    {
        /* searching for the executable in the PATH directories */
        char *path_env = my_getenv("PATH", envp);
        if (!path_env)
        {
            /* Set a default PATH if not found */
            path_env = "/bin:/usr/bin:/usr/local/bin";
        }
        char path_buf[1024];
        my_strcpy(path_env, path_buf);

        char *dir = path_buf;
        char *next_dir;
        /* while there are more directories in the PATH */
        while (dir != NULL && *dir != '\0')
        {
            next_dir = NULL;
            char *p = dir;
            /* parsing each directory in the PATH */
            while (*p)
            {
                if (*p == ':')
                {
                    *p = '\0';
                    next_dir = p + 1;
                    break;
                }
                p++;
            }
            /* creating the full path to the executable*/
            path[0] = '\0';
            my_strcpy(dir, path);
            my_strcat(path, "/");
            my_strcat(path, command->argv[0]);
            /* checking if the executable is accessible */
            if (access(path, X_OK) == 0)
            {
                execve(path, command->argv, envp);
            }
            /* move to the next directory in the PATH */
            dir = next_dir;
        }
    }

    const char exec_error[] = "Error: Command execution failed.\n";
    print_error(exec_error, my_strlen(exec_error));
    _exit(1);
}

/* signal handling for ctrl + C*/
void sigint_handler(int sig)
{
    (void)sig;
    const char *stop_msg = "-->mysh(task ended)\n";
	write(2, stop_msg, my_strlen(stop_msg)); 
}

void sigchild_handler(int sig)
{
    (void)sig;
    cleanup_zombies();
}

int get_job(struct Job *job, char *envp[])    // changed
{
    free_all();
    reset_bytes_read();
    cleanup_zombies();
    print_prompt();
    read_input();

    if (check_read_error() == 2) /* ctrl + C was pressed at the prompt */
    {
        return EMPTY_LINE;
    }
    if (check_read_error() == 1)
    {
        return READ_ERROR;
    }
    if (check_end_of_input()) /*if ctrl + D is pressed after no input*/
    {
        write(1, "\n", 1);
        return EXIT;
    }

    if (buffer_overflow_detected())
    {
        return BUFFER_OVERFLOW;
    }

    format_string();

    if (tokenize_string(envp) != 0)
    {
        return HEAP_EXCEEDED;
    }
    if (num_tokens == 0)
    {
        return EMPTY_LINE;
    }
    if (check_exit_condition())
    {
        return EXIT;
    }

    return parse_job(job);
}


void run_job(struct Job *job, char *envp[])
{
    /* handling the 'cd' command internally */
    if (my_strcmp(job->pipeline[0].argv[0], "cd") == 0)
    {
        const char *target_dir = job->pipeline[0].argv[1];
        if (!target_dir)
        {
            target_dir = my_getenv("HOME", envp); /* we assume the user wants to go to their home directory */
        }
        if (target_dir && chdir(target_dir) != 0) /*tries to enter home directory*/
        {
            const char cd_error[] = "Error: cd failed.\n";
            print_error(cd_error, my_strlen(cd_error));
        }
        return;
    }

    /* handling the 'fg' command internally to resume a stopped/background job in foreground */
    if (my_strcmp(job->pipeline[0].argv[0], "fg") == 0)
    {
        if (saved_pgid <= 0)
        {
            const char fg_err[] = "Error: no job to bring to foreground.\n";
            print_error(fg_err, my_strlen(fg_err));
            return;
        }
        signal(SIGCHLD, SIG_DFL);
        tcsetpgrp(0, saved_pgid);       /* give terminal to the job */
        kill(-saved_pgid, SIGCONT);     /* wake up the process group */

        int status = 0;
        while (waitpid(-saved_pgid, &status, WUNTRACED) > 0)
        {
            if (WIFSTOPPED(status))
            {
                const char *suspend_msg = "\n-->mysh(task suspended)\n";
                write(2, suspend_msg, my_strlen(suspend_msg));
                break;
            }
        }
        if (!WIFSTOPPED(status))
        {
            saved_pgid = 0;             /* job finished */
        }
        tcsetpgrp(0, getpgrp());        /* give terminal back to mysh */
        signal(SIGCHLD, sigchild_handler);
        return;
    }

    /* handling the 'bg' command internally to resume a stopped job and beings running it in the background */
    if (my_strcmp(job->pipeline[0].argv[0], "bg") == 0)
    {
        if (saved_pgid <= 0)
        {
            const char bg_err[] = "Error: no job to resume in background.\n";
            print_error(bg_err, my_strlen(bg_err));
            return;
        }
        kill(-saved_pgid, SIGCONT);     /* wake up the process group in background */
        return;
    }



    pid_t pids[MAX_PIPELINE_LEN];
    unsigned int started = 0;
    unsigned int i;
    int in_fd = 0;          /* input for the current stage (0 = keyboard) */
    int out_fd;             /* output for the current stage */
    int next_in_fd;         /* pipe read end, saved for the next stage */
    int fds[2];

    int group = 0; /* starts a new process group for every job */

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

    /* turn off the sigchild handler while waiting so it does not steal foreground status */
    signal(SIGCHLD, SIG_DFL);

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
            /* > or >> : last stage writes to the output file */
            if (job->append == 1)
            {
                out_fd = open(job->outfile_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
            }
            else
            {
                out_fd = open(job->outfile_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }
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

        pids[started] = spawn(&job->pipeline[i], in_fd, out_fd, next_in_fd, group, envp);
        if (pids[started] > 0)
        {
            if(group == 0)
            {
                group = pids[started];
            }
            if(group > 0)
            {
                setpgid(pids[started], group); /* set in parent as well to avoid a race condition*/
            }
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
    if (job->background == 0 && started > 0)
    {
        tcsetpgrp(0, group); /* give terminal to the job */
        int status = 0;
        for (i = 0; i < started; i++)
        {
            status = 0;
            waitpid(pids[i], &status, WUNTRACED);
            if (WIFSTOPPED(status))
            {
                saved_pgid = group; /* saving the group ID so that the fg command can use it to resume it*/
                const char *suspend_msg = "\n-->mysh(task suspended)\n";
                write(2, suspend_msg, my_strlen(suspend_msg));
            }
        }
        tcsetpgrp(0, getpgrp()); /* return terminal control back to mysh */
    }
    else if (job->background == 1 && started > 0)
    {
        saved_pgid = group; /* saving the background job group id */
    }

    /* turning the sigchild back on for zombies*/
    signal(SIGCHLD, sigchild_handler);
}

/*
 * Forks a child to run one command.
 *   in_fd:     what the child should use as standard input  (0 = keyboard as normal)
 *   out_fd:    what the child should use as standard output (1 = screen as normal)
 *   unused_fd: a pipe end the child must close, or -1 if none
 * Returns the child's pid, or -1 if fork failed. Does NOT wait.
 */
int spawn(struct Command *command, int in_fd, int out_fd, int unused_fd, int pgid, char *envp[])
{
    int pid = fork();

    if (pid < 0)
    {
        const char fork_error[] = "Error: Failed to spawn child process.\n";
        print_error(fork_error, my_strlen(fork_error));
        return -1;
    }


    if (pid == 0) /* child */
    {
        if (pgid >= 0)
        {  
            setpgid(0, pgid); /*make current process's group id = pgid. If pgid = 0, then new group id = pid*/
        }

        /* restoring the default signal handler in the child process */
        signal(SIGINT, SIG_DFL); /*for ctrl + C*/
        signal(SIGTSTP, SIG_DFL);/*for ctrl + Z*/
        signal(SIGTTOU, SIG_DFL);
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
        run_child_process(command,envp);
    }

    return pid;                         /* parent */
}

void cleanup_zombies(void)
{
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
    {
        /* if our saved background job finished, reset saved_pgid */
        if (pid == saved_pgid)
        {
            saved_pgid = 0;
        }
    }
}