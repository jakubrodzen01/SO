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
#include <pthread.h>

#define MAX 256 // max 8184

int key, idKolejki;
int iloscWiadomosci = 0;
int flaga = 0;
pthread_t threadNadawcy, threadOdbiorcy;

typedef struct message 
{
    long odbiorca; // odbiorca
    long nadawca;  // nadawca
    char message[MAX];
} msgData;

void *nadawca();
void *odbiorca();
void przerwanie();

void dodlaczDoKolejki() 
{
    // key = ftok(".", 'A');
    key = 19042001;
    if (key == -1) 
    {
        perror("Blad ftok\n");
        exit(EXIT_FAILURE);
    } 
    else 
    {
        printf("Klucz do kolejki wiadomosci zostal utowrzony\n");
    }

    idKolejki = msgget(key, 0606);  //tylko dolaczy do istniejacej kolejki
    if (idKolejki == -1) 
    {
        perror("Blad msgget lub serwer nie dziala\n");
        exit(EXIT_FAILURE);
    } 
    else 
    {
        printf("Dolaczono do kolejki wiadomosci: %d\n", idKolejki);
    }
}

void sygnaly() 
{                                   //Funkcja signal() zwraca wskaźnik do funkcji obsługi poprzedniego sygnału lub SIG_ERR, jeśli wystąpił błąd.
    signal(SIGINT, przerwanie);     //SIGINT = CTRL + C
    
}

