typedef struct processes {
	int pid;
	char p_name[32];
	int num;
	char State[32];
}proc;

void sighandler(int signum);
void inthandler(int signum);
void tstphandler(int signum);
void chldhandler(int signum);
int min(int a, int b);
int prompt();
int overkill();
int jobs();
int kjob(char inp[][256]);
int fg(char inp[][256]);
int bg(char inp[][256]);
int setenviron(char inp[][256]);
int unsetenviron(char inp[][256]);
int cd(char inp[]);
int pwd();
int echo(char inp[10][256]);
int hidden(const struct dirent* dir);
int hiddensort(const struct dirent** dir1, const struct dirent** dir2);
int ls(char inp[10][256]);
int clocks(char inp[][256]);
int remind_me(char inp[][256]);
int execute(char inp[][256]);
int pinfo(char inp[]);