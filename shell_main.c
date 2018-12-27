#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
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

int ttyd;
int termpgid;

int forepid = -1;
int mainpid;

int saved_stdout;
int saved_stdin;

volatile sig_atomic_t end = 0;

void sighandler(int signum)
{
	end = 1;
}

void inthandler(int signum)
{
	if (forepid == mainpid)
		signal(SIGINT, inthandler);

	else if (forepid > 0)
	{
		kill(forepid, 2);
		forepid = -1;
	}

	signal(SIGINT, inthandler);
}

void tstphandler(int signum)
{
	if (forepid == mainpid)
		signal(SIGTSTP, tstphandler);

	else if (forepid > 0)
	{
		kill(forepid, 19);
		forepid = -1;
	}

	signal(SIGTSTP, tstphandler);
}

proc procs[256];
proc temps[256];

int proccount = 0;
int jobnum = 1;

void chldhandler(int signum)
{
	int i;
	int tempcount = 0;
	int temppid;
	int status;

	temppid = waitpid(-1, &status, WNOHANG);

	for (i = 0; i < proccount; i++)
	{
		temppid = waitpid(procs[i].pid, &status, WNOHANG);

		if (kill(procs[i].pid, 0) == -1 && errno == ESRCH)
		{
			if (WIFEXITED(status) == 1)
			{
				fprintf(stderr, "[%d] %s with pid %d has exited normally.\n", procs[i].num, procs[i].p_name, procs[i].pid);
			}

			else
			{
				fprintf(stderr, "[%d] %s with pid %d has exited abnormally.\n", procs[i].num, procs[i].p_name, procs[i].pid);
			}

			prompt();
			fflush(stdout);
		}
		else
		{
			strcpy(temps[tempcount].p_name, procs[i].p_name);
			temps[tempcount].pid = procs[i].pid;
			temps[tempcount].num = procs[i].num;
			strcpy(temps[tempcount].State, procs[i].State);
			tempcount++;
		}
	}

	for (i = 0; i < tempcount; i++)
	{
		strcpy(procs[i].p_name, temps[i].p_name);
		procs[i].pid = temps[i].pid;
		procs[i].num = temps[i].num;
		strcpy(procs[i].State, temps[i].State);
	}

	proccount = tempcount;

	if (proccount > 0)
	{
		jobnum = procs[proccount - 1].num + 1;
	}

	else
	{
		jobnum = 1;
	}
}

