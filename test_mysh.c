/*
 * test_mysh.c - automated tests for mysh
 *
 *   gcc -o test_mysh test_mysh.c
 *   ./test_mysh
 *
 * Every test in the table below is run against ./mysh. Results are printed
 * and saved to test_results.md.
 *
 * How a test runs: mysh is started with its input coming from a pipe. Each
 * input line is written 300 ms apart (so each arrives in its own read()),
 * then the pipe is closed, which mysh sees as Ctrl+D. Its stdout and stderr
 * are saved to files and checked. If mysh is still running after 10 seconds
 * it is killed and the test fails.
 *
 * This is a separate test program, not part of the shell, so it uses the
 * normal C library.
 */

#define _DEFAULT_SOURCE      /* for mkdtemp and realpath on Linux */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "jobs.h"            /* BUFFER_SIZE, MAX_ARGS, MAX_TOKENS, MAX_PIPELINE_LEN */

#define LONGEST_LINE (BUFFER_SIZE - 2)     /* longest line mysh accepts: 255 */
#define TEXT_SIZE    65536

/* ---------- what each test checks ---------- */

enum Check
{
    OUT_LINE,       /* stdout has a line exactly equal to expected */
    OUT_HAS,        /* stdout contains expected somewhere */
    OUT_LACKS,      /* stdout does not contain expected */
    ERR_HAS,        /* stderr contains expected */
    ERR_EMPTY,      /* stderr is empty */
    EXITS_OK,       /* mysh exits by itself without crashing */
    FILE_IS,        /* the file named in 'file' contains expected */
    TIME_UNDER,     /* the run took less than expected seconds */
    TIME_AT_LEAST   /* the run took at least expected seconds */
};

struct Test
{
    const char *category;
    const char *name;
    const char *input;      /* lines typed into mysh, separated by \n */
    enum Check check;
    const char *expected;
    const char *file;       /* only used by FILE_IS */
};

/* Filled in by setup() before the tests run; the table points at these. */
char home[PATH_MAX], path[TEXT_SIZE], tmp_dir[PATH_MAX], work_dir[PATH_MAX];
char long_var[200];                                  /* echo $AAAA... (100 A's) */
char longest_ok[300], longest_x[300];                /* a 255-char line and its output */
char too_long[300], way_too_long[500];               /* 256 and 400 chars */
char args_ok[300], args_too_many[300], tokens_too_many[300];
char pipe_ok[300], pipe_too_long[300];

struct Test tests[] =
{
    /* category              name                                    input                                      check          expected */
    { "Basic commands",      "runs a simple command",                "echo hello",                              OUT_LINE,      "hello" },
    { "Basic commands",      "simple command prints no error",       "echo hello",                              ERR_EMPTY,     "" },
    { "Basic commands",      "collapses extra spaces",               "   echo   a    b   ",                     OUT_LINE,      "a b" },
    { "Basic commands",      "treats tabs as spaces",                "\techo\tx\ty",                            OUT_LINE,      "x y" },
    { "Basic commands",      "empty line does nothing",              "\necho after",                            OUT_LINE,      "after" },
    { "Basic commands",      "whitespace-only line does nothing",    "   \necho after",                         OUT_LINE,      "after" },
    { "Basic commands",      "runs a command by absolute path",      "/bin/echo abs",                           OUT_LINE,      "abs" },
    { "Basic commands",      "finds commands through PATH",          "ls",                                      OUT_LINE,      "in.txt" },
    { "Basic commands",      "unknown command reports an error",     "nosuchcmd_xyz",                           ERR_HAS,       "Error: Command execution failed." },
    { "Basic commands",      "keeps running after unknown command",  "nosuchcmd_xyz\necho alive",               OUT_LINE,      "alive" },
    { "Basic commands",      "bad path with a slash reports error",  "./nosuchfile_xyz",                        ERR_HAS,       "Error: Command execution failed." },
    { "Basic commands",      "exit quits the shell",                 "exit\necho after_exit",                   OUT_LACKS,     "after_exit" },
    { "Basic commands",      "exit with extra words is not exit",    "exit now\necho alive",                    OUT_LINE,      "alive" },
    { "Basic commands",      "end of input (Ctrl+D) exits cleanly",  "echo x",                                  EXITS_OK,      "" },

