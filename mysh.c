#include "mystring.h"
#include "jobs.h"


int main(int argc, char *argv[], char *envp[])
{
  int exitShell = 0;
  struct Command command;

  write(1, "mysh> ", 6);

  get_command(&command);

  /* TO DO: prompt for and read command line */

  while (!exitShell)
  {
    if (command.argc > 0)
    {
      if (mystrcmp(command.argv[0], "exit") == 0)
      {
        exitShell = 1;
      }
      else
      {
        run_command(&command);
      }

      if (!exitShell)
      {
        write(1, "mysh> ", 6);
        get_command(&command);
      }
    }
  }

  return 0;
}