int main()
{
	ttyd = open("/dev/tty", O_RDWR, 0700);
	tcsetpgrp(ttyd, getpid());

	mainpid = getpid();

	saved_stdout = dup(1);
	saved_stdin = dup(0);

	while (1)
	{
		signal(SIGINT, inthandler);
		signal(SIGTSTP, tstphandler);

		signal(SIGCHLD, chldhandler);

		dup2(saved_stdin, 0);

		dup2(saved_stdout, 1);

		int inpflag;
		int outpflag;
		int appflag;

		inpflag = 0;
		outpflag = 0;
		appflag = 0;

		char inputfile[32];
		char outputfile[32][16];
		char appendfile[32][16];
		
		int appcount;
		int outpcount;

		outpcount = 0;

		int errout;

		errout = 0;

		int fdin;
		int fdout;
		int fdappend;

		int pipearr[2];
		int pipefin = 0;
		int newchild;

		prompt();

		int i;
		int j;
		int k;
		int c;

		i = 0;
		j = 0;
		k = 0;
		c = 0;
		
		char* inp;
		size_t inplen = 0;
		ssize_t inpreadbytes;

		inpreadbytes = getline(&inp, &inplen, stdin);

		char *temppipe;
		char pipecommands[10][256];

		temppipe = strtok(inp, "|\n");
		while (temppipe != NULL)
		{
			strcpy(pipecommands[i], temppipe);
			i++;
			temppipe = strtok(NULL, "|\n");
		}

		pipecommands[i][0] = '\0';

		i = 0;

		/*for (j = 0; pipecommands[j][0] != '\0'; j++)
			printf("%s\n", pipecommands[j]);
		printf("ayy\n");*/
		if (pipecommands[1][0] == '\0')
		{
			i = 0;
			char *temp;
			char tokens[10][256];

			temp = strtok(pipecommands[0], "; \n");

			while (temp != NULL)
			{
				strcpy(tokens[i], temp);
				i++;
				temp = strtok(NULL, "; \n");
			}

			tokens[i][0] = '\0';

			/*for (j = 0; tokens[j][0] != '\0'; j++)
			{
			printf("%s\n", tokens[j]);
			}

			printf("lmao\n");*/

			if (!strcmp(tokens[0], "quit"))
			{
				if (tokens[1][0] == '\0')
				{
					overkill();
					printf("Bye\n");
					exit(0);
				}

				else
				{
					fprintf(stderr, "Error: Expecting no arguments to function");
				}
			}

			for (i = 0; tokens[i][0] != '\0'; i++)
			{
				if (i == 0)
				{
					continue;
				}

				if (!strcmp(tokens[i], "<"))
				{
					inpflag = 1;
					tokens[i][0] = '\0';
					k = i;
					while (tokens[k + 1][0] != '\0' && tokens[k + 1][0] != '<' && tokens[k + 1][0] != '>')
					{
						strcpy(inputfile, tokens[k + 1]);
						k++;
					}
				}

				if (!strcmp(tokens[i], ">"))
				{
					outpflag = 1;
					tokens[i][0] = '\0';
					k = i;
					while (tokens[k + 1][0] != '\0' && tokens[k + 1][0] != '<' && tokens[k + 1][0] != '>')
					{
						strcpy(outputfile[outpcount], tokens[k + 1]);
						outpcount++;
						k++;
					}
				}

				if (!strcmp(tokens[i], ">>"))
				{
					outpflag = 2;
					tokens[i][0] = '\0';
					k = i;
					while (tokens[k + 1][0] != '\0' && tokens[k + 1][0] != '<' && tokens[k + 1][0] != '>')
					{
						strcpy(appendfile[appcount], tokens[k + 1]);
						appcount++;
						k++;
					}
				}
			}

			if (inpflag == 1)
			{
				fdin = open(inputfile, O_RDONLY);

				if (fdin < 0)
				{
					perror("Error");
					continue;
				}

				dup2(fdin, 0);
				close(fdin);
			}

			if (outpflag == 1)
			{
				for (j = 0; j < outpcount; j++)
				{
					fdout = open(outputfile[j], O_WRONLY | O_TRUNC | O_CREAT, 0644);
					if (fdout < 0)
					{
						perror("Error");
						errout = 1;
						break;
					}

					close(fdout);
				}

				if (errout == 1)
				{
					continue;
				}

				fdout = open(outputfile[outpcount - 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (fdout < 0)
				{
					perror("Error");
					continue;
				}

				dup2(fdout, 1);
				close(fdout);

			}

			else if (outpflag == 2)
			{
				for (j = 0; j < appcount; j++)
				{
					fdappend = open(appendfile[j], O_WRONLY | O_APPEND | O_CREAT, 0644);
					if (fdappend < 0)
					{
						perror("Error");
						errout = 1;
						break;
					}

					close(fdappend);
				}

				if (errout == 1)
				{
					continue;
				}

				fdappend = open(appendfile[appcount - 1], O_WRONLY | O_APPEND | O_CREAT, 0644);
				if (fdappend < 0)
				{
					perror("Error");
					continue;
				}

				dup2(fdappend, 1);
				close(fdappend);
			}

			if (!strcmp(tokens[0], "cd"))
			{
				if (tokens[2][0] == '\0')
				{
					cd(tokens[1]);
				}

				else if (tokens[2][0] == '&')
				{
					cd(tokens[1]);
				}

				else if (tokens[1][0] == '&')
				{
					if (tokens[2][0] != '\0')
						cd(tokens[2]);
				}

				else
				{
					fprintf(stderr, "Error: Expecting only one argument to function.\n");
				}
			}

			else if (!strcmp(tokens[0], "pwd"))
			{
				if (tokens[1][0] == '\0')
				{
					pwd();
				}

				else if (tokens[1][0] == '&')
				{
					pwd();
				}

				else
				{
					fprintf(stderr, "Error: Expecting no arguments to function.\n");
				}
			}

			else if (!strcmp(tokens[0], "echo"))
			{
				echo(tokens);
			}

			else if (!strcmp(tokens[0], "ls"))
			{
				ls(tokens);
			}

			else if (!strcmp(tokens[0], "pinfo"))
			{
				if ((strcmp(tokens[1], "&") && tokens[2][0] == '\0') || tokens[1][0] == '\0')
				{
					pinfo(tokens[1]);
				}

				else if (!strcmp(tokens[1], "&"))
				{
					pinfo(tokens[2]);
				}

				else if (tokens[2][0] == '&' && tokens[3][0] == '\0')

				{
					pinfo(tokens[1]);
				}

				else
				{
					fprintf(stderr, "Error: Expecting one or less arguments to function.\n");
				}
			}

			else if (!strcmp(tokens[0], "clock"))
			{
				clocks(tokens);
			}

			else if (!strcmp(tokens[0], "remindme"))
			{
				if (tokens[1][0] == '\0')
				{
					fprintf(stderr, "Error: Expecting two arguments to function.\n");
				}

				else if (tokens[2][0] == '\0')
				{
					fprintf(stderr, "Error: Expecting two arguments to function.\n");
				}

				else if (tokens[3][0] == '\0')
				{
					remind_me(tokens);
				}

				else
				{
					fprintf(stderr, "Error: Expecting two arguments to function.\n");
				}
			}

			else if (!strcmp(tokens[0], "overkill"))
			{
				if (tokens[1][0] == '\0')
				{
					overkill();
				}

				else
				{
					fprintf(stderr, "Error: Expecting no arguments to function.\n");
				}
			}

			else if (!strcmp(tokens[0], "setenv"))
			{
				if (tokens[1][0] == '\0')
				{
					fprintf(stderr, "Error: Expecting variable name.\n");
				}

				else if (tokens[2][0] == '\0' || tokens[3][0] == '\0')
				{
					setenviron(tokens);
				}

				else
				{
					fprintf(stderr, "Error: Expecting one or two arguments to function.\n");
				}

			}

			else if (!strcmp(tokens[0], "unsetenv"))
			{
				if (tokens[1][0] == '\0')
				{
					fprintf(stderr, "Error: Expecting variable name.\n");
				}

				else if (tokens[2][0] == '\0')
				{
					unsetenviron(tokens);
				}

				else
				{
					fprintf(stderr, "Error: Expecting one argument to function.\n");
				}

			}

			else if (!strcmp(tokens[0], "jobs"))
			{
				if (tokens[1][0] == '\0')
				{
					jobs();
					//printf("%d\n", proccount);
				}

				else
				{
					fprintf(stderr, "Error: Expecting no arguments to function.\n");
				}
			}

			else if (!strcmp(tokens[0], "kjob"))
			{
				if (tokens[1][0] == '\0' || tokens[2][0] == '\0' || tokens[3][0] != '\0')
				{
					fprintf(stderr, "Error: Expecting three arguments to function.\n");
				}

				else
				{
					kjob(tokens);
				}
			}

			else if (!strcmp(tokens[0], "fg"))
			{
				if (tokens[1][0] == '\0')
				{
					fprintf(stderr, "Error: Expecting one argument to function.\n");
				}

				else if (tokens[2][0] == '\0')
				{
					fg(tokens);
				}

				else
				{
					fprintf(stderr, "Error: Expecting one argument to function.\n");
				}
			}

			else if (!strcmp(tokens[0], "bg"))
			{
				if (tokens[1][0] == '\0')
				{
					fprintf(stderr, "Error: Expecting one argument to function.\n");
				}

				else if (tokens[2][0] == '\0')
				{
					bg(tokens);
				}

				else
				{
					fprintf(stderr, "Error: Expecting one argument to function.\n");
				}
			}

			else
			{
				execute(tokens);
			}
		}

		else
		{
			for (c = 0; pipecommands[c][0] != '\0'; c++)
			{
				i = 0;
				char *temp;
				char tokens[10][256];

				temp = strtok(pipecommands[c], "; \n\0");

				while (temp != NULL)
				{
					strcpy(tokens[i], temp);
					i++;
					temp = strtok(NULL, "; \n\0");
				}

				tokens[i][0] = '\0';

				/*for (j = 0; tokens[j][0] != '\0'; j++)
				{
					printf("%s\n", tokens[j]);
				}

				printf("lmao\n");*/

				if (!strcmp(tokens[0], "quit"))
				{
					if (tokens[1][0] == '\0')
					{
						overkill();
						printf("Bye\n");
						exit(0);
					}

					else
					{
						fprintf(stderr, "Error: Expecting no arguments to function");
					}
				}

				pipe(pipearr);

				newchild = fork();

				if (newchild == 0)
				{
					dup2(pipefin, 0);

					if (pipecommands[c + 1][0] != '\0')
					{
						dup2(pipearr[1], 1);
					}

					else
					{
						dup2(saved_stdout, 1);
					}

					close(pipearr[0]);

					for (i = 0; tokens[i][0] != '\0'; i++)
					{
						if (i == 0)
						{
							continue;
						}

						if (!strcmp(tokens[i], "<"))
						{
							inpflag = 1;
							tokens[i][0] = '\0';
							k = i;
							while (tokens[k + 1][0] != '\0' && tokens[k + 1][0] != '<' && tokens[k + 1][0] != '>')
							{
								strcpy(inputfile, tokens[k + 1]);
								k++;
							}
						}

						if (!strcmp(tokens[i], ">"))
						{
							outpflag = 1;
							tokens[i][0] = '\0';
							k = i;
							while (tokens[k + 1][0] != '\0' && tokens[k + 1][0] != '<' && tokens[k + 1][0] != '>')
							{
								strcpy(outputfile[outpcount], tokens[k + 1]);
								outpcount++;
								k++;
							}
						}

						if (!strcmp(tokens[i], ">>"))
						{
							outpflag = 2;
							tokens[i][0] = '\0';
							k = i;
							while (tokens[k + 1][0] != '\0' && tokens[k + 1][0] != '<' && tokens[k + 1][0] != '>')
							{
								strcpy(appendfile[appcount], tokens[k + 1]);
								appcount++;
								k++;
							}
						}
					}

					if (inpflag == 1)
					{
						fdin = open(inputfile, O_RDONLY);

						if (fdin < 0)
						{
							perror("Error");
							continue;
						}

						dup2(fdin, 0);
						close(fdin);
					}

					if (outpflag == 1)
					{
						for (j = 0; j < outpcount; j++)
						{
							fdout = open(outputfile[j], O_WRONLY | O_TRUNC | O_CREAT, 0644);
							if (fdout < 0)
							{
								perror("Error");
								errout = 1;
								break;
							}

							close(fdout);
						}

						if (errout == 1)
						{
							continue;
						}

						fdout = open(outputfile[outpcount - 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
						if (fdout < 0)
						{
							perror("Error");
							continue;
						}

						dup2(fdout, 1);
						close(fdout);

					}

					else if (outpflag == 2)
					{
						for (j = 0; j < appcount; j++)
						{
							fdappend = open(appendfile[j], O_WRONLY | O_APPEND | O_CREAT, 0644);
							if (fdappend < 0)
							{
								perror("Error");
								errout = 1;
								break;
							}

							close(fdappend);
						}

						if (errout == 1)
						{
							continue;
						}

						fdappend = open(appendfile[appcount - 1], O_WRONLY | O_APPEND | O_CREAT, 0644);
						if (fdappend < 0)
						{
							perror("Error");
							continue;
						}

						dup2(fdappend, 1);
						close(fdappend);
					}

					if (!strcmp(tokens[0], "cd"))
					{
						if (tokens[2][0] == '\0')
						{
							cd(tokens[1]);
						}

						else if (tokens[2][0] == '&')
						{
							cd(tokens[1]);
						}

						else if (tokens[1][0] == '&')
						{
							if (tokens[2][0] != '\0')
								cd(tokens[2]);
						}

						else
						{
							fprintf(stderr, "Error: Expecting only one argument to function.\n");
						}
					}

					else if (!strcmp(tokens[0], "pwd"))
					{
						if (tokens[1][0] == '\0')
						{
							pwd();
						}

						else if (tokens[1][0] == '&')
						{
							pwd();
						}

						else
						{
							fprintf(stderr, "Error: Expecting no arguments to function.\n");
						}
					}

					else if (!strcmp(tokens[0], "echo"))
					{
						echo(tokens);
					}

					else if (!strcmp(tokens[0], "ls"))
					{
						ls(tokens);
					}

					else if (!strcmp(tokens[0], "pinfo"))
					{
						if ((strcmp(tokens[1], "&") && tokens[2][0] == '\0') || tokens[1][0] == '\0')
						{
							pinfo(tokens[1]);
						}

						else if (!strcmp(tokens[1], "&"))
						{
							pinfo(tokens[2]);
						}

						else if (tokens[2][0] == '&' && tokens[3][0] == '\0')

						{
							pinfo(tokens[1]);
						}

						else
						{
							fprintf(stderr, "Error: Expecting one or less arguments to function.\n");
						}
					}

					else if (!strcmp(tokens[0], "clock"))
					{
						clocks(tokens);
					}

					else if (!strcmp(tokens[0], "remindme"))
					{
						if (tokens[1][0] == '\0')
						{
							fprintf(stderr, "Error: Expecting two arguments to function.\n");
						}

						else if (tokens[2][0] == '\0')
						{
							fprintf(stderr, "Error: Expecting two arguments to function.\n");
						}

						else if (tokens[3][0] == '\0')
						{
							remind_me(tokens);
						}

						else
						{
							fprintf(stderr, "Error: Expecting two arguments to function.\n");
						}
					}

					else if (!strcmp(tokens[0], "overkill"))
					{
						if (tokens[1][0] == '\0')
						{
							overkill();
						}

						else
						{
							fprintf(stderr, "Error: Expecting no arguments to function.\n");
						}
					}

					else if (!strcmp(tokens[0], "setenv"))
					{
						if (tokens[1][0] == '\0')
						{
							fprintf(stderr, "Error: Expecting variable name.\n");
						}

						else if (tokens[2][0] == '\0' || tokens[3][0] == '\0')
						{
							setenviron(tokens);
						}

						else
						{
							fprintf(stderr, "Error: Expecting one or two arguments to function.\n");
						}

					}

					else if (!strcmp(tokens[0], "unsetenv"))
					{
						if (tokens[1][0] == '\0')
						{
							fprintf(stderr, "Error: Expecting variable name.\n");
						}

						else if (tokens[2][0] == '\0')
						{
							unsetenviron(tokens);
						}

						else
						{
							fprintf(stderr, "Error: Expecting one argument to function.\n");
						}

					}

					else if (!strcmp(tokens[0], "jobs"))
					{
						if (tokens[1][0] == '\0')
						{
							jobs();
							printf("%d\n", proccount);
						}

						else
						{
							fprintf(stderr, "Error: Expecting no arguments to function.\n");
						}
					}

					else if (!strcmp(tokens[0], "kjob"))
					{
						if (tokens[1][0] == '\0' || tokens[2][0] == '\0' || tokens[3][0] != '\0')
						{
							fprintf(stderr, "Error: Expecting three arguments to function.\n");
						}

						else
						{
							kjob(tokens);
						}
					}

					else if (!strcmp(tokens[0], "fg"))
					{
						if (tokens[1][0] == '\0')
						{
							fprintf(stderr, "Error: Expecting one argument to function.\n");
						}

						else if (tokens[2][0] == '\0')
						{
							fg(tokens);
						}

						else
						{
							fprintf(stderr, "Error: Expecting one argument to function.\n");
						}
					}

					else if (!strcmp(tokens[0], "bg"))
					{
						if (tokens[1][0] == '\0')
						{
							fprintf(stderr, "Error: Expecting one argument to function.\n");
						}

						else if (tokens[2][0] == '\0')
						{
							bg(tokens);
						}

						else
						{
							fprintf(stderr, "Error: Expecting one argument to function.\n");
						}
					}

					else
					{
						execute(tokens);
					}

					exit(0);
				}

				else
				{
					wait(NULL);
					close(pipearr[1]);
					pipefin = pipearr[0];
				}
			}
		}
	}

	return 0;
}