    { "Environment variables", "$HOME expands",                      "echo $HOME",                              OUT_LINE,      home },
    { "Environment variables", "$PATH expands",                      "echo $PATH",                              OUT_HAS,       path },
    { "Environment variables", "unset variable expands to nothing",  "echo start $NO_SUCH_VAR_XYZ end",         OUT_HAS,       "end" },
    { "Environment variables", "a lone $ stays as $",                "echo cost $",                             OUT_LINE,      "cost $" },
    { "Environment variables", "variable used as a cd target",       "cd $HOME\npwd",                           OUT_LINE,      home },
    { "Environment variables", "very long variable name does not crash", long_var,                             OUT_LINE,      "alive" },

    { "cd",                  "cd to an absolute path",               "cd /usr\npwd",                            OUT_LINE,      "/usr" },
    { "cd",                  "cd with no argument goes to HOME",     "cd\npwd",                                 OUT_LINE,      home },
    { "cd",                  "cd .. goes up a level",                "cd ..\npwd",                              OUT_LINE,      tmp_dir },
    { "cd",                  "cd to a missing dir reports an error", "cd /no/such/dir",                         ERR_HAS,       "Error: cd failed." },
    { "cd",                  "shell keeps working after failed cd",  "cd /no/such/dir\npwd",                    OUT_LINE,      work_dir },

    { "Redirection",         "> writes stdout to a file",            "echo hi > r1.out",                        FILE_IS,       "hi",                   "r1.out" },
    { "Redirection",         "> leaves the screen empty",            "echo hi > r1.out",                        OUT_LACKS,     "hi" },
    { "Redirection",         "> overwrites an existing file",        "echo one > r2.out\necho two > r2.out",    FILE_IS,       "two",                  "r2.out" },
    { "Redirection",         ">> appends to a file",                 "echo one >> r3.out\necho two >> r3.out",  FILE_IS,       "one\ntwo",             "r3.out" },
    { "Redirection",         ">> creates the file if missing",       "echo new >> r4.out",                      FILE_IS,       "new",                  "r4.out" },
    { "Redirection",         "< reads stdin from a file",            "head -n 1 < in.txt",                      OUT_LINE,      "banana" },
    { "Redirection",         "< and > together",                     "sort < in.txt > r5.out",                  FILE_IS,       "apple\nbanana\ncherry", "r5.out" },
    { "Redirection",         "< and >> together",                    "echo top > r6.out\nsort < in.txt >> r6.out", FILE_IS,    "top\napple\nbanana\ncherry", "r6.out" },
    { "Redirection",         "missing input file",                   "sort < missing.txt",                      ERR_HAS,       "Error: cannot open input file." },
    { "Redirection",         "output file that can't be opened",     "echo x > /no/such/dir/f.out",             ERR_HAS,       "Error: cannot open output file." },
    { "Redirection",         "> with no file name",                  "echo hi >",                               ERR_HAS,       "Error: missing file name after < or >." },
    { "Redirection",         "< with no file name",                  "sort <",                                  ERR_HAS,       "Error: missing file name after < or >." },
    { "Redirection",         "> followed by an operator",            "echo hi > |",                             ERR_HAS,       "Error: missing file name after < or >." },
    { "Redirection",         "> before < is rejected (grammar)",     "sort > r7.out < in.txt",                  ERR_HAS,       "Error: unexpected symbol." },
    { "Redirection",         "two > redirections are rejected",      "echo hi > a.out > b.out",                 ERR_HAS,       "Error: unexpected symbol." },
    { "Redirection",         "two < redirections are rejected",      "sort < in.txt < in.txt",                  ERR_HAS,       "Error: unexpected symbol." },

    { "Pipes",               "two-stage pipe",                       "echo hello | tr a-z A-Z",                 OUT_LINE,      "HELLO" },
    { "Pipes",               "three-stage pipe",                     "sort -r in.txt | head -n 2 | tail -n 1",  OUT_LINE,      "banana" },
    { "Pipes",               "< feeds the first stage",              "sort | head -n 1 < in.txt",               OUT_LINE,      "apple" },
    { "Pipes",               "> takes the last stage",               "sort in.txt | head -n 1 > p1.out",        FILE_IS,       "apple",                "p1.out" },
    { "Pipes",               "pipe with both < and >",               "sort | tail -n 1 < in.txt > p2.out",      FILE_IS,       "cherry",               "p2.out" },
    { "Pipes",               "pipe ends are closed (no hang)",       "yes | head -n 3",                         OUT_LINE,      "y" },
    { "Pipes",               "pipe doesn't crash or hang",           "yes | head -n 3",                         EXITS_OK,      "" },
    { "Pipes",               "pipe starting with |",                 "| sort",                                  ERR_HAS,       "Error: missing command." },
    { "Pipes",               "pipe ending with |",                   "echo hi |",                               ERR_HAS,       "Error: missing command." },
    { "Pipes",               "empty stage between pipes",            "echo hi | | sort",                        ERR_HAS,       "Error: missing command." },
    { "Pipes",               "unknown command inside a pipe",        "echo hi | nosuchcmd_xyz\necho alive",     OUT_LINE,      "alive" },

