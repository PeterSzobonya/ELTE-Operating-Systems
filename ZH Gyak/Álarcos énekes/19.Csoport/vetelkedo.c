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

#define Alarcot_FEL SIGUSR1

typedef struct
{
    char *question;
    int answer;
} Question;

pid_t mainProcessValue = 0;
int ready = 0;

void startHandler(int sig)
{
    if (sig == Alarcot_FEL)
    {
        ready++;
    }
}

void generateQuestions(Question *questionArray[], const char *questionSentences[], int arraySize)
{
    for (int i = 0; i < arraySize; i++)
    {
        srand(time(NULL));
        int randomAnswer = rand() % 5 + 1;
        questionArray[i] = malloc(sizeof(Question));
        questionArray[i]->question = strdup(questionSentences[i]);
        questionArray[i]->answer = randomAnswer;
    }
}

pid_t firstPlayer(int pipe_id_rec, int pipe_id_send)
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

    kill(mainProcessValue, Alarcot_FEL);

    char randomNames[][10] = {"Tigris", "Oroszlán", "Leopárd"};

    srand(time(NULL));
    int r = rand() % 3;
    write(pipe_id_send, &randomNames[r], 10);

    exit(0);
}

pid_t secPlayer(int pipe_id_rec, int pipe_id_send)
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

    kill(mainProcessValue, Alarcot_FEL);

    char randomNames[][10] = {"Malac", "Szamár", "Zebra"};

    srand(time(NULL));
    int r = rand() % 3;
    write(pipe_id_send, &randomNames[r], 10);

    exit(0);
}

int main(int argc, char **argv)
{
    mainProcessValue = getpid();
    signal(Alarcot_FEL, startHandler);

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

    int io_pipes2[2];
    int succ2 = pipe(io_pipes2);
    if (succ2 == -1)
    {
        exit(-1);
    }

    int io_pipes3[2];
    int succ3 = pipe(io_pipes3);
    if (succ3 == -1)
    {
        exit(-1);
    }

    firstPlayer(io_pipes[0], io_pipes1[1]);
    secPlayer(io_pipes2[0], io_pipes3[1]);

    while (ready < 1)
        ;
    puts("Alarc fenn! - 1");
    while (ready < 2)
        ;
    puts("Alarc fenn! - 2");

    char firstPlayerName[10];
    read(io_pipes1[0], &firstPlayerName, sizeof(firstPlayerName));
    printf("Jatekvezető hangosan mondja: %s\n", firstPlayerName);

    char secPlayerName[10];
    read(io_pipes3[0], &secPlayerName, sizeof(secPlayerName));
    printf("Jatekvezető hangosan mondja: %s\n", secPlayerName);

    const char *questionSentences[] = {
        "Question sentece 1",
        "Question sentece 2",
        "Question sentece 3",
    };

    int N = 3;
    Question *questions[N];
    generateQuestions(questions, questionSentences, N);

    srand(time(NULL));
    int randomQuestion = rand() % 3;
    printf("Kerdes adatok: %s, %i, %i\n", questions[randomQuestion]->question, questions[randomQuestion]->answer, strlen(questions[randomQuestion]->question));

    wait(NULL);
    close(io_pipes[0]);
    close(io_pipes[1]);
    close(io_pipes1[0]);
    close(io_pipes1[1]);
    close(io_pipes2[0]);
    close(io_pipes2[1]);
    close(io_pipes3[0]);
    close(io_pipes3[1]);
    return 0;
}