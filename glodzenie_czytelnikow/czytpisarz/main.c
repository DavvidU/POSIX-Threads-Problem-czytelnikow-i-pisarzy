/**
*@project Projekt 2 - Czytelnicy i Pisarze
*@author Artur Leszczak (111502), Dawid Ugniewski (109522), PS2
*@version 1.1.0 - Wariant - Głodzenie czytelników
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
#include <time.h>
/*
*Funkcje własne
*/

int ilosc_pisarzy = 0;
int ilosc_czytelnikow = 0;
volatile int ilosc_pisarzy_w_czytelni = 0;
volatile int ilosc_czytelnikow_w_czytelni = 0;
volatile int ilosc_pisarzy_w_kolejce = 0;
volatile int ilosc_czytelnikow_w_kolejce = 0;

int nextTicket = 0;
int servingTicket = 1;

pthread_mutex_t mutex_dane = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_nextTicket = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex_servingTicket = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_czekanieAzWyjdziePisarz = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t czekanieWKolejce = PTHREAD_COND_INITIALIZER;

pthread_cond_t czekanieAzWyjdziePisarz = PTHREAD_COND_INITIALIZER;
pthread_cond_t czekanieAzWyjdaWszyscy = PTHREAD_COND_INITIALIZER;


void *Funkcja_czytelnika();
void *Funkcja_pisarza();
void Wypisz_komunikat();
void Wypisz_stan();
long zlicz_pisarzy_w_kolejce();
long zlicz_czytelnikow_w_kolejce();
long zlicz_pisarzy_w_czytelni();
long zlicz_czytelnikow_w_czytelni();


int wpusc_pisarz = 0;	
int wpusc_czytelnika = 1;

long **kolejka_czytelnikow;
long **kolejka_pisarzy;

int infoflag = 0;

/*
* Funkcje pomocnicze
*/


int LosujKilkaSekund()
{
    int wylosowana_liczba;
    wylosowana_liczba = ((rand() % 9) + 1); //zakres [1;9]
    return wylosowana_liczba;
}

long zlicz_pisarzy_w_kolejce(){
	//zlicz pisarzy w kolejce
	long x=0;
	for(int i=0;i<ilosc_pisarzy;i++){
		if(kolejka_pisarzy[i][1] == 0){
			x++;
		}
	}
	
	return x;
}
long zlicz_czytelnikow_w_kolejce(){
	//zlicz czytelnikow w kolejce
	long x=0;
	for(int i=0;i<ilosc_czytelnikow;i++){
		if(kolejka_czytelnikow[i][1] == 0){
			x++;
		}
	}
	
	return x;
}
long zlicz_pisarzy_w_czytelni(){
	//zlicz pisarzy w czytelni
	long x=0;
	for(long i=0;i<ilosc_pisarzy;i++){
		if(kolejka_pisarzy[i][1] == 1){
			x++;
		}
	}
	
	return x;
}
long zlicz_czytelnikow_w_czytelni(){
	//zlicz czytelnikow w czytelni
	long x=0;
	for(long i=0;i<ilosc_czytelnikow;i++){
		if(kolejka_czytelnikow[i][1] == 1){
			x++;
		}
	}
	
	return x;
}

int main(int argc, char **argv){
	/*
	* Zmienne główne main
	*/
	
	int pflag = 0;
	int cflag = 0;
	

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
		printf("Podano parametr pisarze (C) i wynosi %d\n", ilosc_czytelnikow);
	}
	if(infoflag){
		printf("Podano parametr info!\n");
	}
	
	/*
	* Inicjalizacja watkow "(lista osob)"
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
	
	int rt; //zmienna pomocnicza decydujaca o poprawności tworzenia wątku

	
	int i;
	long* liczbaWatkowCzytelnikow = malloc(sizeof(long)*ilosc_czytelnikow);
	long* liczbaWatkowPisarzy = malloc(sizeof(long)*ilosc_pisarzy);
	
	
	/*Pętla tworząca czytelników*/
	
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
	* Pętle kończące wątki;
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

