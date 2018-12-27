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

int cd(char inp[])
{
	int i;

	if (inp[0] == '~')
	{
		char proc_loc[64];
		char homedir[256];

		sprintf(proc_loc, "/proc/%d/exe", getpid());

		int bytes = min(readlink(proc_loc, homedir, 256), 255);
		if (bytes >= 0)
		{
			homedir[bytes - 4] = '\0';
			bytes -= 4;
		}

		else
		{
			perror("Error");
		}

		for (i = 1; inp[i] != '\0'; i++)
		{
			inp[i - 1] = inp[i];
		}

		inp[i - 1] = '\0';
		inp[i] = 'a';

		char temp[256];

		strcpy(temp, inp);
		strcat(homedir, temp);
		strcpy(inp, homedir);
	}

	if (chdir(inp) == -1)
	{
		perror("Error");
	}

	return 0;
}

int pwd()
{
	char cwd[512];

	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		perror("Error");
		strcpy(cwd, "???");
	}

	printf("%s\n", cwd);

	return 0;
}

int echo(char inp[10][256])
{
	int i;
	int j;
	int squote = 0;

	for (i = 1; inp[i][0] != '\0'; i++)
	{
		if (inp[i][0] == '$' && squote == 0)
		{
			for (j = 1; inp[i][j] != '\0'; j++)
			{
				inp[i][j - 1] = inp[i][j];
			}

			inp[i][j - 1] = '\0';

			printf("%s ", getenv(inp[i]));
			continue;
		}

		printf("%s ", inp[i]);
	}

	printf("\n");
	return 0;
}

int hidden(const struct dirent* dir)
{
	if (dir->d_name[0] == '.')
		return 0;
	else
		return 1;
}

int hiddensort(const struct dirent** dir1, const struct dirent** dir2)
{
	const char* d1;
	const char* d2;

	d1 = (*dir1)->d_name;
	d2 = (*dir2)->d_name;

	if (d1[0] == '.')
	{
		d1++;
	}

	if (d2[0] == '.')
	{
		d2++;
	}

	return strcmp(d1, d2);
}