    { "Limits",              "longest allowed line is accepted",     longest_ok,                                OUT_LINE,      longest_x },
    { "Limits",              "line one char too long is rejected",   too_long,                                  ERR_HAS,       "exceeds maximum length" },
    { "Limits",              "too-long line is not executed",        too_long,                                  OUT_LACKS,     "xxxxxxxxxx" },
    { "Limits",              "shell recovers after a too-long line", way_too_long,                              OUT_LINE,      "alive" },
    { "Limits",              "MAX_ARGS arguments works",             args_ok,                                   OUT_LINE,      "alive" },
    { "Limits",              "MAX_ARGS arguments prints no error",   args_ok,                                   ERR_EMPTY,     "" },
    { "Limits",              "MAX_ARGS + 1 arguments is rejected",   args_too_many,                             ERR_HAS,       "too many" },
    { "Limits",              "MAX_TOKENS + 1 tokens is rejected",    tokens_too_many,                           ERR_HAS,       "too many" },
    { "Limits",              "MAX_PIPELINE_LEN stages works",        pipe_ok,                                   OUT_LINE,      "hi" },
    { "Limits",              "MAX_PIPELINE_LEN + 1 stages is rejected", pipe_too_long,                          ERR_HAS,       "Error:" },
    /* No heap test: HEAP_SIZE (1026) is bigger than the longest line, so it can't be reached. */

    { "Background jobs",     "& returns to the prompt right away",   "sleep 3 &\necho back",                    TIME_UNDER,    "3" },
    { "Background jobs",     "prompt works while a job runs",        "sleep 3 &\necho back",                    OUT_LINE,      "back" },
    { "Background jobs",     "background job produces its output",   "echo bg_output &",                        OUT_LINE,      "bg_output" },
    { "Background jobs",     "background job with redirection",      "echo bgfile > bg1.out &",                 FILE_IS,       "bgfile",               "bg1.out" },
    { "Background jobs",     "background pipeline",                  "sort in.txt | head -n 1 > bg2.out &",     FILE_IS,       "apple",                "bg2.out" },
    { "Background jobs",     "& on its own",                         "&",                                       ERR_HAS,       "Error: missing command." },
    { "Background jobs",     "words after & are rejected",           "sleep 1 & echo hi",                       ERR_HAS,       "Error: unexpected symbol." },
    { "Background jobs",     "& before a redirection is rejected",   "echo hi & > x.out",                       ERR_HAS,       "Error: unexpected symbol." },

    { "fg / bg",             "fg with no job",                       "fg",                                      ERR_HAS,       "Error: no job to bring to foreground." },
    { "fg / bg",             "bg with no job",                       "bg",                                      ERR_HAS,       "Error: no job to resume in background." },
    { "fg / bg",             "fg waits for a background job",        "sleep 2 &\nfg\necho after_fg",            TIME_AT_LEAST, "1" },
    { "fg / bg",             "shell continues after fg",             "sleep 2 &\nfg\necho after_fg",            OUT_LINE,      "after_fg" },
};

#define NUM_TESTS (int)(sizeof tests / sizeof tests[0])

