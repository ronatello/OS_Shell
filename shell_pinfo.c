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

int pinfo(char inp[])
{
	struct Pinfo {
		pid_t pid;
		char proc_status[5];
		int vm;
		char exepath[256];
	};

	struct Pinfo pinfo;

	pid_t pid;

	if (inp[0] == '\0' || inp[0] == '&')
	{
		pinfo.pid = getpid();
	}

	else
	{
		pinfo.pid = atoi(inp);
	}

	char proc_loc[64];
	sprintf(proc_loc, "/proc/%d/", pinfo.pid);

	char* line;
	size_t linelen = 0;
	FILE* stream;

	stream = fopen(strcat(proc_loc, "status"), "r");

	if (stream == NULL)
	{
		fprintf(stderr, "Error: Process with pid %d does not exist.\n", pinfo.pid);
		return 0;
	}

	int i = 0;

	for (i = 0; i < 3; i++)
	{
		getline(&line, &linelen, stream);
	}

	char* temp;

	temp = strtok(line, " \t");
	temp = strtok(NULL, " \t");

	strcpy(pinfo.proc_status, temp);

	for (i = 3; i < 18; i++)
	{
		getline(&line, &linelen, stream);
	}

	temp = strtok(line, " \t");

	if (strcmp(temp, "VmSize:") == 0)
	{
		temp = strtok(NULL, " ");
		pinfo.vm = atoi(temp);
	}

	else
	{
		pinfo.vm = 0;
	}

	fclose(stream);

	sprintf(proc_loc, "/proc/%d/exe", pinfo.pid);

	int nullbyte = min(readlink(proc_loc, pinfo.exepath, sizeof(pinfo.exepath)), 255);

	if (nullbyte > 0)
	{
		pinfo.exepath[nullbyte] = '\0';
	}

	else
	{
		perror("Error");
		strcpy(pinfo.exepath, "Unknown");
	}

	printf("pid -- %d\nProcess Status -- %s\nMemory -- %d kB (Virtual Memory)\nExecutable Path -- %s\n", pinfo.pid, pinfo.proc_status, pinfo.vm, pinfo.exepath);
}