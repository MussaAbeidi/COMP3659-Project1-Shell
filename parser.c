#include <unistd.h>
#include "parser.h"
#include "mystring.h"

extern char *tokens[];   
extern int num_tokens;
static int pos;


static int is_special(const char *tok)
{
    return my_strcmp(tok, "|") == 0 || my_strcmp(tok, "<") == 0 ||
        my_strcmp(tok, ">") == 0 || my_strcmp(tok, "&") == 0;
}

static int parse_error(const char *message)
{
    print_error(message, my_strlen(message));
    return PARSE_ERROR;
}

int parse_command(struct Command *command)
{
    command->argc = 0;
    while (pos < num_tokens && !is_special(tokens[pos]))
    {
        if (command->argc == MAX_ARGS){
            return parse_error("Error: too many arguments.\n");
        }
        command->argv[command->argc] = tokens[pos];
        command->argc++;
        pos++;
    }
    command->argv[command->argc] = NULL;

    if (command->argc == 0){
        return parse_error("Error: missing command.\n");
    }
    return 0;
}

int parse_pipeline(struct Job *job)
{
    int more = 1;
    job->num_stages = 0;

    while (more)
    {
        if (job->num_stages == MAX_PIPELINE_LEN)
        {
            return parse_error("Error: pipeline too long.\n");
        }
        if (parse_command(&job->pipeline[job->num_stages]) != 0){
            return PARSE_ERROR;
        }
        job->num_stages++;

        if (pos < num_tokens && my_strcmp(tokens[pos], "|") == 0){
            pos++;          /* skip the | */
        }
        else
        {
            more = 0;
        }
    }
    return 0;
}

/* helper for "< file" and "> file" */
int parse_filename(char **path)
{
    pos++;              /* skip the < or > */
    if (pos >= num_tokens|| is_special(tokens[pos]))
    {
        return parse_error("Error: missing file name after < or >.\n");
    }
    *path = tokens[pos];
    pos++;
    return 0;
}

/* Job -> Pipeline [< token] [> token] [&] */
int parse_job(struct Job *job)
{
    pos = 0;
    job->infile_path = NULL;
    job->outfile_path = NULL;
    job->background = 0;

    if (parse_pipeline(job) != 0){
        return PARSE_ERROR;
    }

    if (pos < num_tokens && my_strcmp(tokens[pos], "<") == 0)
    {
        if (parse_filename(&job->infile_path) != 0)
        {
            return PARSE_ERROR;
        }
    }

    if (pos < num_tokens && my_strcmp(tokens[pos], ">") == 0)
    {
        if (parse_filename(&job->outfile_path) != 0)
        {
            return PARSE_ERROR;
        }
    }

    if (pos < num_tokens && my_strcmp(tokens[pos], "&") == 0)
    {
        job->background = 1;
        pos++;
    }

    if (pos < num_tokens)
    {
        return parse_error("Error: unexpected symbol.\n");
    }
    return 0;
}