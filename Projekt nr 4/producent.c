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
    srand(time(NULL));

    FILE * plik;
    char nazwaPliku[256];
    char znak;

    int ilosc=stringNaLong(argv[1]);

    //ustawienie nazwy pliku we_PID
    sprintf(nazwaPliku,"we_%d",getpid());

    if(!(plik=fopen(nazwaPliku,"w")))
    {
        perror("Blad otwarcia pliku producenta");
        exit(EXIT_FAILURE);
    }
    
    for(int i=0; i<ilosc; i++)
    {
        znak = 'a' + rand() % 26;

        if(write(1, &znak, sizeof(char))==-1)   //write(numer deskryptora pliku, do którego chcemy zapisać dane, adres zmiennej ktora zapisze, wielkosc zmiennej), zwraca liczbe zapisanych bajtow
        {
            perror("Blad zapisywania znakow");
            exit(EXIT_FAILURE);
        }

       if(!(fprintf(plik, "%c", znak)))
	{
	    perror("Blad zapisu znaku do pliku");
            exit(EXIT_FAILURE);	
	}
    }
    fclose(plik);
    sleep(1);
    exit(0);
}

long stringNaLong(char* string)
{
    char* wyj;
    char text[100];
    long wartosc;
    size_t ostatni = strlen(string)-1;
    //usuwanie '\n'
    if(string[ostatni] == '\n')
    {
        strncpy(text, string, ostatni);     //strncpy(strTo, strFrom, size)
        wartosc = strtol(text, &wyj, 10);   //strtol(lancuch znakow do przekonwertowania, argument wyjściowy do którego zostanie zapisany adres pierwszego znaku, którego nie udało się przekonwertować, system liczbowy )
    }
    else
    {
        wartosc = strtol(string, &wyj, 10);
    }
    if(errno == ERANGE)
    {
        perror("Przekroczono zakres long\n");
        exit(EXIT_FAILURE);
    }
    if(*wyj)
    {
        perror("Blad konwersji na long\n");
        exit(EXIT_FAILURE);
    }
    return wartosc;
}
