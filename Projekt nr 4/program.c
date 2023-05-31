#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <string.h>

long stringNaLong(char* string);

int main(int argc, char* argv[])
{
    int fd[2]; // potok
    int prod, kons, ilosc, limit;
    int uruchomioneProcesu;
    char bufor[256];
    char * end;
    FILE * sprawdz;
    prod = stringNaLong(argv[1]);
    kons = stringNaLong(argv[2]);
    ilosc = stringNaLong(argv[3]);
    
    //sprawdzenie ilosci parametrow wywolania
    if(argc != 4)
    {
        perror("Podano zla ilosc argumentow wywolania programu!!!\n");
        exit(EXIT_FAILURE);
    }

    //sprawdzenie ile procesow jest uruchomionych
    sprawdz = popen("ps -ux | wc -l", "r");
    fgets(bufor, sizeof(bufor), sprawdz);
    pclose(sprawdz);

    uruchomioneProcesu = stringNaLong(bufor);
    // -[TEXT], -[PS], -[sh], -[wc-l]
    uruchomioneProcesu -= 4;

    //dopuszczalny limit procesow
    printf("Uruchomionych procesow: %d ",uruchomioneProcesu);
    struct rlimit lim;
    if(getrlimit(RLIMIT_NPROC, &lim)==-1)
    {
        perror("Blad limitu\n");
        exit(EXIT_FAILURE);
    }
    else
        limit=lim.rlim_max;

    printf("Limit: %d\n",limit); // 257104
    
    if(kons < 1 || prod < 1 || ilosc < 1 ){
		printf("Argumenty musza byc dodanie\n!");
		exit(EXIT_FAILURE);
	}

    // Sprawdzenie, czy użytkownik może uruchomić odpowiednią liczbę procesów
    if(prod + kons + uruchomioneProcesu > limit)
    {
        perror("Nie mozna utworzyc tylu procesow, bo Limit zostanie przekroczony\n");
        exit(EXIT_FAILURE);
    }

    //tworzenie potoku
    if(pipe(fd) == -1)
    {
        perror("Blad tworzenia potoku\n");
        exit(EXIT_FAILURE);
    }

    //tworzenie procesow producent
    for(int i=0; i<prod; i++)
    {
        switch(fork())
        {
		case -1:
                    if(errno == EAGAIN)
		    {
                        perror("Pamiec chwilowo niedostepna producent");
			exit(EXIT_FAILURE);
                    }
                    perror("fork error producent");
                    exit(EXIT_FAILURE);
		case 0:
			close(fd[0]); //zamykam odczyt
			if(dup2(fd[1],1)==-1) //duplikuje 1 do fd[1]
                    {
                        perror("dup error producent\n");
                        exit(EXIT_FAILURE);
                    }
                    close(fd[1]);
			if(!execlp("./producent","producent",argv[3],NULL))
			{
                        perror("execlp error producent");
		        exit(EXIT_FAILURE);
                    }
				default:
                    break;
			}
    }    
    //tworzenie procesow konsument
    for(int i=0; i<kons; i++)
    {
        switch(fork())
        {
		case -1:
                    if(errno == EAGAIN)
		    {
                        perror("Pamiec chwilowo niedostepna konsument");
                        exit(EXIT_FAILURE);
                    }
                    perror("fork error konsument");
                    exit(EXIT_FAILURE);
		case 0:
		    close(fd[1]); // zamykanie deskryptora do zapisu
	            if(dup2(fd[0],0)==-1)
                    {
                        perror("dup error konsument\n");
                        exit(EXIT_FAILURE);
                    }
                    close(fd[0]);
		    if(!execlp("./konsument","konsument",NULL))
                    {
					    perror("execlp error konsument");
					    exit(EXIT_FAILURE);
                    }
		default:
                    break;
			}
    }   

    if(close(fd[0])==-1)
    {
        perror("Blad zamykania deskryptora do odczytu\n");
        exit(EXIT_FAILURE);
    }
    if(close(fd[1])==-1)
    {
        perror("Blad zamykania deskryptora do zapisu\n");
        exit(EXIT_FAILURE);
    }

    //czekanie na zakonczenie wszystkich producentow i konsumentow
    for(int i=0 ; i<prod+kons; i++)
	    wait(NULL);

    exit(0);
}

// producent fd[1] --------------------------> konsument fd[0]
long stringNaLong(char* string)
{
    char* wyj;
    char str[100];
    long wartosc;
    size_t ostatni = strlen(string)-1;
    if(string[ostatni]== '\n')
    {
        strncpy(str, string, ostatni);      //strncpy(strTo, strFrom, size)
        wartosc = strtol(str, &wyj, 10);    //strtol(lancuch znakow do przekonwertowania, argument wyjściowy do którego zostanie zapisany adres pierwszego znaku, którego nie udało się przekonwertować, system liczbowy )
    }
    else
    {
        wartosc = strtol(string, &wyj, 10);
    }
    if(errno == ERANGE)
    {
        perror("Wartosc przekracza zakres longa\n");
        exit(EXIT_FAILURE);
    }
    if(*wyj)
    {
        perror("Nie udalo sie skonwertowac ciagu znakow do long\n");
        exit(EXIT_FAILURE);
    }
    return wartosc;
}
