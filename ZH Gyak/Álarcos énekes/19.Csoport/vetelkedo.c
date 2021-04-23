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

struct Message
{
    long mtype;
    char mguess;
};

pid_t mainProcessValue = 0;
int ready = 0;
int messageQueue;

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

char *evaluate(int playerAnswer, int goodAnswer)
{
    char *result = malloc(2 * sizeof(char));
    if (playerAnswer == goodAnswer)
    {
        printf("Valaszod: %u, Mikulast kapsz!\n", playerAnswer);
        result[0] = 'j';
        result[1] = '\0';
    }
    else
    {
        printf("Valszod: %u, Virgacsot kapsz!\n", playerAnswer);
        result[0] = 'h';
        result[1] = '\0';
    }
    return result;
}

int rand_id(int max)
{
    return rand() % max;
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

    char question[50];
    read(pipe_id_rec, &question, sizeof(question));
    sleep(1);
    srand(time(NULL));
    int answer = (rand() % 5) + 1;
    printf("Elso jatekos: Kapott kerdes: %s es erre a valaszom: %i\n", question, answer);
    write(pipe_id_send, &answer, sizeof(answer));

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

    char question[50];
    read(pipe_id_rec, &question, sizeof(question));
    sleep(2);
    srand(time(NULL));
    int answer = (rand() % 5) + 1;
    printf("Masodik jatekos: Kapott kerdes: %s es erre a valaszom: %i\n", question, answer);
    write(pipe_id_send, &answer, sizeof(answer));

    sleep(1);
    char puffer;
    puffer = rand_id(100) < 50 ? 'h' : 'j';
    int status;
    struct Message ms = {5, puffer};
    status = msgsnd(messageQueue, &ms, sizeof(char), 0);
    if (status < 0)
    {
        perror("msgsnd");
    }

    exit(0);
}

int main(int argc, char **argv)
{
    mainProcessValue = getpid();
    signal(Alarcot_FEL, startHandler);

    int status;
    key_t mainKey;

    mainKey = ftok(argv[0], 1);
    messageQueue = msgget(mainKey, 0600 | IPC_CREAT);
    if (messageQueue < 0)
    {
        perror("msgget");
        return -1;
    }

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

    write(io_pipes[1], questions[randomQuestion]->question, strlen(questions[randomQuestion]->question));
    write(io_pipes2[1], questions[randomQuestion]->question, strlen(questions[randomQuestion]->question));

    int firstAnswer;
    int secondAnswer;
    char *firstResult;
    char *secResult;
    read(io_pipes1[0], &firstAnswer, sizeof(int));
    printf("Elso jatekos valsza: %i\n", firstAnswer);
    firstResult = evaluate(firstAnswer, questions[randomQuestion]->answer);
    read(io_pipes3[0], &secondAnswer, sizeof(int));
    printf("Masodik jatekos valsza: %i\n", secondAnswer);
    secResult = evaluate(secondAnswer, questions[randomQuestion]->answer);

    printf("Elso jatekos valasza ismetelen a kovetkezo: (jo / hamis) -> %s\n", firstResult);
    evaluate(firstAnswer, questions[randomQuestion]->answer);
    char *guessValue = malloc(2 * sizeof(char));
    struct Message ms;
    status = msgrcv(messageQueue, &ms, sizeof(char), 5, 0);
    if (status < 0)
    {
        perror("msgrcv");
    }
    else
    {
        guessValue[0] = ms.mguess;
        guessValue[1] = '\0';
        printf("A kapott tipp a MASODIK jatekostol az eslore: %c\n", ms.mguess);
    }

    if (strcmp(firstResult, guessValue) == 0)
    {
        printf("A masodik jatekos sikeres valaszt adott! Jo valasz, mikulast kap!\n");
    }
    else
    {
        printf("A masodik jatekos nem jo valaszt adott! Rossz valasz, virgacsot kap!\n");
    }

    wait(NULL);
    close(io_pipes[0]);
    close(io_pipes[1]);
    close(io_pipes1[0]);
    close(io_pipes1[1]);
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