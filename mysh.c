#include <unistd.h>
#include <signal.h>
#include "mystring.h"
#include "jobs.h"

void sigint_handler(int sig)
{
    (void)sig;
    write(1, "\n", 1);
}

int main(int argc, char *argv[], char *envp[])
{
  /* ignore the SIGINT signal */
  signal(SIGINT, sigint_handler);

  int exitShell = 0;
  int exitCode = 0;
  struct Job job;

  while (!exitShell)
    {
      int status = get_job(&job);

      if (status == EXIT)
        {
          exitShell = 1;
        }
      else if (status == READ_ERROR)
        {
          exitShell = 1;
          exitCode = 1;
        }
      else if (status == 0)
        {
          run_job(&job, envp);
        }
    }

  return exitCode;
}