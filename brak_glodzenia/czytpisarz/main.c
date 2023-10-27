/**
*@project Projekt 2 - Czytelnicy i Pisarze
*@author Artur Leszczak (111502), Dawid Ugniewski (109522), PS2
*@version 1.1.0
*/

/*
*Definicje wbudowane
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

/*
*Funkcje własne
*/

#include "functionsA.c"
#include "functionsD.c"

int infoflag = 0;
int ilosc_pisarzy = 0;
int ilosc_czytelnikow = 0;
int pflag = 0;
int cflag = 0;

/*
*  Zmienne symbolizujace pobyt w czytelni
*/

pthread_mutex_t mutex_dane = PTHREAD_MUTEX_INITIALIZER;
volatile int ilosc_pisarzy_w_czytelni = 0;
volatile int ilosc_czytelnikow_w_czytelni = 0;

/**********/

pthread_mutex_t mutex_nextTicket = PTHREAD_MUTEX_INITIALIZER;
volatile int nextTicket = 0;

/**********/

pthread_mutex_t mutex_servingTicket = PTHREAD_MUTEX_INITIALIZER;
volatile int servingTicket = 1;
pthread_cond_t czekanieWKolejce = PTHREAD_COND_INITIALIZER;

/*
*  Zmienna symbolizujaca ile czytelnikow z rzedu wzielo bilet
*/

pthread_mutex_t mutex_wstrzymanieCzytelnikow = PTHREAD_MUTEX_INITIALIZER;
volatile int ticketyCzytelnikow = 0;
pthread_cond_t wstrzymanieCzytelnikow = PTHREAD_COND_INITIALIZER;

/*
*  Zmienne warunkowe sygnalizujace spelnienie warunkow pobytu w czytelni
*/

pthread_cond_t czekanieAzWyjdziePisarz = PTHREAD_COND_INITIALIZER;
pthread_cond_t czekanieAzWyjdaWszyscy = PTHREAD_COND_INITIALIZER;


void *Funkcja_czytelnika();
void *Funkcja_pisarza();
void Wypisz_komunikat();
void Wypisz_stan();

long **kolejka_czytelnikow;
long **kolejka_pisarzy;


