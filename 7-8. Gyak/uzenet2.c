#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct uzenet
{
    long mtype; //ez egy szabadon hasznalhato ertek, pl uzenetek osztalyozasara
    char mtext[1024];
};

// sendig a message
int szulo(int uzenetsor)
{
    struct uzenet uz = {5, "Hajra Fradi!"};
    int status;

    status = msgsnd(uzenetsor, &uz, strlen(uz.mtext) + 1, 0);
    // a 3. param ilyen is lehet: sizeof(uz.mtext)
    // a 4. parameter gyakran IPC_NOWAIT, ez a 0-val azonos, bővebben az uzenet.c ben le van írva
    if (status < 0)
        perror("msgsnd");

    status = msgrcv(uzenetsor, &uz, 1024, 4, 0);
    if (status < 0)
        perror("msgsnd");
    else
        printf("A kapott uzenet (gyerektol) kodja: %ld, szovege:  %s\n", uz.mtype, uz.mtext);
    return 0;
}

// receiving a message.
int gyerek(int uzenetsor)
{
    struct uzenet uz = {4, "asdasd"};
    int status;
    status = msgsnd(uzenetsor, &uz, strlen(uz.mtext) + 1, 0);
    // a 3. param ilyen is lehet: sizeof(uz.mtext)
    // a 4. parameter gyakran IPC_NOWAIT, ez a 0-val azonos, bővebben az uzenet.c ben le van írva
    if (status < 0)
        perror("msgsnd");

    status = msgrcv(uzenetsor, &uz, 1024, 5, 0);
    if (status < 0)
        perror("msgsnd");
    else
        printf("A kapott uzenet (szulotol) kodja: %ld, szovege:  %s\n", uz.mtype, uz.mtext);
    return 0;
}

int main(int argc, char *argv[])
{
    pid_t child;
    int uzenetsor, status;
    key_t kulcs;

    // msgget needs a key, amelyet az ftok gener�l
    //.
    kulcs = ftok(argv[0], 1);
    printf("A kulcs: %d\n", kulcs);
    uzenetsor = msgget(kulcs, 0600 | IPC_CREAT);
    if (uzenetsor < 0)
    {
        perror("msgget");
        return 1;
    }

    child = fork();
    if (child > 0)
    {
        szulo(uzenetsor); // Parent sends a message.
        wait(NULL);
        // After terminating child process, the message queue is deleted.
        status = msgctl(uzenetsor, IPC_RMID, NULL);
        if (status < 0)
            perror("msgctl");
        return 0;
    }
    else if (child == 0)
    {
        return gyerek(uzenetsor);
        // The child process receives a message.
    }
    else
    {
        perror("fork");
        return 1;
    }

    return 0;
}