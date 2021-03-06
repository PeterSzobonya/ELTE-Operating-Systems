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

enum message
{
    counting
};

struct Message
{
    long mtype;
    char mtext[1024];
};

struct sharedData
{
    int point;
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

void starthandler(int sig)
{
    if (sig == SIGUSR1)
    {
        ready++;
    }
}

int rand_point()
{
    srand(time(NULL));
    return rand() % 10;
}

pid_t student(int pipe_id_rec, int pipe_id_send)
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

    enum message sign;
    int counter = 1;
    while (counter < 4)
    {
        read(pipe_id_rec, &sign, sizeof(enum message));
        printf("Tanulo mondja: %i\n", counter);
        counter++;
    }

    int status;
    struct Message ms = {5, "Mit csináljak?"};
    status = msgsnd(messageQueue, &ms, strlen(ms.mtext) + 1, 0);
    if (status < 0)
    {
        perror("msgsnd");
    }
    struct Message msFromParent;
    status = msgrcv(messageQueue, &msFromParent, 1024, 5, 0);
    if (status < 0)
    {
        perror("msgrcv");
    }
    else
    {
        printf("A kapott uzenet az oktatotol: kodja: %ld, szovege: %s, pidje: %i\n", msFromParent.mtype, msFromParent.mtext, mainProcessValue);
    }

    semaphoreOperation(semid, -1);
    printf("Tanulo pontja: %i\n", s->point);
    semaphoreOperation(semid, 1);

    shmdt(s);
    exit(0);
}

int main(int argc, char **argv)
{
    mainProcessValue = getpid();
    signal(SIGUSR1, starthandler);

    int status;
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

    student(io_pipes[0], io_pipes1[1]);

    while (ready < 1)
        ;
    puts("Tanulás kezdés!");

    enum message puffer = counting;
    int mainCounter = 1;
    while (mainCounter < 4)
    {
        sleep(1);
        printf("Oktato mondja: %i\n", mainCounter);
        write(io_pipes[1], &puffer, sizeof(enum message));
        mainCounter++;
    }

    struct Message ms;
    status = msgrcv(messageQueue, &ms, 1024, 5, 0);
    if (status < 0)
    {
        perror("msgsnd");
    }
    else
    {
        printf("A kapott uzenet az tanulotol: kodja: %ld, szovege: %s\n", ms.mtype, ms.mtext);
    }
    struct Message msToChild = {5, "Rántsd meg a zsinórt!"};
    status = msgsnd(messageQueue, &msToChild, strlen(msToChild.mtext) + 1, 0);
    if (status < 0)
    {
        perror("msgsnd");
    }

    int resultPoint = rand_point();
    semaphoreOperation(semid, -1);
    s->point = resultPoint;
    semaphoreOperation(semid, 1);

    shmdt(s);
    wait(NULL);
    semaphoreDelete(semid);
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