int main(int argc, char **argv){

	srand(time(NULL));
	
	/*
	*Sprawdzenie przekazanych argumentów i ich wartości
	*/
	int opcja;
	
	while((opcja = getopt(argc,argv,"P:C:i:")) != -1){
		switch(opcja){
		case 'i': /*Parametr info wlaczony*/
		      if(strcmp(optarg, "nfo") == 0){
		      	infoflag = 1;
		      }else{
		      	fprintf(stderr, "Nieznany parametr: %c%s\n",opcja,optarg);
		      	exit(EXIT_FAILURE);
		      }
		      break;
		case 'P':
			pflag = 1;
			ilosc_pisarzy = atoi(optarg);
			if(ilosc_pisarzy<0){
				fprintf(stderr,"Opcja -%c wymaga podania wartości dodatniej lub równej zero!\n", opcja);
				exit(EXIT_FAILURE);
				return 0;
			}
			break;
		case 'C':
			cflag = 1;
			ilosc_czytelnikow = atoi(optarg);
			if(ilosc_czytelnikow<0){
				fprintf(stderr,"Opcja -%c wymaga podania wartości dodatniej lub równej zero!\n", opcja);
				exit(EXIT_FAILURE);
				return 0;
			}
			break;
		    case '?':
		    	if(optopt == 'P'){
		        fprintf(stderr,"Opcja -%c wymaga podania wartości liczbowej dodatniej lub równej zeru!\n", optopt);
		        }else if(optopt == 'C'){
			  fprintf(stderr,"Opcja -%c wymaga podania wartości liczbowej dodatniej lub równej zeru!\n", optopt);
			}else{
			  fprintf(stderr, "Nieznana opcja! '\\x%x'.\n", optopt);
			}
			exit(EXIT_FAILURE);
		     	return 0;
		    default:
			abort();
		}
	}
	
	if(pflag){
		printf("Podano parametr pisarze (P) i wynosi %d\n", ilosc_pisarzy);
	}
	if(cflag){
		printf("Podano parametr czytelnicy (C) i wynosi %d\n", ilosc_czytelnikow);
	}
	if(infoflag){
		printf("Podano parametr info!\n");
	}
	
	
	/*
	* Inicjalizacja watkow
	*/

	kolejka_czytelnikow = (long **)malloc(sizeof(long*)*ilosc_czytelnikow);
	kolejka_pisarzy = (long **)malloc(sizeof(long*)*ilosc_pisarzy);

	for(int i = 0; i < ilosc_czytelnikow; ++i)
	{
		kolejka_czytelnikow[i] = (long *)malloc(sizeof(long)*2);
		kolejka_czytelnikow[i][0] = -1;
		kolejka_czytelnikow[i][1] = -1;
	}

	for(int i = 0; i < ilosc_pisarzy; ++i)
	{
		kolejka_pisarzy[i] = (long *)malloc(sizeof(long)*2);
		kolejka_pisarzy[i][0] = -1;
		kolejka_pisarzy[i][1] = -1;
	}
	
	pthread_t watkiCzytelnicy[ilosc_czytelnikow];
	pthread_t watkiPisarze[ilosc_pisarzy];

	int rt;

	long* liczbaWatkowCzytelnikow = malloc(sizeof(long)*ilosc_czytelnikow);
	long* liczbaWatkowPisarzy = malloc(sizeof(long)*ilosc_pisarzy);

	int i;

	for(i = 0; i < ilosc_czytelnikow; ++i)
	{	
		liczbaWatkowCzytelnikow[i] = i;
		rt = pthread_create(&watkiCzytelnicy[i], NULL, &Funkcja_czytelnika, &liczbaWatkowCzytelnikow[i]); //tworzenie wątku czytelnika
		/*W przypadku niepowodzenia tworzenia wątku czytelnika*/
		if( rt != 0 )
		{
			fprintf(stderr, "Blad tworzenia watku (czytelnik) nr %ld\n", &liczbaWatkowCzytelnikow[i]);
			fprintf(stderr, "%s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		
	}
	
	/*Pętla tworząca pisarzy*/
	for(i = 0; i < ilosc_pisarzy; ++i)
	{
		liczbaWatkowPisarzy[i] = i;
		rt = pthread_create(&watkiPisarze[i], NULL, &Funkcja_pisarza, &liczbaWatkowPisarzy[i]);//tworzenie wątku pisarza
		/*W przypadku niepowodzenia tworzenia wątku pisarza*/
		if( rt != 0 )
		{
			fprintf(stderr, "Blad tworzenia watku (pisarz) nr %ld\n", &liczbaWatkowPisarzy[i]);
			fprintf(stderr, "%s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		
	}
	
	/*
	* Pętla kończąca wątki;
	*/

	for(i = 0; i < ilosc_czytelnikow ; ++i)
	{
		pthread_join(watkiCzytelnicy[i], NULL);
	}
	for(i = 0; i < ilosc_pisarzy ; ++i)
	{
		pthread_join(watkiPisarze[i], NULL);
	}


	exit(0);

}

void *Funkcja_czytelnika(void *arg)
{
	int ticket = -1;

	int numer = *(int *) arg;
	pthread_mutex_lock(&mutex_dane);
	kolejka_czytelnikow[numer][0] = numer;
	kolejka_czytelnikow[numer][1] = 0;
	pthread_mutex_unlock(&mutex_dane);

	
	while(1)
	{
		//sprawdzenie czy za duzo czytelnikow z rzedu nie pobralo

		pthread_mutex_lock( &mutex_wstrzymanieCzytelnikow );
		++ticketyCzytelnikow; //zaktualizuj liczbe czytelnikow z rzedu
		while ( ticketyCzytelnikow >= 10)
		{
			pthread_cond_wait( &wstrzymanieCzytelnikow, &mutex_wstrzymanieCzytelnikow );
		}
		pthread_mutex_unlock( &mutex_wstrzymanieCzytelnikow );

		// pobieranie biletu
		pthread_mutex_lock(&mutex_nextTicket);
		ticket = ++nextTicket;
		pthread_mutex_unlock(&mutex_nextTicket);

		// weryfikacja biletu
		
		pthread_mutex_lock(&mutex_servingTicket);
		while( ticket != servingTicket)
		{
			pthread_cond_wait(&czekanieWKolejce, &mutex_servingTicket);
		}
		//tu wpuszcza po kolei (FIFO)
		++servingTicket;
		pthread_mutex_unlock(&mutex_servingTicket);
		
		// wpuszczono watek do drzwi
		// sprawdz czy nie ma pisarza, jesli jest => czekaj
		
		pthread_mutex_lock(&mutex_dane);
		
		while( ilosc_pisarzy_w_czytelni > 0 )
		{
			pthread_cond_wait( &czekanieAzWyjdziePisarz, &mutex_dane );
		}
		
		// jesli nie ma, wpusc czytelnika
		// i wolaj nastepnego do bramki

		++ilosc_czytelnikow_w_czytelni;
		kolejka_czytelnikow[numer][1] = 1;
		if(infoflag == 0)
			Wypisz_komunikat();
		if(infoflag == 1)
			Wypisz_stan();

		pthread_mutex_unlock( &mutex_dane);
		
		pthread_cond_broadcast(&czekanieWKolejce); //wolaj natepnego do bramki
		
		sleep(LosujKilkaSekund()); //czytelnik przebywa w czytelni
		
		pthread_mutex_lock( &mutex_dane);
		--ilosc_czytelnikow_w_czytelni;
		kolejka_czytelnikow[numer][1] = 0;
		if(infoflag == 0)
			Wypisz_komunikat();
		if(infoflag == 1)
			Wypisz_stan();
		
		pthread_mutex_unlock(&mutex_dane); //czytelnik wyszedl

		//sprawdz, czy piarz moze juz wejsc
		pthread_cond_broadcast(&czekanieWKolejce); //wolaj natepnego do bramki
		pthread_cond_signal( &czekanieAzWyjdaWszyscy );

		sleep(LosujKilkaSekund());
	}
}

void *Funkcja_pisarza(void *arg)
{
	int ticket = -1;

	int numer = *(int *) arg;
	pthread_mutex_lock(&mutex_dane);
	kolejka_pisarzy[numer][0] = numer;
	kolejka_pisarzy[numer][1] = 0;
	pthread_mutex_unlock(&mutex_dane);

	while(1)
	{
		// pobieranie biletu

		pthread_mutex_lock(&mutex_nextTicket);
		ticket = ++nextTicket;
		pthread_mutex_unlock(&mutex_nextTicket);

		//zresetuj liczbe czytelnikow z rzedu

		pthread_mutex_lock( &mutex_wstrzymanieCzytelnikow );
		ticketyCzytelnikow = 0;
		pthread_mutex_unlock( &mutex_wstrzymanieCzytelnikow );

		pthread_cond_broadcast( &wstrzymanieCzytelnikow );

		// weryfikacja biletu
		
		pthread_mutex_lock(&mutex_servingTicket);
		while( ticket != servingTicket)
		{
			pthread_cond_wait(&czekanieWKolejce, &mutex_servingTicket);
		}
		//tu wpuszcza po kolei (FIFO)
		++servingTicket;
		pthread_mutex_unlock(&mutex_servingTicket);

		// wpuszczono watek do drzwi
		// zaktualizuj numer kolejno obslugiwanego

		// sprawdz czy pusta czytelnia, jesli nie => czekaj

		pthread_mutex_lock( &mutex_dane );
		
		while( ilosc_czytelnikow_w_czytelni > 0 || ilosc_pisarzy_w_czytelni > 0)
		{
			pthread_cond_wait( &czekanieAzWyjdaWszyscy, &mutex_dane );
		}
		

		//jesli pusto => wpusc pisarza
		// i wolaj nastepnego do bramki
		
		++ilosc_pisarzy_w_czytelni;
		kolejka_pisarzy[numer][1] = 1;
		if(infoflag == 0)
			Wypisz_komunikat();
		if(infoflag == 1)
			Wypisz_stan();

		pthread_mutex_unlock( &mutex_dane );

		pthread_cond_broadcast(&czekanieWKolejce); //wolaj natepnego do bramki

		sleep(LosujKilkaSekund()); // Pisarz przebywa w czytelni

		pthread_mutex_lock( &mutex_dane );
		--ilosc_pisarzy_w_czytelni;
		kolejka_pisarzy[numer][1] = 0;
		if(infoflag == 0)
			Wypisz_komunikat();
		if(infoflag == 1)
			Wypisz_stan();
		pthread_mutex_unlock( &mutex_dane ); //Pisarz wyszedl

		//sprawdz, czy ktos moze juz wejsc
		pthread_cond_broadcast(&czekanieWKolejce); //wolaj natepnego do bramki
		pthread_cond_broadcast( &czekanieAzWyjdaWszyscy );
		pthread_cond_broadcast( &czekanieAzWyjdziePisarz );


		sleep(LosujKilkaSekund());
	}
}

void Wypisz_komunikat()
{
	fprintf(stderr, "Kczytelnikow: %d Kpisarzy: %d [C:%d P:%d]\n", 
	ilosc_czytelnikow - ilosc_czytelnikow_w_czytelni,
	ilosc_pisarzy - ilosc_pisarzy_w_czytelni,
	ilosc_czytelnikow_w_czytelni, ilosc_pisarzy_w_czytelni);
}

void Wypisz_stan()
{
	fprintf(stderr, "Kolejka czytelnikow:\n");
	for(int i = 0; i < ilosc_czytelnikow; ++i)
	{
		// Wypisz czekajacych czytelnikow
	
		if(kolejka_czytelnikow[i][1] == 0)
		{
			fprintf(stderr, "Czytelnik %ld\n", kolejka_czytelnikow[i][0]+1);
		}

	}
	fprintf(stderr, "----------------\n");
	fprintf(stderr, "Kolejka pisarzy\n");
	for(int i = 0; i < ilosc_pisarzy; ++i)
	{
		// Wypisz czekajacych pisarzy
		

		if(kolejka_pisarzy[i][1] == 0)
		{
			fprintf(stderr, "Pisarz %ld\n", kolejka_pisarzy[i][0]+1);
		}
		

	}
	fprintf(stderr, "----------------\n");
	fprintf(stderr, "Czytelnicy w czytelni:\n");
	for(int i = 0; i < ilosc_czytelnikow; ++i)
	{
		// Wypisz czytelnikow w czytelni
		

		if(kolejka_czytelnikow[i][1] == 1)
		{
			fprintf(stderr, "Czytelnik %ld\n", kolejka_czytelnikow[i][0]+1);
		}
		

	}
	fprintf(stderr, "----------------\n");
	fprintf(stderr, "Pisarze w czytelni:\n");
	for(int i = 0; i < ilosc_pisarzy; ++i)
	{
		// Wypisz czytelnikow w czytelni
		

		if(kolejka_pisarzy[i][1] == 1)
		{
			fprintf(stderr, "Pisarz %ld\n", kolejka_pisarzy[i][0]+1);
		}
		

	}
	fprintf(stderr, "----------------\n");
}
