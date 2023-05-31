#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/shm.h>

#define SP 0                    //dla zapewnienia wykluczenia wzajemnego procesu piszącego względem wszystkich innych procesów, inicjowany wartością 1
#define W 1                     //dla zapewnienia wykluczenia wzajemnego procesowi czytającemu w chwilach rozpoczynania i kończenia korzystania z zasobu, inicjowany wartością 1
void makeMem(void);
void conectMem(void);
void closeMem(void);
void connectSem(void);
void semafor_p(int);
void semafor_v(int);
void handler(int);
int memId;
key_t memoryKey;
int semaforId;
key_t semKey;

long int *mem;

int main(int argc, char* argv[])
{
    signal(SIGINT, *handler);
    connectSem();
    makeMem();
    conectMem();
    int pid = getpid();
    srand(time(NULL)+pid);
    int letter;
    while(1) 
    {
        semafor_p(SP);          //sekcja krytyczna zapisu litery przez pisarza
        printf("Pisarz wszedl do biblioteki, %i              %i\n", mem[1], pid);
        letter = (rand()%25)+65;
        mem[2] = letter;
        // sleep(1);
        //usleep(10000);
        semafor_v(SP);          //
    }
    closeMem();
    exit(0);
}

void makeMem()
{
    memoryKey = ftok(".", 0);
    if(memoryKey == -1) {
        perror("writer Blad tworzenia klucza\n");
        exit(EXIT_FAILURE);
    }
    memId = shmget(memoryKey, 2 * sizeof(long int)+1, 0600 | IPC_CREAT);            //shmget(klucz, rozmiar obszaru pamieci, flagi)
    if(memId == -1){
        perror("writer Blad tworzenia pamieci dzielonej\n");
        exit(EXIT_FAILURE);
    }

}

void conectMem()
{
    mem = shmat(memId, 0, 0);                                       //shmmat(id pamieci dzielonej, system sam znajdzie adres, zadne specjalne flagi)
    if(*mem == -1 && errno !=0) {
        perror("writer Problem z przydzieleniem pamieci\n");
        exit(EXIT_FAILURE);
    }
}

void closeMem()
{

    if(shmctl(memId, IPC_RMID, 0) || shmdt(mem)){
        perror("writer Blad odlaczenia pamieci dzielonej\n");
        exit(EXIT_FAILURE);
    }

}

void connectSem(void)
{
    semKey = ftok(".", 0);
    if(semKey == -1){
        perror("writer Semafor: Blad tworzenia klucza\n");
        exit(EXIT_FAILURE);
    }

    semaforId = semget(semKey, 2, IPC_CREAT | 0600);                        //semget(klucz, liczba semaforow, flaga)
    if(semaforId == -1){
        perror("writer Blad uzyskania dostepu do semafora\n");
        exit(EXIT_FAILURE);
    }
}

void semafor_p(int nr)
{
    int x;
    struct sembuf buforSem;
    buforSem.sem_num = nr;
    buforSem.sem_op = -1;
    buforSem.sem_flg = 0;

    x = semop(semaforId, &buforSem, 1);                             //semop(id semafora, wskaznik na strukture sembuf ktora zawiera informacje o operacjach na semaforze, liczba operacji ktore maja zostac wykonane)
    if(x == -1) {
        if(errno == 4) {                                            //EINTR oznacza, ze operacja zakonczyła się przed czasem, poniewaz proces otrzymal sygnal przerwania.
            semafor_p(nr);
        }
        else {
            perror("writer Blad zamykania semafora\n");
            exit(EXIT_FAILURE);
        }
    }
}

void semafor_v(int nr)
{
    int x;
    struct sembuf buforSem;
    buforSem.sem_num = nr;
    buforSem.sem_op = 1;
    buforSem.sem_flg = 0;

    x = semop(semaforId, &buforSem, 1);                     //semop(id semafora, wskaznik na strukture sembuf ktora zawiera informacje o operacjach na semaforze, liczba operacji ktore maja zostac wykonane)
    if(x == -1){
        if(errno == 4){                                      //EINTR oznacza, ze operacja zakonczyła się przed czasem, poniewaz proces otrzymal sygnal przerwania.
            semafor_v(nr);              
        }
        else {
            perror("writer Blad otwierania semafora\n");
            exit(EXIT_FAILURE);
        }
    }
}

void handler(int sig)
{
    closeMem();
    exit(0);
}