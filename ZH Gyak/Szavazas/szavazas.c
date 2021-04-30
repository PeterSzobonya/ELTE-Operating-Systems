#include "sys/types.h"
#include "unistd.h"
#include "stdlib.h"
#include "signal.h"
#include "stdio.h"
#include "string.h"
#include "time.h"

pid_t mainszalertek = 0;

int ellenorzo_be[2];
int ellenorzo_ki[2];
int pecsetelo_ki[2];

void sigint_handler(int sig)
{
	/*if(szalertek != 0)
	{
		kill(szalertek,SIGKILL);
		puts(" Thread halted. ");
	}*/
	exit(-1);
}

void skip(int s) {}

int rand_id(int max)
{
	return rand() % max;
}

pid_t ellenorzo_tag(int pipe_id_rec, int pipe_id_send)
{
	pid_t szal = fork();
	if (szal == -1)
		exit(-1);
	if (szal > 0)
	{
		return szal;
	}
	signal(SIGUSR1, skip);

	kill(mainszalertek, SIGUSR1);

	int szavazoszam, szavazo_idje;
	read(pipe_id_rec, &szavazoszam, sizeof(int));
	int *idk = (int *)calloc(sizeof(int), szavazoszam);
	int i = 0;
	while (i < szavazoszam)
	{
		read(pipe_id_rec, &szavazo_idje, sizeof(int));
		idk[i] = szavazo_idje;
		printf("Ellenorzo Tag: A(z) %i. szavazó ID-je %i.\n", i, szavazo_idje);
		i++;
	}
	puts("Ellenorzo Tag: Minden szavazó adatait megkaptam.");

	i = 0;
	char puffer;
	while (i < szavazoszam)
	{
		write(ellenorzo_ki[1], idk + i, sizeof(int));
		puffer = rand_id(100) < 20 ? 'h' : 'j';
		write(ellenorzo_ki[1], &puffer, sizeof(char));
		i++;
	}
	i = -1;
	write(ellenorzo_ki[1], &i, sizeof(int));

	exit(0);
}

pid_t pecsetelo_tag(int pipe_id_rec, int pipe_id_send)
{
	pid_t szal = fork();
	if (szal == -1)
		exit(-1);
	if (szal > 0)
	{
		return szal;
	}
	signal(SIGUSR1, skip);

	kill(mainszalertek, SIGUSR1);

	int fogadott = 0;
	char puffer;
	int szavazhat = 0;
	int nem_szavazhat = 0;
	while (fogadott != -1)
	{
		read(pipe_id_rec, &fogadott, sizeof(int));
		if (fogadott == -1)
			break;
		read(pipe_id_rec, &puffer, sizeof(char));
		if (puffer == 'h')
		{
			printf("Pecsetelo Tag: Hibás ID-t kaptam: %i.\n", fogadott);
			nem_szavazhat++;
		}
		else if (puffer == 'j')
		{
			printf("Pecsetelo Tag: Jó ID-t kaptam: %i.\n", fogadott);
			szavazhat++;
		}
	}
	puts("Pecsetelo Tag: Az összes ID-t megkaptam.");

	write(pecsetelo_ki[1], &szavazhat, sizeof(int));
	write(pecsetelo_ki[1], &nem_szavazhat, sizeof(int));
	printf("Pecsetelo Tag: Szavazhat %i\n", szavazhat);
	printf("Pecsetelo Tag: Nem Szavazhat %i\n", nem_szavazhat);

	srand(time(NULL));
	int szavazasi_folyamat = 0;
	while (szavazasi_folyamat < szavazhat)
	{
		int nRandonNumber = rand() % ((4 + 1) - 1) + 1;
		printf("Pecsetelo Tag Szavazas: Szavazat lett leadva a %d szamu partra!\n", nRandonNumber);
		write(pecsetelo_ki[1], &nRandonNumber, sizeof(int));
		szavazasi_folyamat++;
	}

	exit(0);
}

int jelek = 0;

void varakozas(int sig)
{
	if (sig == SIGUSR1)
		jelek++;
}

