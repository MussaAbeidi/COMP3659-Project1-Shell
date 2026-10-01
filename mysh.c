#include <unistd.h>
#include "mystring.h"
#include "jobs.h"

int main(int argc, char *argv[], char *envp[])
{
  int exitShell = 0;
  int exitCode = 0;
  struct Job job;

  while (!exitShell)
    {
      /* prompt for and read command line */
      int status = get_job(&job);

      /* process command line */
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
          run_job(&job);
        }
      /* BUFFER_OVERFLOW: do nothing, loop prompts again */
    }

  return exitCode;
}