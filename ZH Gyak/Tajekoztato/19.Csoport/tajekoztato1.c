#include "sys/types.h"
#include "unistd.h"
#include "stdlib.h"
#include "signal.h"
#include "stdio.h"
#include "string.h"
#include "time.h"
#include "wait.h"
#include "sys/ipc.h"
#include "sys/msg.h"
#include "sys/shm.h"
#include "sys/sem.h"
#include "sys/stat.h"

struct message
{
    long mtype;
    char mtext[1024];
};

struct sharedData
{
    char text[1024];
};

pid_t mainProcessValue = 0;
int ready = 0;
int messageQueue;
int semid;
struct sharedData *s;

int semaphoreCreation(const char *pathname, int semaphoreValue)
{
    int semid;
    key_t key;

    key = ftok(pathname, 1);
    if ((semid = semget(key, 1, IPC_CREAT | S_IRUSR | S_IWUSR)) < 0)
        perror("semget");
    if (semctl(semid, 0, SETVAL, semaphoreValue) < 0)
        perror("semctl");

    return semid;
}

void semaphoreOperation(int semid, int op)
{
    struct sembuf operation;

    operation.sem_num = 0;
    operation.sem_op = op;
    operation.sem_flg = 0;

    if (semop(semid, &operation, 1) < 0)
        perror("semop");
}

void semaphoreDelete(int semid)
{
    semctl(semid, 0, IPC_RMID);
}

void readyHandler(int sig)
{
    if (sig == SIGUSR1)
    {
        ready++;
    }
}

pid_t policeOfficer(int pipe_id_rec, int pipe_id_send)
{
    pid_t process = fork();
    if (process == -1)
    {
        exit(-1);
    }
    if (process > 0)
    {
        return process;
    }

    kill(mainProcessValue, SIGUSR1);

    char puffer[50];
    read(pipe_id_rec, &puffer, sizeof(puffer));
    printf("Kapott Kérdés: %s\n", puffer);
    write(pipe_id_send, "Igen kötelező, ha elhagyja a lakást!", 50);

    exit(0);
}

pid_t chiefMedicalOfficer()
{
    pid_t process = fork();
    if (process == -1)
    {
        exit(-1);
    }
    if (process > 0)
    {
        return process;
    }

    kill(mainProcessValue, SIGUSR1);

    int status;
    struct message ms = {5, "A maszk viselése valóban nagyon fontos, védve a többi embert és magunkat is a boltokban és az utakon."};
    status = msgsnd(messageQueue, &ms, strlen(ms.mtext) + 1, 0);
    if (status < 0)
    {
        perror("msgsnd");
    }

    exit(0);
}

int main(int argc, char **argv)
{
    mainProcessValue = getpid();
    signal(SIGUSR1, readyHandler);

    key_t mainKey;

    mainKey = ftok(argv[0], 1);
    messageQueue = msgget(mainKey, 0600 | IPC_CREAT);
    if (messageQueue < 0)
    {
        perror("msgget");
        return -1;
    }

    int sh_mem_id;
    sh_mem_id = shmget(mainKey, sizeof(s), IPC_CREAT | S_IRUSR | S_IWUSR);
    s = shmat(sh_mem_id, NULL, 0);

    semid = semaphoreCreation(argv[0], 1);

    int io_pipes[2];
    int succ = pipe(io_pipes);
    if (succ == -1)
    {
        exit(-1);
    }

    int io_pipes1[2];
    int succ1 = pipe(io_pipes1);
    if (succ1 == -1)
    {
        exit(-1);
    }

    policeOfficer(io_pipes1[0], io_pipes[1]);
    chiefMedicalOfficer();

    while (ready < 1)
        ;
    puts("policeOfficer készen áll!");
    while (ready < 2)
        ;
    puts("chiefMedicalOfficer készen áll!");

    char puffer[50];
    write(io_pipes1[1], "Maszk viselése kötelező minden boltban?", 50);
    read(io_pipes[0], &puffer, sizeof(puffer));
    printf("A Rendőr alezredes válasza: %s\n", puffer);

    int status;
    struct message ms;
    status = msgrcv(messageQueue, &ms, 1024, 5, 0);
    if (status < 0)
    {
        perror("msgrcv");
    }
    else
    {
        printf("A kapott üzenet a tisztifőorvostól kodja: %ld, szövege: %s\n", ms.mtype, ms.mtext);
    }

    wait(NULL);
    close(io_pipes[0]);
    close(io_pipes[1]);
    close(io_pipes1[0]);
    close(io_pipes1[1]);
    status = msgctl(messageQueue, IPC_RMID, NULL);
    if (status < 0)
    {
        perror("msgctl");
    }
    return 0;
}