/* Tests that need a real keyboard; added to the report for filling in by hand. */
const char *manual_tests[][4] =
{
    { "Signals",     "Ctrl+C ends a foreground job",        "sleep 5, then Ctrl+C",        "-->mysh(task ended), prompt returns" },
    { "Signals",     "Ctrl+C at an empty prompt",           "Ctrl+C",                      "new prompt; shell does not exit" },
    { "Signals",     "Ctrl+Z suspends a job",               "sleep 30, then Ctrl+Z",       "-->mysh(task suspended), prompt returns" },
    { "Job control", "fg resumes a suspended job",          "sleep 30, Ctrl+Z, fg",        "sleep resumes; Ctrl+C now ends it" },
    { "Job control", "bg resumes in the background",        "sleep 30, Ctrl+Z, bg",        "prompt stays usable while sleep runs" },
    { "Job control", "fg resumes a whole pipeline",         "sleep 30 \\| sleep 30, Ctrl+Z, fg", "both stages resume; Ctrl+C ends both" },
    { "Terminal",    "Program reads the keyboard",          "cat, type lines, Ctrl+D",     "cat echoes each line, then back to mysh" },
    { "Terminal",    "Terminal handed back after fg",       "cat, Ctrl+Z, fg",             "cat can read the keyboard again" },
    { "Signals",     "Ctrl+C doesn't hit background jobs",  "sleep 30 &, then Ctrl+C",     "background sleep keeps running" },
    { "Zombies",     "Finished background jobs are reaped", "sleep 1 &, wait 2s, Enter",   "no process in state Z (ps -o pid,stat,comm)" },
    { "Job control", "Finished job is forgotten",           "sleep 1 &, wait 2s, fg",      "Error: no job to bring to foreground." },
    { "Basic",       "Ctrl+D at an empty prompt",           "Ctrl+D",                      "prints a newline and exits" },
    { "Terminal",    "Full-screen program works",           "less in.txt",                 "displays normally; q returns to mysh" },
};

/* ---------- results of the last run ---------- */

char out_text[TEXT_SIZE];     /* stdout, with "$ " prompts and blank lines removed */
char err_text[TEXT_SIZE];     /* stderr */
int killed;                   /* 1 if mysh was still running after 10 seconds */
int status;                   /* how mysh ended, from waitpid */
double seconds;               /* how long the run took */

/* ---------- helpers ---------- */

void sleep_ms(int ms)
{
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* Read a whole file into buf, without its trailing newlines. Returns 0 if missing. */
int read_file(const char *name, char *buf)
{
    FILE *f = fopen(name, "r");
    size_t n;

    buf[0] = '\0';
    if (f == NULL)
    {
        return 0;
    }
    n = fread(buf, 1, TEXT_SIZE - 1, f);
    fclose(f);
    while (n > 0 && buf[n - 1] == '\n')
    {
        n--;
    }
    buf[n] = '\0';
    return 1;
}

/* Remove "$ " prompts and empty lines, so only the commands' output is left. */
void clean_output(char *s)
{
    char *from = s, *to = s;
    int at_line_start = 1;

    while (*from)
    {
        if (from[0] == '$' && from[1] == ' ')
        {
            from += 2;
        }
        else if (*from == '\n' && at_line_start)
        {
            from++;
        }
        else
        {
            at_line_start = (*from == '\n');
            *to++ = *from++;
        }
    }
    while (to > s && to[-1] == '\n')
    {
        to--;                               /* drop the last newline */
    }
    *to = '\0';
}

/* Does text contain a line exactly equal to line? */
int has_line(const char *text, const char *line)
{
    size_t len = strlen(line);
    const char *p = text;

    while ((p = strstr(p, line)) != NULL)
    {
        int starts_line = (p == text || p[-1] == '\n');
        int ends_line = (p[len] == '\n' || p[len] == '\0');
        if (starts_line && ends_line)
        {
            return 1;
        }
        p++;
    }
    return 0;
}

/* Fill buf with n copies of piece, after start. */
void repeat(char *buf, const char *start, const char *piece, int n)
{
    strcpy(buf, start);
    while (n-- > 0)
    {
        strcat(buf, piece);
    }
}

/* ---------- running mysh ---------- */

void run_mysh(const char *bin, const char *input)
{
    char out_file[PATH_MAX + 16], err_file[PATH_MAX + 16];
    int to_shell[2];
    pid_t pid;
    double start = now();
    const char *line = input;

    snprintf(out_file, sizeof out_file, "%s/out.txt", tmp_dir);
    snprintf(err_file, sizeof err_file, "%s/err.txt", tmp_dir);
    pipe(to_shell);

    pid = fork();
    if (pid == 0)
    {
        /* child: run mysh in the work folder, reading the pipe, writing the files */
        chdir(work_dir);
        dup2(to_shell[0], 0);
        dup2(open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644), 1);
        dup2(open(err_file, O_WRONLY | O_CREAT | O_TRUNC, 0644), 2);
        close(to_shell[1]);
        execl(bin, bin, (char *)NULL);
        _exit(127);
    }

    /* parent: type each line, one every 300 ms */
    close(to_shell[0]);
    while (1)
    {
        size_t len = strcspn(line, "\n");      /* length of this line */
        write(to_shell[1], line, len);
        write(to_shell[1], "\n", 1);
        sleep_ms(300);
        if (line[len] == '\0')
        {
            break;                              /* that was the last line */
        }
        line += len + 1;
    }
    close(to_shell[1]);                     /* end of input, like Ctrl+D */

    /* wait for mysh to exit, killing it after 10 seconds */
    killed = 0;
    while (waitpid(pid, &status, WNOHANG) == 0)
    {
        if (now() - start > 10)
        {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            killed = 1;
        }
        sleep_ms(20);
    }
    seconds = now() - start;

    sleep_ms(300);                          /* let background jobs finish writing */
    read_file(out_file, out_text);
    clean_output(out_text);
    read_file(err_file, err_text);
}