int main(int argc, char **argv)
{
	signal(SIGINT, sigint_handler); // Interrupt the process
	signal(SIGUSR1, varakozas);
	mainszalertek = getpid();

	if (argc < 2)
	{
		puts("Hiányzó argumentum!");
		exit(-1);
	}

	int szavazok = atoi(argv[1]);
	int *idk = (int *)calloc(sizeof(int), szavazok);
	//calloc() allocates the memory and also initializes the allocated memory block to zero. If we try to access the content of these blocks then we’ll get 0.

	printf("Elnok: A szavazók száma %i.\n", szavazok);

	int i = 0;
	srand(time(NULL));
	while (i < szavazok)
	{
		idk[i] = rand_id(1000);
		i++;
	}

	int io_pipes_in[2];
	int succ = pipe(io_pipes_in);
	if (succ == -1)
		exit(-1);

	int io_pipes_out[2];
	int succ1 = pipe(io_pipes_out);
	if (succ1 == -1)
		exit(-1);

	int succ2 = pipe(ellenorzo_be);
	if (succ2 == -1)
		exit(-1);

	int succ3 = pipe(ellenorzo_ki);
	if (succ3 == -1)
		exit(-1);

	int succ4 = pipe(pecsetelo_ki);
	if (succ4 == -1)
		exit(-1);

	ellenorzo_tag(io_pipes_out[0], io_pipes_in[1]);
	pecsetelo_tag(ellenorzo_ki[0], ellenorzo_be[1]);

	while (jelek < 1)
		;
	puts("Az első biztonsági tag készen áll.");
	while (jelek < 2)
		;
	puts("A második biztonsági tag készen áll.");

	write(io_pipes_out[1], &szavazok, sizeof(int));
	i = 0;
	while (i < szavazok)
	{
		write(io_pipes_out[1], idk + i, sizeof(int));
		i++;
	}

	int szavazhat_elnok = 0;
	int nem_szavazhat_elnok = 0;
	read(pecsetelo_ki[0], &szavazhat_elnok, sizeof(int));
	read(pecsetelo_ki[0], &nem_szavazhat_elnok, sizeof(int));

	// printf("Elnok: Szavazhat %i\n", szavazhat_elnok);
	// printf("Elnok: Nem Szavazhat %i\n", nem_szavazhat_elnok);
	double szavahat_szazalek = (double)szavazhat_elnok / (double)szavazok * 100;
	double nem_szavahat_szazalek = (double)nem_szavazhat_elnok / (double)szavazok * 100;

	FILE *outfile;
	outfile = fopen("osszesites.txt", "a");

	if (outfile == NULL)
	{
		fprintf(stderr, "\nHiba fajl megnyitaskor\n");
		exit(1);
	}
	fprintf(outfile, "Elnok: Szavazatot leadhat a szavazni eljovok %f szazaleka.\n", szavahat_szazalek);
	fprintf(outfile, "Elnok: Szavazatot nem adhat le a szavazni eljovok %f szazaleka.\n", nem_szavahat_szazalek);
	fclose(outfile);

	int puffer_szavazat = 0;
	int elso_part = 0;
	int masodik_part = 0;
	int harmadik_part = 0;
	int negyedik_part = 0;

	int szavazas_folyamata = 0;
	while (szavazas_folyamata < szavazhat_elnok)
	{
		read(pecsetelo_ki[0], &puffer_szavazat, sizeof(int));
		if (puffer_szavazat == 1)
		{
			elso_part++;
		}
		else if (puffer_szavazat == 2)
		{
			masodik_part++;
		}
		else if (puffer_szavazat == 3)
		{
			harmadik_part++;
		}
		else if (puffer_szavazat == 4)
		{
			negyedik_part++;
		}
		szavazas_folyamata++;
	}
	printf("Elnok: Szavazatot leadhat a szavazni eljovok %f szazaleka.\n", szavahat_szazalek);
	printf("Elnok: Szavazatot nem adhat le a szavazni eljovok %f szazaleka.\n", nem_szavahat_szazalek);
	printf("Eredmény:\n");
	printf("A elso párt szavazat szám: %i\n", elso_part);
	printf("A masodik párt szavazat szám: %i\n", masodik_part);
	printf("A harmadik párt szavazat szám: %i\n", harmadik_part);
	printf("A negyedik párt szavazat szám: %i\n", negyedik_part);

	wait(NULL);
	return 0;
}
