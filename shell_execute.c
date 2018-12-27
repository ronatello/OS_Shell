#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>
#include "header.h"

int execute(char inp[][256])
{
	char* args[16];
	char* temp;

	pid_t pid;

	int status;
	int bgflag = 0;

	extern int forepid;

	extern int proccount;
	extern proc procs[256];
	extern int jobnum;
	
	int i;
	int j = 0;

	for (i = 0; inp[i][0] != '\0'; i++)
	{
		args[i] = (char*)malloc(128 * sizeof(char));

		if (i > 0 && (!strcmp(inp[i], "&")))
		{
			bgflag = 1;
			j++;
			continue;
		}

		strcpy(args[i - j], inp[i]);
	}

	args[i - j] = NULL;

	if (bgflag == 1)
	{
		if ((pid = fork()) < 0)
		{
			fprintf(stderr, "Error: Forking child process failed\n");
			return -1;
		}

		else if (pid == 0)
		{
			if (execvp(args[0], args) < 0)
			{
				fprintf(stderr, "Error: Execution failed (maybe %s is not a command?)\n", args[0]);
				exit(1);
			}
		}

		else
		{
			fprintf(stdout, "[%d]+ %d\n", jobnum, pid);

			setpgid(pid, 0);
			//printf("ayy lmao\n");
			procs[proccount].pid = pid;
			strcpy(procs[proccount].p_name, args[0]);
			procs[proccount].num = jobnum;
			//printf("ayy lmao\n");
			strcpy(procs[proccount].State, "Running");
			jobnum++;
			proccount++;

			//printf("ayy lmao %d\n", proccount);
		}
	}

	else
	{
		if ((pid = fork()) < 0)
		{
			fprintf(stderr, "Error: Forking child process failed\n");
			exit(1);
		}
		else if (pid == 0)
		{
			if (execvp(args[0], args) < 0)
			{
				fprintf(stderr, "Error: Execution failed (maybe %s is not a command?)\n", args[0]);
				exit(1);
			}
		}

		forepid = pid;

		waitpid(pid, &status, WUNTRACED);

		forepid = -1;

		if (WIFSTOPPED(status) != 0)
		{
			for (i = 0; i < proccount; i++)
			{
				if (procs[i].pid == pid)
				{
					fprintf(stdout, "[%d]+ %d\n", procs[i].num, pid);
					setpgid(pid, pid);
					strcpy(procs[proccount].State, "Stopped");
					return 0;
				}
			}

			fprintf(stdout, "[%d]+ %d\n", jobnum, pid);

			setpgid(pid, 0);

			procs[proccount].pid = pid;
			strcpy(procs[proccount].p_name, args[0]);
			procs[proccount].num = jobnum;
			strcpy(procs[proccount].State, "Stopped");
			jobnum++;
			proccount++;
		}

	}
}

int fg(char inp[][256])
{
	extern int forepid;

	extern int proccount;
	extern proc procs[256];
	extern int ttyd;
	extern int termpgid;
	extern int jobnum;

	int status;
	int i;
	int j;
	int job;

	job = atoi(inp[1]);

	for (i = 0; i < proccount; i++)
	{
		if (procs[i].num == job)
		{
			signal(SIGTTOU, SIG_IGN);

			kill(procs[i].pid, SIGCONT);

			forepid = procs[i].pid;

			//tcsetpgrp(ttyd, procs[i].pid);

			waitpid(procs[i].pid, &status, WUNTRACED);

			forepid = -1;

			if (WIFSTOPPED(status) != 0)
			{
				for (j = 0; j < proccount; j++)
				{
					if (procs[j].pid == procs[i].pid)
					{
						fprintf(stdout, "[%d]+ %d\n", procs[j].num, procs[j].pid);
						setpgid(procs[j].pid, 0);
						strcpy(procs[j].State, "Stopped");
						return 0;
					}
				}

				fprintf(stdout, "[%d]+ %d\n", procs[i].num, procs[i].pid);

				setpgid(procs[i].pid, 0);

				procs[proccount].pid = procs[i].pid;
				strcpy(procs[proccount].p_name, procs[i].p_name);
				procs[proccount].num = jobnum;
				strcpy(procs[proccount].State, "Stopped");
				jobnum++;
				proccount++;
			}

			//tcsetpgrp(ttyd, getpid());
			signal(SIGTTOU, SIG_DFL);

			return 0;
		}
	}

	fprintf(stderr, "No such job.\n");
	return 0;
}

int bg(char inp[][256])
{
	extern int proccount;
	extern proc procs[256];
	int i;
	int job;

	job = atoi(inp[1]);

	for (i = 0; i < proccount; i++)
	{
		if (procs[i].num == job)
		{
			if (!strcmp(procs[i].State, "Stopped"))
			{
				kill(procs[i].pid, SIGCONT);
				strcpy(procs[i].State, "Running");
				return 0;
			}

			else
			{
				fprintf(stderr, "Background process already running.\n");
				return 0;
			}
		}
	}

	fprintf(stderr, "No such job.\n");
	return 0;
}