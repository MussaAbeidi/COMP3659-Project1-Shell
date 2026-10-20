#include <unistd.h>
#include <signal.h>
#include "mystring.h"
#include "jobs.h"


int main(int argc, char *argv[], char *envp[])
{
  	/* ignore the SIGINT signal */
  	signal(SIGINT, sigint_handler);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN); /* needed so that the tcsetpgrp works */
	signal(SIGCHLD, sigchild_handler); /*cleanup zombie children when the get_job() hasnt been invoked but here are still zombie children from before*/
	int exit_shell = 0;
	int exit_code = 0;
	struct Job job;

	while (!exit_shell)
	{
		int status = get_job(&job, envp);

		if (status == EXIT)
		{
		 exit_shell = 1;
		}
		else if (status == READ_ERROR)
		{
		 	exit_shell = 1;
			exit_code = 1;
		}
		else if (status == 0)
		{
			run_job(&job, envp);
		}
	}

	return exit_code;
}