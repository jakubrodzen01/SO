#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#define MAX 128
#define NAZWA_SERWERA_FIFO "serwer_fifo"

struct dane_do_przekazania {
 pid_t pid_klienta;
 char message[MAX];
 };

int main(int argc, char *argv[]) 
{
    struct dane_do_przekazania dane;
    int ilosc_wiad=3; //limit wiadomosci ktore moze wyslac jeden klient.
    int client_fifo_fd;
    
    int server_fifo_fd = open(NAZWA_SERWERA_FIFO, O_WRONLY); // serwer sie odblokowal i poszedl spac
    if (server_fifo_fd < 0) 
    {
        perror("Blad otwarcia potoku serwera");
        exit(EXIT_FAILURE);
    }

    char client_fifo_name[20];
    sprintf(client_fifo_name, "client_%d", getpid());
    if (mkfifo(client_fifo_name, 0600) == -1) // tworzenie kolejki fifo klienta
    {
        perror("KLIENT: Blad tworzenia potoku klienta");
        exit(EXIT_FAILURE);
    }
    dane.pid_klienta = getpid();
    
       
        
    for(int i = 0; i < ilosc_wiad; i++)
    { 
        int  iterator=0;
        char znak;
        char bufor[MAX];
        printf("Podaj komunikat: ");

        while((znak = fgetc(stdin)) != '\n')
            {
                if((MAX-1)>iterator)
                {
                    bufor[iterator] = znak;
                }
                iterator++;
            }
            if(iterator >= (MAX-1))
            {
                bufor[MAX - 1] = '\0';
            }
            else
            {
                bufor[iterator]= '\0';
            }

            strcpy(dane.message, bufor);

            if(MAX + sizeof(int) > PIPE_BUF) // pipebuf- maksymalna ilosc bajtow w fifo
            {
                perror("KLIENT: Wielkosc struktury przekroczyla PIPE_BUF.");
                close(server_fifo_fd);
                exit(EXIT_FAILURE);
            }

            if (write(server_fifo_fd, &dane, sizeof(struct dane_do_przekazania)) < 0) 
                {
                    perror("KLIENT: Blad zapisu do potoku serwera");
                    exit(EXIT_FAILURE);
                }
            else
                printf("KLIENT: Wysylam wiadomosc do Serwera o tresci %s\n", dane.message);

        if(i==0) // pierwsza iteracja tylko wtedy otweiram 
        {
            client_fifo_fd = open(client_fifo_name, O_RDONLY);
            if (client_fifo_fd < 0) 
            {
                perror("KLIENT: Blad otwarcia potoku klienta");
                exit(EXIT_FAILURE);
            }
        }
        
        
            int czytaj;

            while (czytaj = read(client_fifo_fd, &dane, sizeof(struct dane_do_przekazania)) == 0)
                usleep(1);      //wait(NULL) będzie blokowal dzialanie programu az do zakonczenia jakiegos procesu, co w tym przypadku moze być traktowane jak czekanie na dostepnosc danych.

            if (czytaj == -1)
            {
                perror("KLIENT: Blad odczytu wiadomosci z kolejki klienta\n");
                exit(EXIT_FAILURE);
            }
            else
                printf("KLIENT: Odebralem wiadomosc dla %d o tresci %s\n", dane.pid_klienta, dane.message);
            
            
        }
        close(server_fifo_fd);
        close(client_fifo_fd);
    

        if(unlink(client_fifo_name) < 0)
        {
            perror("SERWER: Blad usuniecia kolejki fifo klienta\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("KLIENT: Kolejka fifo klienta zostala usunieta\n");
        }
        printf("KLIENT: Klient zostal zakonczony\n");
    exit(0);
}

    
    

    // for(int i = 0;i < ilosc_wiad; i++)
    // {
    // }
        

    

    