int ls(char inp[10][256])
{
	int loc[50];
	int dirno = 0;

	int lflag = 0;
	int aflag = 0;
	int i = 1;
	int j = 0;

	while (inp[i][0] != '\0')
	{
		if (inp[i][0] == '-')
		{
			for (j = 1; inp[i][j] != '\0'; j++)
			{
				if (inp[i][j] == 'l')
					lflag = 1;
				if (inp[i][j] == 'a')
					aflag = 1;
			}

			if (j == 1)
			{
				printf("Error: Please enter flag name completely.\n");
			}
		}

		else if (!strcmp(inp[i], "&"))
		{
			i++;
			continue;
		}

		else
		{
			loc[dirno] = i;
			dirno++;
		}

		i++;
	}

	char dirnames[16][128];
	char fulldirpath[128];

	getcwd(fulldirpath, sizeof(fulldirpath));
	char temp[256];

	char proc_loc[64];
	char homedir[256];

	sprintf(proc_loc, "/proc/%d/exe", getpid());

	int bytes = min(readlink(proc_loc, homedir, 256), 255);
	if (bytes >= 0)
	{
		homedir[bytes - 4] = '\0';
		bytes -= 4;
	}

	if (dirno == 0)
	{
		strcpy(dirnames[0], ".");
		dirno++;
	}

	else
	{
		for (i = 0; i < dirno; i++)
		{
			if (inp[loc[i]][0] == '~')
			{
				for (j = 1; inp[loc[i]][j] != '\0'; j++)
				{
					inp[loc[i]][j - 1] = inp[loc[i]][j];
				}

				inp[loc[i]][j - 1] = '\0';

				strcpy(dirnames[i], homedir);

				if (inp[loc[i]][0] != '/')
				{
					strcat(dirnames[i], "/");
				}

				strcat(dirnames[i], inp[loc[i]]);
			}
			else
				strcpy(dirnames[i], inp[loc[i]]);
		}

		dirnames[dirno][0] = '\0';
	}

	for (i = 0; i < dirno; i++)
	{
		struct dirent** files;
		int retscandir;

		if (aflag == 1)
		{
			retscandir = scandir(dirnames[i], &files, NULL, hiddensort);
			if (retscandir < 0)
			{
				perror("Error");
				return 0;
			}
		}

		else
		{
			retscandir = scandir(dirnames[i], &files, hidden, alphasort);
			if (retscandir < 0)
			{
				perror("Error");
				return 0;
			}
		}

		char fullpath[256];
		char file[128];

		if (lflag == 0)
		{
			for (j = 0; j < retscandir; j++)
			{
				strcpy(file, files[j]->d_name);
				strcpy(fullpath, dirnames[i]);
				strcat(fullpath, "/");
				strcat(fullpath, file);

				struct stat fil;
				if (stat(fullpath, &fil) < 0)
				{
					perror("Error");
					continue;
				}

				if (S_ISDIR(fil.st_mode))
				{
					printf("%s%s%s  ", "\x1B[1;34m", file, "\x1B[0m");
				}

				else if (fil.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
				{
					printf("%s%s%s  ", "\x1B[1;32m", file, "\x1B[0m");
				}

				else
				{
					printf("%s  ", file);
				}

			}
			printf("\n");
		}

		else
		{
			struct stat dir;
			long int blocks = 0;

			for (j = 0; j < retscandir; j++)
			{
				strcpy(file, files[j]->d_name);
				strcpy(fullpath, dirnames[i]);
				strcat(fullpath, "/");
				strcat(fullpath, file);

				struct stat filblock;

				if (stat(fullpath, &filblock) < 0)
				{
					perror("Error");
					continue;
				}

				blocks += filblock.st_blocks;
			}

			stat(dirnames[i], &dir);

			printf("total %ld\n", blocks / 2);

			for (j = 0; j < retscandir; j++)
			{
				strcpy(file, files[j]->d_name);
				strcpy(fullpath, dirnames[i]);
				strcat(fullpath, "/");
				strcat(fullpath, file);

				struct stat fil;

				if (stat(fullpath, &fil) < 0)
				{
					perror("Error");
					continue;
				}

				if (S_ISDIR(fil.st_mode))
					printf("d");
				else
					printf("-");

				if (fil.st_mode & S_IRUSR)
					printf("r");
				else
					printf("-");

				if (fil.st_mode & S_IWUSR)
					printf("w");
				else
					printf("-");

				if (fil.st_mode & S_IXUSR)
					printf("x");
				else
					printf("-");

				if (fil.st_mode & S_IRGRP)
					printf("r");
				else
					printf("-");

				if (fil.st_mode & S_IWGRP)
					printf("w");
				else
					printf("-");

				if (fil.st_mode & S_IXGRP)
					printf("x");
				else
					printf("-");

				if (fil.st_mode & S_IROTH)
					printf("r");
				else
					printf("-");

				if (fil.st_mode & S_IWOTH)
					printf("w");
				else
					printf("-");

				if (fil.st_mode & S_IXOTH)
					printf("x");
				else
					printf("-");

				printf(" %lu ", fil.st_nlink);

				struct passwd* pw;
				struct group* grp;

				pw = getpwuid(fil.st_uid);
				grp = getgrgid(fil.st_gid);

				printf("%s %s ", pw->pw_name, grp->gr_name);

				printf("%5ld ", fil.st_size);

				char timestr[128];
				const struct tm* time = localtime(&fil.st_mtime);
				const char format[16] = "%b %e %R ";
				strftime(timestr, sizeof(timestr), format, time);

				printf("%s ", timestr);

				if (S_ISDIR(fil.st_mode))
				{
					printf("%s%s%s\n", "\x1B[1;34m", file, "\x1B[0m");
				}

				else if (fil.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
				{
					printf("%s%s%s\n", "\x1B[1;32m", file, "\x1B[0m");
				}

				else
				{
					printf("%s\n", file);
				}
			}
		}
	}
	return 0;
}

int clocks(char inp[][256])
{
	signal(SIGINT, sighandler);
	int t;

	if (!strcmp(inp[1], "-t"))
	{
		if (inp[2][0] != '\0')
		{
			if (inp[3][0] != '\0')
			{
				fprintf(stderr, "Error: Expecting only one argument for current flag.\n");
				return 0;
			}

			t = atoi(inp[2]);

			if (t <= 0)
			{
				fprintf(stderr, "Please enter a valid natural number.\n");
				return 0;
			}
		}

		else
		{
			fprintf(stderr, "Error: Expecting argument to given function.\n");
			return 0;
		}
	}

	else
	{
		if (inp[1][0] != '\0')
		{
			fprintf(stderr, "Error: Expecting no arguments to given function.\n");
			return 0;
		}

		t = 1;
	}

	extern int end;

	while (!end)
	{
		struct tm* tim;

		time_t loc;
		time(&loc);

		tim = localtime(&loc);

		char timestr[128];
		tim = localtime(&loc);
		char format[16] = "%d %b %Y, %X";
		strftime(timestr, sizeof(timestr), format, tim);

		printf("%s\n", timestr);
		sleep(t);
	}

	end = 0;
}

int remind_me(char inp[][256])
{
	int pid;
	int t = atoi(inp[1]);

	if (t <= 0)
	{
		fprintf(stderr, "Please enter a valid natural number for number of seconds.\n");
		return 0;
	}

	char reminder[256];

	strcpy(reminder, inp[2]);

	if ((pid = fork()) < 0)
	{
		fprintf(stderr, "Error: Execution of remindme could not be forked.\n");
		exit(1);
	}

	else if (pid == 0)
	{
		sleep(t);
		printf("\nReminder: %s\n", reminder);
		prompt();
		exit(0);
	}
}

int overkill()
{
	int i;
	extern int proccount;
	extern proc procs[256];

	for (i = 0; i < proccount; i++)
	{
		if (kill(procs[i].pid, SIGKILL) != -1)
		{
			fprintf(stdout, "%s with pid %d has been killed.\n", procs[i].p_name, procs[i].pid);
		}
		else
		{
			perror("Error");
		}
	}

	proccount = 0;

	return 0;
}

int setenviron(char inp[][256])
{
	if (inp[2][0] == '\0')
	{
		if (setenv(inp[1], "", 1) < 0)
			perror("Error");
	}

	else
	{
		if (setenv(inp[1], inp[2], 1) < 0)
			perror("Error");
	}

	return 0;
}

int unsetenviron(char inp[][256])
{
	if (unsetenv(inp[1]) < 0)
		perror("Error");
}


int jobs()
{
	int i;
	extern int proccount;
	extern proc procs[256];

	//printf("Count me in %d\n", proccount);

	for (i = 0; i < proccount; i++)
	{
		printf("[%d]\t%s\t%s [%d]\n", procs[i].num, procs[i].State, procs[i].p_name, procs[i].pid);
	}

	return 0;
}

int kjob(char inp[][256])
{
	int i;
	int job;
	int sig;
	extern int proccount;
	extern proc procs[256];

	job = atoi(inp[1]);
	sig = atoi(inp[2]);

	for (i = 0; i < proccount; i++)
	{
		if (procs[i].num == job)
		{
			if (kill(procs[i].pid, sig) < 0)
			{
				perror("Error");
				return 0;
			}

			return 0;
		}
	}

	fprintf(stderr, "Error: No such job exists.\n");
	return 0;
}