void *Funkcja_czytelnika( void *arg)
{
	
	
	int numer = *(int *) arg; //otrzymuje identyfikator czytelnika
	
	pthread_mutex_lock(&mutex_dane);
	
		kolejka_czytelnikow[numer][0] = numer; //przydziela otrzymany identyfikator do tablicy zawierajacej informacje o stanie kolejki czytelnikow
	
	pthread_mutex_unlock(&mutex_dane);
	
	while(1)
	{
		kolejka_czytelnikow[numer][1] = -1;
		

		pthread_mutex_lock( &mutex_dane);
		
		//aktualizacja stanu czytelni i kolejki
		
		kolejka_czytelnikow[numer][1] = 0;
		
		/* czeka na dostępność czytelni*/
		while((zlicz_pisarzy_w_kolejce() != 0) || (zlicz_pisarzy_w_czytelni() != 0)){
			pthread_cond_wait(&czekanieAzWyjdziePisarz, &mutex_dane);
		}
		
		kolejka_czytelnikow[numer][1] = 1;
		
		/*Wypisuje komunikat o aktualnym stanie czytelni*/
		if (infoflag == 0)
			Wypisz_komunikat();
		if (infoflag == 1)
			Wypisz_stan();
		
		pthread_mutex_unlock( &mutex_dane);
		
		
		pthread_cond_broadcast(&czekanieWKolejce); //wolaj natepnego do bramki	
		
		sleep(LosujKilkaSekund()); //czytelnik przebywa w czytelni
		
		pthread_mutex_lock( &mutex_dane);
		
		/*Wypisuje komunikat o aktualnym stanie czytelni*/
		if(infoflag == 0)
			Wypisz_komunikat();
		if(infoflag == 1)
			Wypisz_stan();
		
		pthread_mutex_unlock(&mutex_dane); //czytelnik wyszedl
		/*Aktualizuje kolejke i stan czytelni*/
		
		kolejka_czytelnikow[numer][1] = -1;
		
		/*Wypisuje komunikat o aktualnym stanie czytelni i kolejkach*/
		if(infoflag == 0)
			Wypisz_komunikat();
		if(infoflag == 1)
			Wypisz_stan();
		
		//sprawdz, czy piarz moze juz wejsc
		pthread_cond_broadcast( &czekanieAzWyjdaWszyscy );
		
		/*odczekuje przed ponowieniem próby wejscia do kolejki*/
		sleep(LosujKilkaSekund());
		
	}
}

void *Funkcja_pisarza(void *arg)
{
	
	

	int numer = *(int *) arg; //stały identyfikator pisarza
	
	pthread_mutex_lock(&mutex_dane);
		
	kolejka_pisarzy[numer][0] = numer; //przydziela identyfikator do tablicy kolejki pisarzy
	
	pthread_mutex_unlock(&mutex_dane);
	
	while(1)
	{
		//ustawia stan przebywania w kolejce (-1 - nie dołączył do kolejki)
		kolejka_pisarzy[numer][1] = -1;
		
		kolejka_pisarzy[numer][1] = 0;
	

		// sprawdz czy pusta czytelnia, jesli nie => czekaj

		pthread_mutex_lock( &mutex_dane );
		
		while ((zlicz_czytelnikow_w_czytelni() > 0) || (zlicz_pisarzy_w_czytelni() > 0)) {
    			pthread_cond_wait(&czekanieAzWyjdaWszyscy, &mutex_dane);
		}	
		
		//jesli pusto => wpusc pisarza
		//aktualizuj info o kolejce iczytelni
		
	
		kolejka_pisarzy[numer][1] = 1;
		
		
		//wyswietl zmiany
		
		if(infoflag == 0)
			Wypisz_komunikat();
		if(infoflag == 1)
			Wypisz_stan();

		
		pthread_mutex_unlock( &mutex_dane );
		

		pthread_cond_broadcast(&czekanieWKolejce); //wolaj natepnego do bramki
		

		sleep(LosujKilkaSekund()); // Pisarz przebywa w czytelni
			

		pthread_mutex_lock( &mutex_dane );
		
			
		pthread_mutex_unlock( &mutex_dane ); //Pisarz wyszedl
		//aktualizuje stan czytelni i kolejki
		
		kolejka_pisarzy[numer][1] = -1;
		
		//wyswietla info
		if(infoflag == 0)
			Wypisz_komunikat();
		if(infoflag == 1)
			Wypisz_stan();
		
		//sprawdz, czy ktos moze juz wejsc
		pthread_cond_broadcast( &czekanieAzWyjdaWszyscy );
		pthread_cond_broadcast( &czekanieAzWyjdziePisarz );
		
		//usypia na moment

		sleep(LosujKilkaSekund());
	}
}

void Wypisz_komunikat()
{
	fprintf(stderr, "Kczytelnikow: %ld Kpisarzy: %ld [C:%ld P:%ld]\n", 
	zlicz_czytelnikow_w_kolejce(),
	zlicz_pisarzy_w_kolejce(),
	zlicz_czytelnikow_w_czytelni(), zlicz_pisarzy_w_czytelni());
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



