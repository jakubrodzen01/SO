#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX 256 //max 8184

int key, idKolejki;
int flaga = 0;

typedef struct message 
{
    long odbiorca;
    long nadawca;
    char message[MAX];
} msgData;

void przerwanie();
void powiekszanie();

void utworzKolejke() 
{
    //key = ftok(".", 'A');
    key = 19042001;
    if (key == -1) 
    {
        perror("Blad ftok\n");
        exit(EXIT_FAILURE);
    } 
    else 
    {
        printf("Klucz do kolejki wiadomosci zostal utworzony\n");
    }

    idKolejki = msgget(key, 0606 | IPC_CREAT);
    if (idKolejki == -1) 
    {
        perror("Blad msgget\n");
        exit(EXIT_FAILURE);
    } 
    else 
    {
        printf("Utworzono kolejke wiadomosci %d\n", idKolejki);
    }
}

void sygnaly() 
{                                   //Funkcja signal() zwraca wskaźnik do funkcji obsługi poprzedniego sygnału lub SIG_ERR, jeśli wystąpił błąd.
    signal(SIGINT, przerwanie);     //SIGINT = CTRL + C
    signal(SIGTERM, przerwanie);    //SIGTERM = sygnal konca, ktory jest wysylany do procesu, gdy chcemy zakonczyc dzialanie tego procesu, najczesciej przez inny proces lub system operacyjny
    //signal(SIGSTOP, przerwanie);
}

void odbierzIWyslij() 
{
    msgData message;
    int status = 0;
    while (1) 
    {
        printf("\nOczekiwanie na wiadomosc\n");

        status = msgrcv(idKolejki, &message, MAX + sizeof(long), 1, 0);     //msgrcv(idKolejki, wskaznik do bufora gdzie bedzie zapisana wiadomosc, maksymalny rozmiar odebranego komunikatu, typ odbieranego komunikatu 0 to pierwszy dostepny a liczba dodatnia to pierwszy dostepny danego typu, flaga 0 - zawieszony w oczekiwaniu na wlasciwy IPC_NOWAIT - nie czeka MSG_NOERROR - obcina jak komunikat za dlugi), zwraca liczbe pobranych bajtow
        if (status == -1) 
        {
            if (errno == E2BIG)     //E2BIG wiadomosc jest zbyt duza zeby zmiescic sie w bufforze
            {
                printf("\nWielkosc wiadomosci nie jest poprawna\n");
                flaga = 1;
                przerwanie();
            }
            perror("Blad msgrcv\n");
            flaga = 1;
            przerwanie();
        }

        printf("Odebralem wiadomosc: %s\tod %ld\n", message.message, message.nadawca);

        message.odbiorca = message.nadawca;
        message.nadawca = 1;
        powiekszanie(message.message, sizeof(message.message));

        status = msgsnd(idKolejki, &message, strlen(message.message) + sizeof(long) + 1, 0);    //msgsnd(idKolejki, wskaznik do struktury zawierajacej wiadomosc do wyslania, rozmiar wiadomosci do wyslania, flaga z reakcja na przepelnienie kolejki IPC_NOWAIT - zwroci -1 i nie wysle a 0 zawiesi proces do momentu zwolnienia miejsca), zwraca 0 lub -1

        if (status == -1) 
        {
            perror("Blad msgrcv\n");
            flaga = 1;
            przerwanie();
        }

        printf("Odsylam wiadomosc do %ld\n", message.odbiorca);
        sleep(1);
    }
}

void przerwanie() 
{
    int zamkniecie = msgctl(idKolejki, IPC_RMID, NULL);     //msgctl(idKolejki, kod operacji, wskaznik do struktury ktora jest uzywana do przechowywania informacji o kolejce wiadomosci lub do okreslenia nowych wartosci dla pol kolejki wiadomosci), zwraca 0 lub -1
    if (zamkniecie == -1) 
    {
        perror("Blad msgctl\n");
        exit(EXIT_FAILURE);
    }
    printf("\nUsunalem kolejke %d\nSerwer konczy dzialanie\n", idKolejki);

    if (flaga == 0) 
    {
        exit(0);
    }
    else 
    {
        exit(EXIT_FAILURE);
    }
}

void powiekszanie(char *text, int rozmiar) 
{
    if (rozmiar) 
    {
        for (int i = 0; i < rozmiar; i++) 
        {
            text[i] = toupper(text[i]);
        }
    }
}

int main() 
{
    utworzKolejke();
    sygnaly();
    odbierzIWyslij();
}