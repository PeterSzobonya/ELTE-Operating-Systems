#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for pipe()
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
//
// unnamed pipe example
//
int main(int argc, char *argv[])
{
    int pipefd[2]; // unnamed pipe file descriptor array
    pid_t pid;
    char sz[100] = "asd"; // char array for reading from pipe

    if (pipe(pipefd) == -1)
    {
        perror("Hiba a pipe nyitaskor!");
        exit(EXIT_FAILURE);
    }
    pid = fork(); // creating parent-child processes
    if (pid == -1)
    {
        perror("Fork hiba");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    { // child process
        sleep(3);         // sleeping a few seconds, not necessary
        close(pipefd[1]); // usually we close the unused write end
        printf("Gyerek elkezdi olvasni a csobol az adatokat!\n");

        while(strcmp("0",sz) != 0) {
            read(pipefd[0], sz, 3); // reading max 100 chars
            printf("Gyerek olvasta uzenet: %s", sz);
            printf("\n");
        }
        close(pipefd[0]); // finally we close the used read end
    }
    else
    { // szulo process
        printf("Szulo indul!\n");
        close(pipefd[0]); // usually we close unused read end

        srand(time(NULL));
        int r = -1;
        do {
            r = rand() % 100;
            printf("Random number %i", r);
            printf("\n");
            sprintf(sz, "%d", r); //változóba iratja a számot
            write(pipefd[1], sz, 3);
        } while(r != 0);
        
        close(pipefd[1]); // Closing write descriptor
        printf("Szulo beirta az adatokat a csobe!\n");
        fflush(NULL); // flushes all write buffers (not necessary)
        wait(NULL);   // waiting for child process (not necessary)
                      // try it without wait()
        printf("Szulo befejezte!\n");
    }
    exit(EXIT_SUCCESS); // force exit, not necessary
}
