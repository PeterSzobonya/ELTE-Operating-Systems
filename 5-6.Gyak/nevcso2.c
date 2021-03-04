#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h> // for errno, the number of last error

int main(int argc, char *argv[])
{
    int pid, fd;
    printf("Fifo start!\n");
    char pipename[20];
    char sz[100];
    // In most of system not required full path,
    // enough a simple name, eg. alma.fa
    // In Debian must define full path name,
    // so best place is in Debian the /tmp dir.
    sprintf(pipename, "/tmp/%d", getpid());
    int fid = mkfifo(pipename, S_IRUSR | S_IWUSR); // creating named pipe file
    // S_IWGRP, S_IROTH (other jog), file permission mode
    // the file name: fifo.ftc
    // the real fifo.ftc permission is: mode & ~umask
    if (fid == -1)
    {
        printf("Error number: %i", errno);
        perror("Gaz van:");
        exit(EXIT_FAILURE);
    }
    printf("Mkfifo vege, fork indul!\n");
    pid = fork();

    if (pid > 0) //parent
    {
        int sum = 0;
        int szame = 0;
        printf("Csonyitas eredmenye szuloben: %d!\n", fid);
        fd = open(pipename, O_RDONLY);
        while (strcmp("over", sz) != 0)
        {

            read(fd, sz, sizeof(sz)); // reading max 100 chars
            for (int i = 0; i < strlen(sz); ++i)
            {
                if (isdigit(sz[i]) == 0)
                {
                    szame = 0;
                    break;
                }
                else
                {
                    szame = 1;
                }
            }
            sum += atoi(sz);
            printf("Gyerek olvasta uzenet: %d", sum);
            printf("\n");
        }
        close(fd);
        // remove fifo.ftc
        unlink(pipename);
    }
    else // child
    {
        printf("Gyerek vagyok, irok a csobe!\n");
        printf("Csonyitas eredmenye: %d!\n", fid);
        fd = open(pipename, O_WRONLY);
        char msg[100];
        do
        {
            printf("Adj meg egy szamot!:\n ");
            scanf("%s", msg);
            printf("Gyerek olvasott stdin-ről: %s\n", msg);
            write(fd, msg, strlen(msg) + 1);
        } while (strcmp(msg, "over") != 0);

        close(fd);
        printf("Gyerek vagyok, beirtam, vegeztem!\n");
    }

    return 0;
}
