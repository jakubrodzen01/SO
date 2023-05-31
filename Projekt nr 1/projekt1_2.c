#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
char command[50];
sprintf(command, "pstree -p %d", getpid());
for(int i=0; i<3; i++)
{
        if(fork() == 0)
        {
                printf("%d\t%d\t%d\t%d\n", getpid(), getppid(), getuid(), getgid());
        }
}
system(command);
sleep(1);
exit(0);
}