void stworzThreads() 
{
    if (pthread_create(&threadNadawcy, NULL, nadawca, NULL) == -1)      //pthread_create(wskaznik gdzie bedzie idWatku, wskaźnik do struktury pthread_attr_t zawierającej atrybuty wątku, lub NULL, jeśli mają być użyte domyślne atrybuty, wskaźnik do funkcji, która ma być wywołana przez nowy wątek, argument wątku przekazywany do funkcji wątku przy rozpoczęciu jego wykonywania) zwraca 0 lub -1
    {
        perror("Blad pthread_create\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&threadOdbiorcy, NULL, odbiorca, NULL) == -1)    //pthread_create(wskaznik gdzie bedzie idWatku, wskaźnik do struktury pthread_attr_t zawierającej atrybuty wątku, lub NULL, jeśli mają być użyte domyślne atrybuty, wskaźnik do funkcji, która ma być wywołana przez nowy wątek, argument wątku przekazywany do funkcji wątku przy rozpoczęciu jego wykonywania) zwraca 0 lub -1 
    {
        perror("Blad pthread_create\n");
        exit(EXIT_FAILURE);
    }
}

void dolaczThreads() 
{
    if (pthread_join(threadNadawcy, NULL) == -1) //pthread_join(idWatku, wskaźnik do zmiennej, do której ma być zapisana wartość zwracana przez wątek lub NULL, jeśli nie jest interesująca) zwraca 0 lub -1 i sluzy do oczekiwania na zakonczenie watku
    {
        perror("Blad pthread_join\n");  
        exit(EXIT_FAILURE);
    }

    if (pthread_join(threadOdbiorcy, NULL) == -1) //pthread_join(idWatku, wskaźnik do zmiennej, do której ma być zapisana wartość zwracana przez wątek lub NULL, jeśli nie jest interesująca) zwraca 0 lub -1 i sluzy do oczekiwania na zakonczenie watku
    {
        perror("Blad pthread_join\n");
        exit(EXIT_FAILURE);
    }
}

void *nadawca() 
{
    struct msqid_ds msg;    //struktura struktura służąca do przechowywania informacji o kolejce komunikatów. Zmienna typu struct msqid_ds może być używana do pobrania informacji o kolejce lub do ustawienia atrybutów kolejki
    msgData message;
    char znak;
    int max, i;
    max = 0;

    msgctl(idKolejki, IPC_STAT, &msg);  //pobiera informacje o kolejce i zapisuje w strukturze msgctl(idKolejki, kod operacji, wskaznik do struktury ktora jest uzywana do przechowywania informacji o kolejce wiadomosci lub do okreslenia nowych wartosci dla pol kolejki wiadomosci), zwraca 0 lub -1
    max = msg.msg_qbytes - (sizeof(message) - sizeof(long));  //oblicza maksymalny rozmiar komunikatu ktory moze być przechowywany w kolejce o podanym id

    while (1) 
    {
        msgctl(idKolejki, IPC_STAT, &msg);  //msgctl(idKolejki, kod operacji, wskaznik do struktury ktora jest uzywana do przechowywania informacji o kolejce wiadomosci lub do okreslenia nowych wartosci dla pol kolejki wiadomosci), zwraca 0 lub -1
        flaga = 0;

        printf("Ilosc wiadomosci w kolejce = %d\n", msg.msg_qnum);

        while (flaga == 0) 
        {
            i = 0;
            printf("\nPodaj wiadomosc: ");

            while ((znak = fgetc(stdin)) != '\n') 
            {
                if (i < (MAX - 1)) 
                {
                    message.message[i] = znak;
                }
                i++;
            }

            if (i < MAX) 
            {
                flaga = 1;
                message.message[i] = '\0';
            } 
            else 
            {
                printf("Przekroczono maksymalna wielkosc wiadomosci\n");
            }
            
        }
        msgctl(idKolejki, IPC_STAT, &msg);    //msgrcv(idKolejki, wskaznik do bufora gdzie bedzie zapisana wiadomosc, maksymalny rozmiar odebranego komunikatu, typ odbieranego komunikatu 0 to pierwszy dostepny a liczba dodatnia to pierwszy dostepny danego typu, flaga 0 - zawieszony w oczekiwaniu na wlasciwy IPC_NOWAIT - nie czeka MSG_NOERROR - obcina jak komunikat za dlugi), zwraca liczbe pobranych bajtow
        //printf("\n%i  %i\n", msg.__msg_cbytes, max);    //__msg_cbytes liczbę bajtów, które są aktualnie przechowywane w kolejce
        
        if (msg.__msg_cbytes + strlen(message.message) + sizeof(long) + 1 > max) 
        { 
            printf("Kolejka jest pelna, wiadomosc nie zostaje wyslana oraz nastepuje zakonczenie watku wysylajacego\n");
            przerwanie();
        }

        message.odbiorca = 1;
        message.nadawca = getpid();

        if (msgsnd(idKolejki, &message, strlen(message.message) + sizeof(long) + 1, 0) == -1) 
        { 
            if (errno == EIDRM || errno == EINVAL)  //EIDRM = identyfikator kolejki lub semafora został usunięty, EINVAL = niewłaściwa wartość 
            {
                printf("\nSerwer zakonczyl dzialanie, wiadomosc nie zostala wyslana\n");
                exit(0);
            }
            perror("Blad msgsnd\n");
            exit(EXIT_FAILURE);
        }

        iloscWiadomosci++;

        printf("Wiadomosc zostala wyslana\n");
        usleep(600);
    }
}
void *odbiorca() 
{
    msgData message;

    while (1)
    {
        if (iloscWiadomosci > 0) 
        {

            if (msgrcv(idKolejki, &message, MAX + sizeof(long), getpid(), 0) == -1)   //msgrcv(idKolejki, wskaznik do bufora gdzie bedzie zapisana wiadomosc, maksymalny rozmiar odebranego komunikatu, typ odbieranego komunikatu 0 to pierwszy dostepny a liczba dodatnia to pierwszy dostepny danego typu, flaga 0 - zawieszony w oczekiwaniu na wlasciwy IPC_NOWAIT - nie czeka MSG_NOERROR - obcina jak komunikat za dlugi), zwraca liczbe pobranych bajtow
            {
                if (errno == EIDRM || errno == EINVAL) //EIDRM = identyfikator kolejki lub semafora został usunięty, EINVAL = niewłaściwa wartość 
                {
                    printf("\nSerwer zakonczyl dzialanie\n");
                    exit(0);
                }
                perror("Blad msgrcv\n");
                exit(EXIT_FAILURE);
            }

            printf("\nOdebralem wiadomosc: %s\n", message.message);
            iloscWiadomosci--;
            printf("Pozostalo %i wiadomosci do odebrania\n", iloscWiadomosci);
            usleep(600);
        }
    }
}

void przerwanie() 
{
    if (iloscWiadomosci == 0) 
    {
        exit(0);
    }
    else 
    {
        printf("\nNie odczytalem jeszcze wszystkich wyslanych wiadomosci\n");
        if (pthread_detach(threadNadawcy) == -1) 
        {
            perror("\nBlad pthread_detach\n");
            exit(EXIT_FAILURE);
        }
        while (iloscWiadomosci != 0)
            ;
        exit(0);
    }
}


int main() 
{
    dodlaczDoKolejki();
    sygnaly();
    stworzThreads();
    dolaczThreads();
}
