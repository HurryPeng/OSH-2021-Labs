#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sysmacros.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>

void callSubprocess(int id)
{
    char procName[16];
    sprintf(procName, "./%d", id);
    execl(procName,procName, NULL);
}

int main()
{
    printf("Creating devices...\n");

    bool success = true;
    if (mknod("/dev/ttyS0", S_IFCHR | S_IRUSR | S_IWUSR | S_IXUSR, makedev(4, 64)) == -1) success = false;
    if (mknod("/dev/ttyAMA0", S_IFCHR | S_IRUSR | S_IWUSR | S_IXUSR, makedev(204, 64)) == -1) success = false;
    if (mknod("/dev/fb0", S_IFCHR | S_IRUSR | S_IWUSR | S_IXUSR, makedev(29, 0)) == -1) success = false;

    if (success) printf("Device creation successful! \n");
    else printf ("Device creation failed! \n");

    for (int i = 1; i <= 3; ++i)
    {
        printf("Executing task %d: \n", i);
        pid_t pid = fork();
        if (pid < 0) printf("Fork failed! \n");
        else if (pid == 0) // Subprocess
        {
            callSubprocess(i);
        }
        else // Main process
        {
            waitpid(pid, NULL, 0);
        }
    }

    printf("Finished! \n");
    
    while (true)
    {
        int temp = 0;
        scanf("%d", &temp);
        printf("%d\n", temp);
    }

    return 0;
}
