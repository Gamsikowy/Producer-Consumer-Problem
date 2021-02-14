#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>

#define RG 10//rozmiar bufora głównego

int main(int argc, char *argv[]){
	if(!argv[1]){
		printf("Prosze podac liczbe produktow\n");
		exit(1);
	}
	srand(time(NULL));

	int *wolne;//wolne pozycje
	int *zajete;//zajęte pozycje
	int *buf_gl;//bufor główny
	int *licznik;//ilość aktywnych producentów
	int *suma_produktow;//suma produktów do konsumpcji
	int pid = getpid(), indeks_wstawiania, indeks_usuwania, wolna_pozycja_w, wolna_pozycja_z, produkt, i, LP = atoi(argv[1]);//liczba produktów

	int fd1 = shm_open("/wolne", O_CREAT|O_RDWR, 0600);
	int fd2 = shm_open("/zajete", O_CREAT|O_RDWR, 0600);
	int fd3 = shm_open("/buf_gl", O_CREAT|O_RDWR, 0600);
	int fd4 = shm_open("/suma_produktow", O_CREAT|O_RDWR, 0600);

	if(ftruncate(fd1,(RG+2)*sizeof(int)) || ftruncate(fd2,(RG+2)*sizeof(int)) || ftruncate(fd3,RG*sizeof(int)) || ftruncate(fd4,sizeof(int))){
		perror("ftruncate\n");
		exit(1);
	}

	wolne = mmap(NULL, (RG+2)*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd1, 0);
	zajete = mmap(NULL, (RG+2)*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd2, 0);
	buf_gl = mmap(NULL, RG*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd3, 0);
	suma_produktow = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd4, 0);

	int fd0 = shm_open("/licznik", O_CREAT|O_RDWR|O_EXCL, 0600);
	//jeśli fd0 == -1, z powodu flagi O_EXCL, to dany proces nie jest pierwszym w systemie. W przeciwnym wypadku należy przypisać odpowiednie  wartości początkowe
	if(fd0 == -1){
		fd0 = shm_open("/licznik", O_CREAT|O_RDWR, 0600);
		if(ftruncate(fd0, sizeof(int))) {
			perror("ftruncate_licznik\n");
			exit(1);
		}
		licznik = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED , fd0, 0);
	} else {
		if(ftruncate(fd0, sizeof(int))) {
			perror("ftruncate_licznik\n");
			exit(1);
		}
		licznik = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED , fd0, 0);
		licznik[0] = 0;
		suma_produktow[0] = 0;
		wolne[RG] = 0;
		wolne[RG+1] = 0;
		for(i=0; i<RG; i++) wolne[i] = i;
		zajete[RG] = 0;
		zajete[RG+1] = 0;
		for(i=0; i<RG+2; i++) zajete[i] = -1;
	}

	if (wolne == MAP_FAILED || zajete == MAP_FAILED || buf_gl == MAP_FAILED || suma_produktow == MAP_FAILED || licznik == MAP_FAILED){
		perror("mmap\n");
		exit(1);
	}

	sem_t *Sp1 = sem_open("/Sp1", O_CREAT|O_RDWR, 0600, 1);
	sem_t *Sp2 = sem_open("/Sp2", O_CREAT|O_RDWR, 0600, 1);
	sem_t *Sp3 = sem_open("/Sp3", O_CREAT|O_RDWR, 0600, 1);//binarne semafory pomocnicze
	sem_t *Sw = sem_open("/Sw", O_CREAT|O_RDWR, 0600, RG);//semaforWolne
	sem_t *Sz = sem_open("/Sz", O_CREAT|O_RDWR, 0600, 0);//semaforZajete

	if (Sw == SEM_FAILED){
		perror("sem_open_Sw\n");
		exit(1);
	}

	if (Sz == SEM_FAILED){
		perror("sem_open_Sz\n");
		exit(1);
	}

	if (Sp1 == SEM_FAILED){
		perror("sem_open_Sp1\n");
		exit(1);
	}

	if (Sp2 == SEM_FAILED){
		perror("sem_open_Sp2\n");
		exit(1);
	}

	if (Sp3 == SEM_FAILED){
		perror("sem_open_Sp3\n");
		exit(1);
	}

	//inkrementacja ilości producentów
	sem_wait(Sp3);
	licznik[0]++;
	suma_produktow[0] += LP;
	sem_post(Sp3);

	for(i=0; i<LP; i++){
		sem_wait(Sw);
		sem_wait(Sp1);
		wolna_pozycja_w = wolne[RG];
		wolne[RG] = (wolne[RG]+1)%RG;
		indeks_wstawiania = wolne[wolna_pozycja_w];
		wolne[wolna_pozycja_w] = -1;
		sem_post(Sp1);

		sleep(rand()%4+4);
		produkt = (rand()%50+10);
		buf_gl[indeks_wstawiania] = produkt;

		sem_wait(Sp2);
		wolna_pozycja_z = zajete[RG];
		zajete[RG] = (zajete[RG]+1)%RG;
		zajete[wolna_pozycja_z] = indeks_wstawiania;
		printf("Producent: %d umieszcza produkt: %d na miejscu: %d\n", pid, produkt, indeks_wstawiania);
		sem_post(Sp2);
		sem_post(Sz);
	}

	//dekrementacja ilości producentów
	sem_wait(Sp3);
	licznik[0]--;
	sem_post(Sp3);

	sem_close(Sp1);
	sem_close(Sp2);
	sem_close(Sp3);
	sem_unlink("/Sp1");
	sem_unlink("/Sp2");
	sem_unlink("/Sp3");
	if(licznik[0] == 0){
		shm_unlink("/licznik");
		munmap(licznik, sizeof(int));
	}
	printf("Producent %d zakonczyl prace\n", pid);
}
