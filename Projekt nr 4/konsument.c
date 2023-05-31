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

int main(int argc, char* argv[])
{
    FILE * plik;

    char nazwa_pliku[256];
    int odczyt;
    char znak;

    //ustawienie nazwy pliku we_PID
    sprintf(nazwa_pliku,"wy_%d",getpid());

    if(!(plik=fopen(nazwa_pliku,"w")))
    {
        perror("Blad otwarcia pliku producenta");
        exit(EXIT_FAILURE);
    }

    while((odczyt=read(0, &znak, sizeof(char)))>0)      //read(jest numer deskryptora pliku, z którego chcemy odczytać dane, adres zmiennej do ktorej odczyta, wielkosc zmiennej odczytanej)
    {
        if(odczyt==-1)
        {
            perror("Blad wczytywania znakow");
            exit(EXIT_FAILURE);
        }
        if(!(fprintf(plik, "%c", znak)))
        {
            perror("Blad zapisu znaku do pliku");
            exit(EXIT_FAILURE);
        }
    }
    
    fclose(plik);
    exit(0);
}
