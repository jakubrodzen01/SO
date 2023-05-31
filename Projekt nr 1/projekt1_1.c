#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{

printf("PID\tPPID\tUID\tGID\n");

printf("%d\t%d\t%d\t%d\n", getpid(), getppid(), getuid(), getgid());

exit(0);
}
