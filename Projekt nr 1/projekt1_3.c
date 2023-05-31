#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

int main()
{
//char command[50];
//sprintf(command, "pstree -p %d", getpid());

int e;

for(int i=0; i<3; i++)
{
        switch (fork())
        {
        case -1:
                perror("fork error");
                exit(1);
        case 0:
                e = execl("/usr/bin/cut", "cut", "-d:", "-f1", "/etc/group", NULL);
                if(e == -1)
                {
                        perror("exec error");
                        exit(2);
                }
                break;
        default:
                break;
        }
}

//system(command);

int w, s;
for(int i=0; i<3; i++)
{
        w = wait(&s);
        if(w  == -1)
        {
                perror("wait error");
                exit(3);
        }
        printf("PID zakonczonego procesu: %d ze statusem: %d\n", w, s);
}
exit(0);
}
