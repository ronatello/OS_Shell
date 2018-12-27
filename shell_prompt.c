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

int min(int a, int b)
{
	if (a <= b)
		return a;
	else
		return b;
}

int prompt()
{
	struct passwd *pw;
	uid_t uid;
	const char* usrname;

	char prompt[100];

	uid = geteuid();
	pw = getpwuid(uid);
	if (pw)
	{
		usrname = pw->pw_name;
	}

	else
	{
		perror("Error");
		//fprintf(stderr, "ERROR: Username not found. Using default name.\n");
		usrname = "user";
	}

	char hostname[1024];

	int err;

	hostname[1023] = '\0';
	err = gethostname(hostname, 1023);

	if (err != 0)
	{
		perror("Error");
		//fprintf(stderr, "ERROR: Hostname could not be retrieved. Using default hostname.\n");
		strcpy(hostname, "computer");
	}

	char cwd[4096];

	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		perror("Error");
		//fprintf(stderr, "ERROR: Current working directory could not be found.\n");
		strcpy(cwd, "???");
	}

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

	int flag = 0;
	int i;

	for (i = 0; homedir[i] != '\0'; i++)
	{
		if (homedir[i] != cwd[i])
		{
			flag = 1;
			break;
		}
	}

	if (flag == 1)
	{
		strcpy(prompt, cwd);
	}

	else
	{
		prompt[0] = '~';
		for (i = bytes; cwd[i] != '\0'; i++)
		{
			prompt[i - bytes + 1] = cwd[i];
		}

		prompt[i - bytes + 1] = '\0';
	}

	fprintf(stdout, "\x1B[1;32m%s@%s\x1B[0m:\x1B[1;34m%s$\x1B[0m ", usrname, hostname, prompt);

	extern int forepid;

	forepid = -1;

	return 0;
}
