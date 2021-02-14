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
#include <time.h>

#define RG 10//rozmiar bufora głównego

int main(int argc, char *argv[]){
	srand(time(NULL));

	int *wolne;//tablica wolnych pozycji
	int *zajete;//tablica zajetych pozycji
	int *buf_gl;//bufor główny
	int *suma_produktow;//ilość produktow do konsumpcji
	int pid = getpid(), produkt, indeks_pobrania, zajeta_pozycja_w, zajeta_pozycja_z;

	int fd1 = shm_open("/wolne", O_RDWR, 0600);
	int fd2 = shm_open("/zajete", O_RDWR, 0600);
	int fd3 = shm_open("/buf_gl", O_RDWR, 0600);
	int fd4 = shm_open("/suma_produktow", O_RDWR, 0600);

	wolne = mmap(NULL, (RG+2)*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd1, 0);
	zajete = mmap(NULL, (RG+2)*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd2, 0);
	buf_gl = mmap(NULL, RG*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd3, 0);
	suma_produktow = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd4, 0);

	if(buf_gl == MAP_FAILED || zajete == MAP_FAILED || wolne == MAP_FAILED || suma_produktow == MAP_FAILED){
		printf("Nie mozna aktywowac konsumenta, dopoki producent nie rozpocznie pracy\n");
		exit(1);
	}

	sem_t *Sp1 = sem_open("/Sp1", O_CREAT|O_RDWR, 0600, 1);
	sem_t *Sp2 = sem_open("/Sp2", O_CREAT|O_RDWR, 0600, 1);
	sem_t *Sw = sem_open("/Sw", O_RDWR, 0600, RG);
	sem_t *Sz = sem_open("/Sz", O_RDWR, 0600, 0);

	if(Sp1 == SEM_FAILED){
		perror("sem_open_Sp1\n");
		exit(1);
	}

	if(Sp2 == SEM_FAILED){
		perror("sem_open_Sp2\n");
		exit(1);
	}

	if(Sw == SEM_FAILED){
		perror("sem_open_Sw\n");
		exit(1);
	}

	if(Sz == SEM_FAILED){
		perror("sem_open_Sz\n");
		exit(1);
	}

	while(suma_produktow[0]-- > 0){
		sem_wait(Sz);
		sem_wait(Sp2);
		zajeta_pozycja_z = zajete[RG+1];
		zajete[RG+1] = (zajete[RG+1]+1)%RG;
		indeks_pobrania = zajete[zajeta_pozycja_z];
		zajete[zajeta_pozycja_z] = -1;
		sem_post(Sp2);
		sleep(rand()%3+3);
		produkt = buf_gl[indeks_pobrania];
		sem_wait(Sp1);
		zajeta_pozycja_w = wolne[RG+1];
		wolne[RG+1] = (wolne[RG+1]+1)%RG;
		wolne[zajeta_pozycja_w] = indeks_pobrania;
		sem_post(Sp1);
		printf("Kosument: %d korzysta z produktu: %d na miejscu: %d\n", pid, produkt, indeks_pobrania);
		sem_post(Sw);
	}

	sem_close(Sp1);
	sem_close(Sp2);
	sem_close(Sz);
	sem_close(Sw);
	sem_unlink("/Sp1");
	sem_unlink("/Sp2");
	sem_unlink("/Sw");
	sem_unlink("/Sz");
	shm_unlink("/wolne");
	shm_unlink("/zajete");
	shm_unlink("/buf_gl");
	shm_unlink("/suma_produktow");
	munmap(wolne, (RG+2)*sizeof(int));
	munmap(zajete, (RG+2)*sizeof(int));
	munmap(buf_gl, RG*sizeof(int));
	munmap(suma_produktow, sizeof(int));
	printf("Konsument: %d zakonczyl prace\n", pid);
}
