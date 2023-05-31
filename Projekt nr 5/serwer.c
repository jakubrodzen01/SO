#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX 128
#define NAZWA_SERWERA_FIFO "serwer_fifo"

struct dane_do_przekazania 
{
    pid_t pid_klienta;
    char message[MAX];
};

int main() 
{
    int ilosc_wiad=3;

    if (mkfifo(NAZWA_SERWERA_FIFO, 0600) < 0) // tworze fifo (nazwa_sciezkowa, prawa_dostepu)
    {
        perror("SERWER: Blad podczas procesu tworzenia kolejki fifo serwera!\n");
        exit(EXIT_FAILURE);
    }
    printf("SERWER: Blokuje się dopoki moje FIFO nie zostanie otwarte do zapisu.\n");
    
    int server_fifo_fd = open(NAZWA_SERWERA_FIFO, O_RDONLY); // funkcja zablokuje się, dopóki inny proces nie otworzy kolejki do zapisu;
    if (server_fifo_fd < 0) ///Funkcja open() probuje otworzyc plik. Zwraca deskryptor pliku(nieujemna liczba uzywana przy czytaniu, pisaniu do pliku, itd.) lub -1 w przypadku bledu(kod bledu na zmiennej errno).
    {
        perror("SERWER: Blad otwarcia potoku serwera\n");
        exit(EXIT_FAILURE);
    }

    printf("SERWER: zasypiam na 5 sekund\n");
    sleep(5);

        int readBytes;
        struct dane_do_przekazania dane;
        char nazwaFifoKlienta[20];
        int klientFifoFd;
        // read(deskryptor, adres struktury, liczba_bajtow) zwraca liczbe wczytanych bajtow, zwroci 0 gdy fifo bedzie puste i odlaczymy wszystkich klientow
        while (readBytes = read(server_fifo_fd, &dane, sizeof(struct dane_do_przekazania)) > 0) 
        {
            printf("SERWER: Otrzymalem wiadomosc od %d o tresci %s\n",dane.pid_klienta, dane.message);

            if (readBytes == -1) 
            {
                perror("SERWER: Blad odczytu z potoku serwera\n");
                exit(EXIT_FAILURE);
            }
            //powiekszenie liter
            for (int i = 0; dane.message[i] != '\0' ; i++)
                dane.message[i] = toupper(dane.message[i]);

            sprintf(nazwaFifoKlienta, "client_%d", dane.pid_klienta);

            klientFifoFd = open(nazwaFifoKlienta, O_WRONLY); // zapis do fifo klienta||||  funkcja zablokuje się, dopóki inny proces nie otworzy tej kolejki do odczytu;
            if (klientFifoFd < 0)
            {
                perror("SERWER: Blad otwarcia potoku klienta\n");
                exit(EXIT_FAILURE);
            }

            printf("SERWER: Wysylam powrotna dla %d o tresci %s\n",dane.pid_klienta, dane.message);

            if (write(klientFifoFd, &dane, sizeof(struct dane_do_przekazania)) < 0) //zapisywanie do fifo klienta (deskryptor, wskaznik do struktury, ilosc_bajtow)
            {
                perror("SERWER: Blad zapisu do potoku klienta\n");
                exit(EXIT_FAILURE);
            }

            if(close(klientFifoFd)==-1)
            {
                perror("SERWER: Blad przy zamykaniu lacza fifo klienta\n");
                exit(EXIT_FAILURE);
            } 
            //sleep(1);
        
        }

    if(close(server_fifo_fd)==-1)
    {
        perror("SERWER: Blad przy zamykaniu lacza fifo klienta\n");
        exit(EXIT_FAILURE);
    } 

    if(unlink(NAZWA_SERWERA_FIFO) < 0) 
    {
        perror("SERWER: Blad usuniecia kolejki fifo serwera\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_FAILURE);
}
