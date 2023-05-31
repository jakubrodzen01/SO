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

#define SIZE 256

void makeMemory(void);
void connectMemory(void);
void closeMemory(void);
void makeSemaphores(void);
void setSemaphores(int,int);
void semafor_p(int);
void semafor_v(int);
void closeSemaphores(void);
void handler(int);
void interuption();

int memId;
key_t memoryKey;
int semaforId;
key_t semKey;
long *mem;
int *pids;
long writers, readers;
int main(int argc, char* argv[]) {
    if(argc != 4){
        perror("Zla liczba argumentow\n");
        exit(EXIT_FAILURE);
    }
    char *end;
    writers = strtol(argv[1], &end, 10);        //strtol(wskaznik na ciag znakow, wskaznik na wskaznik, ktory bedzie ustawiony na pierwszy niepoprawny znak po skonwertowanej liczbie, podstawa systemu liczbowego)
    if (errno == ERANGE){
        perror("strtol ERANGE error");
        exit(EXIT_FAILURE);
    }

    if(*end != '\0'){
        perror("strtol error");
        exit(EXIT_FAILURE);
    }
    readers = strtol(argv[2], &end, 10);        //strtol(wskaznik na ciag znakow, wskaznik na wskaznik, ktory bedzie ustawiony na pierwszy niepoprawny znak po skonwertowanej liczbie, podstawa systemu liczbowego)
    if (errno == ERANGE){
        perror("strtol ERANGE error");
        exit(EXIT_FAILURE);
    }
    if(*end != '\0'){
        perror("strtol error");
        exit(EXIT_FAILURE);
    }
    long space = strtol(argv[3], &end, 10);     //strtol(wskaznik na ciag znakow, wskaznik na wskaznik, ktory bedzie ustawiony na pierwszy niepoprawny znak po skonwertowanej liczbie, podstawa systemu liczbowego)
    if (errno == ERANGE){
        perror("strtol ERANGE error");
        exit(EXIT_FAILURE);
    }
    if(*end != '\0'){
        perror("strtol error");
        exit(EXIT_FAILURE);
    }

    if(writers < 1 || readers < 1 || space < 1){
        perror("Niepoprawne argumenty wywolania\n");
        exit(EXIT_FAILURE);
    }

    FILE *pd;
    int proceses;
    pd = popen("ps ux | wc -l",  "r");      
    if(pd == NULL){
        perror("popen error");
        exit(EXIT_FAILURE);
    }
    fscanf(pd, "%d", &proceses);             //funkcja odczytuje z deskryptora pd liczbe procesow
    if (pclose(pd) == -1){
        perror("pclose error");
        exit(EXIT_FAILURE);
    }
    int limit;
    pd = popen("ulimit -p", "r");       
    if(pd == NULL){
        perror("popen error");
        exit(EXIT_FAILURE);
    }
    fscanf(pd, "%d", &limit);               //funkcja odczytuje z deskryptora pd limit procesow
     if (pclose(pd) == -1){
        perror("pclose error");
        exit(EXIT_FAILURE);
    }

    if( writers + readers + proceses - 4 > limit ){
        perror("Przekroczony limit procesow, operacja niemozliwa\n");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, *handler);
    makeMemory();
    connectMemory();
    mem[0] = space;
    mem[1] = 0;
    makeSemaphores();
    setSemaphores(0,1);
    setSemaphores(1,1);
    pids = malloc((writers+readers)* sizeof(int));
    for(long i = 0; i < writers; i++) {
        switch(pids[i] = fork()) {
            case -1: {
                perror("Main: writers fork() error\n");
                exit(EXIT_FAILURE);
            }
            case 0: {
                if(execl("./writer", "writer", NULL) == -1) {
                    perror("Main: writers execl() error\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    for(long i = 0; i < readers; i++){
        switch(pids[i+writers] = fork()) {
            case -1: {
                perror("Main: readers fork() error\n");
                exit(EXIT_FAILURE);
            }
            case 0: {
                if(execl("./reader", "reader", NULL) == -1){
                    perror("Main: readers execl() error\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    int stat, x;
    while(x = wait(&stat) > 0){
        if (x == -1){
            perror("wait error");
            interuption();
            exit(EXIT_FAILURE);
        }
    }

    exit(0);
}

void makeMemory() {
    memoryKey = ftok(".", 0);
    if(memoryKey == -1) {
        perror("\nmemory ftok error\n");
        exit(EXIT_FAILURE);
    }
    memId = shmget(memoryKey, 2 * sizeof(long int)+1, 0600 | IPC_CREAT);        //shmget(klucz, rozmiar obszaru pamieci, flagi)
    if(memId == -1){
        perror("shmget error\n");
        exit(EXIT_FAILURE);
    }
}

void connectMemory() {
    mem = shmat(memId, 0, 0);                           //shmmat(id pamieci dzielonej, system sam znajdzie adres, zadne specjalne flagi)
    if(*mem == -1 && errno !=0) {
        perror("shmat error\n");
        exit(EXIT_FAILURE);
    }
}

void closeMemory() {
    if(shmctl(memId, IPC_RMID, 0) == -1 || shmdt(mem) == -1){
        perror("close memory error\n");
        exit(EXIT_FAILURE);
    }
}

void makeSemaphores() {
    semKey = ftok(".", 0);
    if(semKey == -1){
        perror("semaphors ftok error\n");
        exit(EXIT_FAILURE);
    }
    semaforId = semget(semKey, 2, IPC_CREAT | 0600);        //semget(klucz, liczba semaforow, flaga)
    if(semaforId == -1){
        perror("semget error\n");
        exit(EXIT_FAILURE);
    }
}

void setSemaphores(int numer, int value)
{
    int setSem, i;

    setSem = semctl(semaforId, numer, SETVAL, value);       //semctl(id semafora, numer semaforaw tablicy semaforow, numer operacji, wartosc ktora ma byc wstawiona)
    if(setSem == -1){
        perror("semctl error\n");
        exit(EXIT_FAILURE);
    }
}


void closeSemaphores(void)
{
    int delSem;
    delSem = semctl(semaforId, 0, IPC_RMID);
    if(delSem == -1) {
        perror("semctl error\n");
        exit(EXIT_FAILURE);
    }
}

void handler(int sig)
{
    interuption();
    exit(0);
}

void interuption(){
    int x, pid;
    for(int i = 0; i < writers+readers; i++){
        pid = pids[i];
        kill(pid, SIGINT);
    }
    closeSemaphores();
    closeMemory();
}