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
static void usunSemafor();


int main()
{	int i=0;
  char z;
	FILE * file;
  if (!(file = fopen("out","w")))
  {
    fprintf(stderr, "Blad otwarcia pliku out!");
    exit(EXIT_FAILURE);
  }
  utworzSemafor();
  utworzPamiecDzielona();
  dolaczPamiecDzielona();


  while(1)
	{
    semaforMinus(0);
    if(*adresPamieci!=EOF)
    {
      printf("Konsument skonsumowal: %c\n",*adresPamieci);
      fprintf(file,"%c",*adresPamieci);
      semaforPlus(1);
    }
    else 
    {
      break;
    }
	}
  semaforPlus(1);
  odlaczPamiecDzielona();
  printf("Koniec pobierania wartosci\n");
  exit(0);
}
//             tworzenie semafora
static void utworzSemafor()
{
  klucz = ftok(".", 'a'); //zwraca wartosc klucza, (sciezkowa nazwa istniejacego pliku, pojedynczy znak identyfikujacy projekt)
	semafor = semget(990,2,IPC_CREAT|0606);                   //semget(klucz,liczba_semaforow,flaga i prawa dostepu); zwraca identyfikator semafora lub -1
	if(semafor == -1) 
  {
	  perror("semget error");
	  exit(EXIT_FAILURE);
	}
} // usun semafor
static void usunSemafor()
{
	int usun;
  usun=semctl(semafor,0,IPC_RMID);  //zwraca 0 lub wartosc wymagana od cmd (id semafora, numer semafora na zbiorze, kod_polecenia, parametry polecenia);
	if (usun == -1)
  {
	  printf("semctl error\n");
	  exit(EXIT_FAILURE);
	}
  else 
  {
    printf("Pomyslnie usunieto semafor: %d\n",semafor);
  }
}

//            tworzenie dostepu do pamieci dzielonej
void utworzPamiecDzielona()
{
  pamiec=shmget(990, 1, 0606 | IPC_CREAT);                     //shmget(klucz,rozmiar_pamieci,flaga); // zwraca identyfikator pamieci dzielonej lub -1
  if (pamiec==-1) 
    {
      printf("Problemy z utworzeniem pamieci dzielonej lub producent nie zostal wlaczony");
      exit(EXIT_FAILURE);
    }
}
//            dolaczenie (zyskanie dostepu) pamieci dzielonej
void dolaczPamiecDzielona()
{
    adresPamieci = shmat(pamiec, 0, 0);           // shmat(id_segmentu_pamieci)
    if (*adresPamieci == -1)
    {
      if(*adresPamieci == EOF)
      {
        printf("Plik jest pusty!\n");
      }
      else
      {
        printf("shmat error\n");
        exit(EXIT_FAILURE);
      }
      
    }
}
void odlaczPamiecDzielona()
  {
    odl2=shmdt(adresPamieci);
    if (odl2==-1)
      {
        printf("Problemy z odlaczeniem pamieci dzielonej.\n");
        exit(EXIT_FAILURE);
      }
    else printf("Pamiec dzielona zostala odlaczona u konsumenta.\n");
  }

//     podniesienie wartosci semafora
static void semaforPlus(int semaforNr)
{
    int zmien_sem;
    struct sembuf bufor_sem;
    bufor_sem.sem_num=semaforNr;
    bufor_sem.sem_op=1;
    bufor_sem.sem_flg=0;
    zmien_sem=semop(semafor,&bufor_sem,1);	  //zwraca 0 (id zbioru semaforow,wskaznik do tablicy struktur okreslajacych operacje na zbiorze semaforow, liczba semaforow na ktorych ma byc wykonana operacja)
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
    zmien_sem=semop(semafor,&bufor_sem,1);  //zwraca 0 (id zbioru semaforow, – wskaźnik do tablicy struktur określający operacje na zbiorze semaforów, liczba semaforow na ktorych ma byc wykonana operacja)
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