/* ---------- checking one test ---------- */

/* Run test t and fill in the expected and actual descriptions. Returns 1 if it passed. */
int run_test(const char *bin, struct Test *t, char *expected, char *actual)
{
    char file_text[TEXT_SIZE];
    char file_path[PATH_MAX * 2];
    int passed = 0;

    run_mysh(bin, t->input);

    switch (t->check)
    {
        case OUT_LINE:
            passed = has_line(out_text, t->expected);
            sprintf(expected, "stdout: %s", t->expected);
            sprintf(actual, "stdout: %s", out_text);
            break;
        case OUT_HAS:
            passed = strstr(out_text, t->expected) != NULL;
            sprintf(expected, "stdout contains: %s", t->expected);
            sprintf(actual, "stdout: %s", out_text);
            break;
        case OUT_LACKS:
            passed = strstr(out_text, t->expected) == NULL;
            sprintf(expected, "stdout does not contain: %s", t->expected);
            sprintf(actual, "stdout: %s", out_text);
            break;
        case ERR_HAS:
            passed = strstr(err_text, t->expected) != NULL;
            sprintf(expected, "stderr: %s", t->expected);
            sprintf(actual, "stderr: %s", err_text);
            break;
        case ERR_EMPTY:
            passed = err_text[0] == '\0';
            sprintf(expected, "no error message");
            sprintf(actual, "stderr: %s", err_text);
            break;
        case EXITS_OK:
            passed = !killed && WIFEXITED(status);
            sprintf(expected, "shell exits normally");
            if (killed)
                sprintf(actual, "still running after 10 seconds (killed)");
            else if (WIFEXITED(status))
                sprintf(actual, "exit status %d", WEXITSTATUS(status));
            else
                sprintf(actual, "crashed");
            break;
        case FILE_IS:
            snprintf(file_path, sizeof file_path, "%s/%s", work_dir, t->file);
            sprintf(expected, "%s contains: %s", t->file, t->expected);
            if (read_file(file_path, file_text))
            {
                passed = strcmp(file_text, t->expected) == 0;
                sprintf(actual, "%s contains: %s", t->file, file_text);
            }
            else
            {
                sprintf(actual, "%s was not created", t->file);
            }
            break;
        case TIME_UNDER:
            passed = seconds < atof(t->expected);
            sprintf(expected, "finishes in under %ss", t->expected);
            sprintf(actual, "took %.1fs", seconds);
            break;
        case TIME_AT_LEAST:
            passed = seconds >= atof(t->expected);
            sprintf(expected, "takes at least %ss", t->expected);
            sprintf(actual, "took %.1fs", seconds);
            break;
    }
    return passed;
}

/* ---------- writing the report ---------- */

/* Write text as one table cell: newlines become " / ", long text is cut short. */
void write_cell(FILE *f, const char *text)
{
    size_t len = strlen(text), i;

    if (len == 0)
    {
        fprintf(f, "*(empty)*");
        return;
    }
    fprintf(f, "`");
    for (i = 0; i < len && i < 60; i++)
    {
        if (text[i] == '\n')       fprintf(f, " / ");
        else if (text[i] == '\t')  fprintf(f, "<TAB>");
        else if (text[i] == '|')   fprintf(f, "\\|");
        else if (text[i] == '`')   fprintf(f, "'");
        else                       fputc(text[i], f);
    }
    if (len > 60)
    {
        fprintf(f, "... (%zu chars)", len);
    }
    fprintf(f, "`");
}

/* ---------- setup ---------- */

