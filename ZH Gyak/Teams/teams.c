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
int messageQueue;
int semid;
int start = 0;
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

void startHandler(int sig)
{
    if (sig == SIGUSR1)
    {
        start++;
    }
}

pid_t student(int pipe_id_rec, int pipe_id_send)
{
    pid_t process = fork();
    if (process == -1)
        exit(-1);
    if (process > 0)
    {
        return process;
    }

    kill(mainProcessValue, SIGUSR1);

    int status;
    struct message ms = {5, "Jo napot."};
    status = msgsnd(messageQueue, &ms, strlen(ms.mtext) + 1, 0);
    if (status < 0)
    {
        perror("msgsnd");
    }

    char puffer[50];
    read(pipe_id_rec, puffer, sizeof(puffer));
    printf("Oktatotol kapott kerdes: %s\n", puffer);

    char newData[50] = "CFS";
    semaphoreOperation(semid, -1);
    strcpy(s->text, newData);
    printf("Hallgato tag beirja a választ az osztott memoriaba: %s\n", newData);
    semaphoreOperation(semid, 1);

    shmdt(s);
    exit(0);
}

pid_t teacher(int pipe_id_rec, int pipe_id_send)
{
    pid_t process = fork();
    if (process == -1)
        exit(-1);
    if (process > 0)
    {
        return process;
    }

    kill(mainProcessValue, SIGUSR1);

    int status;
    struct message ms;
    status = msgrcv(messageQueue, &ms, 1024, 5, 0);
    if (status < 0)
    {
        perror("msgrcv");
    }
    else
    {
        printf("A kapott üzenet a hallgatotol kodja: %ld, szovege:  %s \n", ms.mtype, ms.mtext);
    }

    write(pipe_id_send, "Melyik a default ütemező a Linux rendszerben?", 50);

    sleep(2);
    semaphoreOperation(semid, -1);
    printf("Hallgato valasza: %s\n", s->text);
    semaphoreOperation(semid, 1);

    shmdt(s);
    exit(0);
}

int main(int argc, char **argv)
{
    mainProcessValue = getpid();
    signal(SIGUSR1, startHandler);

    int status;
    key_t mainKey;

    mainKey = ftok(argv[0], 1);
    messageQueue = msgget(mainKey, 0600 | IPC_CREAT);
    if (messageQueue < 0)
    {
        perror("msgget");
        return 1;
    }

    int sh_mem_id;
    sh_mem_id = shmget(mainKey, sizeof(s), IPC_CREAT | S_IRUSR | S_IWUSR);
    s = shmat(sh_mem_id, NULL, 0);

    semid = semaphoreCreation(argv[0], 1);

    int io_pipes[2];
    int succ = pipe(io_pipes);
    if (succ == -1)
        exit(-1);

    int io_pipes1[2];
    int succ1 = pipe(io_pipes1);
    if (succ1 == -1)
        exit(-1);

    int io_pipes2[2];
    int succ2 = pipe(io_pipes2);
    if (succ2 == -1)
        exit(-1);

    int io_pipes3[2];
    int succ3 = pipe(io_pipes3);
    if (succ3 == -1)
        exit(-1);

    teacher(io_pipes1[0], io_pipes[1]);
    student(io_pipes2[0], io_pipes3[1]);

    while (start < 1)
        ;
    puts("Teacher - start teams meeting");
    while (start < 2)
        ;
    puts("Student - join teams meeting!");

    char puffer[50];
    read(io_pipes[0], puffer, sizeof(puffer));
    write(io_pipes2[1], puffer, sizeof(puffer));

    wait(NULL);
    wait(NULL);
    semaphoreDelete(semid);
    close(io_pipes1[0]);
    close(io_pipes1[1]);
    close(io_pipes[0]);
    close(io_pipes[1]);
    close(io_pipes2[0]);
    close(io_pipes2[1]);
    close(io_pipes3[0]);
    close(io_pipes3[1]);
    status = msgctl(messageQueue, IPC_RMID, NULL);
    if (status < 0)
    {
        perror("msgctl");
    }
    return 0;
}