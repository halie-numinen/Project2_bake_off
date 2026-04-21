// thread
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
// shared memory
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
// semaphores
#include <sys/sem.h>

volatile sig_atomic_t shutdown = 1;

void* bakerActivities(void* arg); 

void handleSigint(int sig) { // helpes with garaceful shutdown
    if (sig == SIGINT) {
        shutdown = 0;
    }
}

struct sembuf acquire = { 0, -1, SEM_UNDO}; 
struct sembuf release = { 0, +1, SEM_UNDO};

int semId;

int main () {
    int shmId;
    key_t key = IPC_PRIVATE;
    pthread_t thread1[500];  
    int threadStatus1; 
    int *threadNumber;
    
    int bakers;
    char *errorCheacking;
    char* noBakers;
    
    printf("Please enter the number of bakers you want: ");
    noBakers = fgets(numberOfBakersString, sizeof(numberOfBakersString), stdin);
    if (noBakers != NULL && strlen(numberOfBakersString) < 1) {
        exit(1);
    }
    numberOfBakersString[strcspn(numberOfBakersString, "\n")] = '\0';
    bakers = (int)strtol(numberOfBakersString, &errorCheacking, 10);
    signal(SIGINT, handleSigint);

    if ((shmId = shmget(key, 1024, IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
        perror("Unable to get shared memory\n");
        exit(1);
    }
    if ((semId = semget(IPC_PRIVATE, 6, S_IRUSR | S_IWUSR)) == -1) { 
        perror("Unable to get semaphore\n");
        exit(1);
    }
    if ((sharedMemoryPtr = (struct resource *) shmat(shmId, 0, 0)) == (void*) -1) { // add in the main struct here (mixer, pantry, spoon, ect.)
        perror("Unable to attach\n");
        exit(1);
    }
    int maxAmounts[] = {1, 2, 2, 3, 5, 1}; // pantries, friges, mixers, bowls, spoons, ovens
    for (int i = 0; i < 6; i++) {
        if (semctl(semId, i, SETVAL, maxAmounts[i]) == -1) {
            perror("unable to initialize semaphore\n");
            exit(1);
        }
    }

    for (int i = 0; i < bakers; i++) {
        threadNumber = malloc(sizeof(int));
        if (!threadNumber) {
            perror("malloc failed.");
            exit(1);
        } 

        threadStatus1 = pthread_create(&thread1[i], NULL,  bakerActivities, threadNumber); // what is the last number (pointer to the number of bakers)
        if (threadStatus1 != 0){ 
            fprintf (stderr, "Thread create error %d: %s\n", threadStatus1, strerror(threadStatus1)); 
            free(threadNumber);
            continue;
        } 
    }
    for(int i = 0; i < bakers; i++) {
        pthread_join(thread1[i], NULL); 
    } 
    if (shmdt(kitchen) < 0) { // the shared memory pointer
        perror("Unable to detach shared memory\n");
        exit(1);
    }
    if (semctl(semId, 0, IPC_RMID) == -1) {
        perror("Unable to deallocate semophore\n");
        exit(1);
    }
    if (shmctl(shmId, IPC_RMID, 0) < 0) {
        perror("Unable to deallocate shared memory\n");
        exit(0);
    }
    return 0;
}

void* bakerActivities(void* bakerId) {
    // for the semaphores and locking and unlocking resources
    int numberOfRecipies = 5;
    char *colors[] = {"red", "gold", "green", "blue", "yellow", "orange", "purple", "pink", "white", "black", "brown", "silver"};
    int id = *((int*)bakerId);
    free(bakerId);
    return NULL;
}