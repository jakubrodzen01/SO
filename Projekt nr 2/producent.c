#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>

int pamiec;
int semafor;
int klucz;
int odl1, odl2;
char *adresPamieci;
char znak;


void utworzPamiecDzielona();
void dolaczPamiecDzielona();
void odlaczPamiecDzielona();
static void semaforPlus(int semaforNr);
static void semaforMinus(int semaforNr);
static void semaforNa1(int semaforNr);
static void utworzSemafor();
static void semaforyNa0();
static void usunSemafor();



int main()
{	int i=0;
  char z;
	FILE * file;
  if (!(file = fopen("in","r")))
  {
    fprintf(stderr, "Blad otwarcia pliku in!");
    exit(EXIT_FAILURE);
  }
  utworzSemafor();
  semaforyNa0();
  utworzPamiecDzielona();
  dolaczPamiecDzielona();

  semaforPlus(1);
 
  do
	{ 
    semaforMinus(1);
    znak=fgetc(file);
    *adresPamieci=znak;
    printf("Producent sprzedal: %c\n",znak);
    semaforPlus(0);
	}
  while(znak != EOF);
  semaforMinus(1);

  odlaczPamiecDzielona();
  usunSemafor();
  fclose(file);
  exit(0);
}

//tworzenie dostepu do pamieci dzielonej
void utworzPamiecDzielona()
{
  pamiec=shmget(990, 1, 0606 | IPC_CREAT);  //zwraca id pamieci dzielonej
  if (pamiec==-1) 
    {
      printf("Problemy z utworzeniem pamieci dzielonej");
      exit(EXIT_FAILURE);
    }
}
//            dolaczenie (zyskanie dostepu) pamieci dzielonej
void dolaczPamiecDzielona()
{
    adresPamieci = shmat(pamiec, 0, 0);   //zwaraca wskaznik do pierwszego bajtu pamieci pierwsze 0 w wolnym miejscu drugie 0 prawa rw
    if (*adresPamieci == -1)
    {
      printf("shmat error\n");
      exit(EXIT_FAILURE);
    }
}
void odlaczPamiecDzielona()
  {
    odl1=shmctl(pamiec,IPC_RMID,0); //usuniecie segmentu pamieci, zwraca 0 lub wartosc wymagana przez cmd, na koncu wskaznik do struktury zawierajacej tryby i zezwolenia 
    odl2=shmdt(adresPamieci);// odlaczenie, zwraca 0
    if (odl1==-1 || odl2==-1)
    {
      printf("Problemy z odlaczeniem pamieci dzielonej.\n");
      exit(EXIT_FAILURE);
    }
    else printf("Pamiec dzielona zostala odlaczona u producenta.\n");
  }
//tworzenie semafora
static void utworzSemafor()
{
  klucz = ftok(".", 'a'); //zwraca wartosc klucza, (sciezkowa nazwa istniejacego pliku, pojedynczy znak identyfikujacy projekt)
	semafor = semget(990,2,IPC_CREAT|0606); //zwraca id semafora
	if(semafor == -1) 
  {
	  perror("semget error");
	  exit(EXIT_FAILURE);
	}
}
static void usunSemafor()
{
	int usun;
  usun=semctl(semafor,0,IPC_RMID);            //zwraca 0 lub wartosc wymagana od cmd (id semafora, numer semafora na zbiorze, kod_polecenia, parametry polecenia);
	if (usun == -1)
  {
		printf("semctl error\n");
	  exit(EXIT_FAILURE);                      //uniwersalny dla wielu jezykow
	}
  else 
  {
    printf("Pomyslnie usunieto semafory: %d\n",semafor);
  }
}
// ustaw wartosc na semaforze
static void semaforyNa0(void)
  {
    int ustaw;
    int i;
    for(i=0;i<2;i++)
    {
        ustaw=semctl(semafor,i,SETVAL,0);
      if (ustaw==-1)
        {
          printf("Nie mozna ustawic semafora.\n");
          exit(EXIT_FAILURE);
        }
    }
    
  }

//     podniesienie wartosci semafora
static void semaforPlus(int semaforNr)
{
    int zmien_sem;
    struct sembuf bufor_sem;
    bufor_sem.sem_num=semaforNr;
    bufor_sem.sem_op=1;
    bufor_sem.sem_flg=0;
    zmien_sem=semop(semafor,&bufor_sem,1);  //zwraca 0 (id zbioru semaforow,wskaznikik do tablicy struktur okreslajacy operacje na zbiorze semaforow, iczba semaforow na ktorych ma byc wykonana operacja)
    if (zmien_sem==-1) 
    {
      if(errno == ERANGE) 
        {
          semaforNa1(semaforNr);
        }
      else
      {
        printf("Nie moglem otworzyc semafora.\n");
        exit(EXIT_FAILURE);
      }
    }
}
static void semaforNa1(int semaforNr)
{
  int ustaw;
  ustaw = semctl(semafor,semaforNr,SETVAL,1);
  if(ustaw == -1)
  {
    printf("ustaw na 1 error\n");
	  exit(EXIT_FAILURE);
  }
}
//        obnizenie wartosci semafora
static void semaforMinus(int semaforNr)
{
    int zmien_sem;
    struct sembuf bufor_sem;
    bufor_sem.sem_num=semaforNr;
    bufor_sem.sem_op=-1;
    bufor_sem.sem_flg=0;
    zmien_sem=semop(semafor,&bufor_sem,1);  //zwraca 0 (id zbioru semaforow, wskaznik do tablicy struktur okreslajacy operacje na zbiorze semaforow, liczba semaforow na ktorych ma byc wykonana operacja)
    if (zmien_sem==-1) 
    {
      if(errno == EINTR) //ctrl+z, semafor sie gubi i wypada z kolejki
      {
        semaforMinus(semaforNr); //ustawiasz w kolejke tam gdzie powinien byc
      }
      else
      {
        printf("semop error\n");
	      exit(EXIT_FAILURE);
      }
  }
}