void setup(void)
{
    char folder[] = "/tmp/mysh_test.XXXXXX";
    char in_file[PATH_MAX + 16];
    FILE *f;

    /* a scratch folder, with in.txt for the redirection tests */
    realpath(mkdtemp(folder), tmp_dir);
    snprintf(work_dir, sizeof work_dir, "%s/work", tmp_dir);
    mkdir(work_dir, 0755);
    snprintf(in_file, sizeof in_file, "%s/in.txt", work_dir);
    f = fopen(in_file, "w");
    fputs("banana\napple\ncherry\n", f);
    fclose(f);

    /* values the table points at */
    snprintf(home, sizeof home, "%s", getenv("HOME"));
    snprintf(path, sizeof path, "%s", getenv("PATH"));

    repeat(long_var, "echo $", "A", 100);
    strcat(long_var, "\necho alive");

    repeat(longest_ok, "echo ", "x", LONGEST_LINE - 5);     /* 255 chars in total */
    strcpy(longest_x, longest_ok + 5);
    repeat(too_long, "echo ", "x", LONGEST_LINE - 4);       /* 256 chars */
    repeat(way_too_long, "echo ", "x", 400);
    strcat(way_too_long, "\necho alive");

    repeat(args_ok, "echo", " a", MAX_ARGS - 1);           /* echo + 19 words = 20 */
    strcat(args_ok, "\necho alive");
    repeat(args_too_many, "echo", " a", MAX_ARGS);
    repeat(tokens_too_many, "echo", " a", MAX_TOKENS);

    repeat(pipe_ok, "echo hi", " | cat", MAX_PIPELINE_LEN - 1);
    repeat(pipe_too_long, "echo hi", " | cat", MAX_PIPELINE_LEN);

    signal(SIGPIPE, SIG_IGN);    /* if mysh exits early, writing to it mustn't kill us */
}

/* ---------- main ---------- */

int main(int argc, char *argv[])
{
    char bin[PATH_MAX];
    char expected[TEXT_SIZE + 512], actual[TEXT_SIZE + 512];
    char command[PATH_MAX + 16];
    int passed = 0, failed = 0, i;
    const char *last_category = "";
    FILE *report;

    if (realpath(argc > 1 ? argv[1] : "./mysh", bin) == NULL || access(bin, X_OK) != 0)
    {
        printf("Can't find ./mysh. Run make first.\n");
        return 2;
    }
    setup();

    report = fopen("test_results.md", "w");
    fprintf(report, "# mysh Test Results\n\n");
    fprintf(report, "Each input line is typed in turn (lines separated by ' / '), then input ends "
                    "like Ctrl+D. Long text is cut short, with its full length in brackets.\n\n");
    fprintf(report, "## Automated tests\n\n");
    fprintf(report, "| Category | Test | Input | Expected output | Actual output | Result |\n");
    fprintf(report, "|---|---|---|---|---|---|\n");

    for (i = 0; i < NUM_TESTS; i++)
    {
        struct Test *t = &tests[i];
        int ok = run_test(bin, t, expected, actual);

        if (strcmp(t->category, last_category) != 0)
        {
            printf("== %s\n", t->category);
            last_category = t->category;
        }
        if (ok)
        {
            passed++;
            printf("  PASS  %s\n", t->name);
        }
        else
        {
            failed++;
            printf("  FAIL  %s\n        expected: %s\n        actual:   %s\n", t->name, expected, actual);
        }

        fprintf(report, "| %s | %s | ", t->category, t->name);
        write_cell(report, t->input);
        fprintf(report, " | ");
        write_cell(report, expected);
        fprintf(report, " | ");
        write_cell(report, actual);
        fprintf(report, " | %s |\n", ok ? "PASS" : "FAIL");
    }

    fprintf(report, "\n**Summary:** %d passed, %d failed\n", passed, failed);

    fprintf(report, "\n## Manual tests\n\n");
    fprintf(report, "These need a real keyboard. Run ./mysh in a terminal and fill in the last two columns.\n\n");
    fprintf(report, "| Category | Test | Input | Expected output | Actual output | Result |\n");
    fprintf(report, "|---|---|---|---|---|---|\n");
    for (i = 0; i < (int)(sizeof manual_tests / sizeof manual_tests[0]); i++)
    {
        fprintf(report, "| %s | %s | %s | %s | | |\n",
                manual_tests[i][0], manual_tests[i][1], manual_tests[i][2], manual_tests[i][3]);
    }
    fclose(report);

    snprintf(command, sizeof command, "rm -rf '%s'", tmp_dir);
    system(command);

    printf("\n%d passed, %d failed\nResults saved to test_results.md\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
