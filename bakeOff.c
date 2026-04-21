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
    char numberOfBakersString[10];
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
    if ((kitchen = (struct resource *) shmat(shmId, 0, 0)) == (void*) -1) { // add in the main struct here (mixer, pantry, spoon, ect.) I the name you have to replace or keep is kitchen
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
        *threadNumber = i; // an Id
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
    char *color = colors[id % 12];
    for (int i = 0; i<numberOfRecipies; i++) {
        struct recipe currentRecipe = recipiesList[i]; // this is a struct that holds a list of the recipies with (name, ingrediens -list, tools-list)
        // char *currentRecipe[] = currentRecipeInfo->recipiesList[j];
        // lock it let the baker do the things
        printf("Baker %s starting the %s recipe\n", color, currentRecipe.recipeName);
        int numberOfIngreadients = 0; // 3
        while (currentRecipe.ingredienceIds[numberOfIngreadients] != 0) {
            numberOfIngreadients++;
        }
        int numberOfTools = 0;
        while (currentRecipe.toolIds[numberOfTools] != 0) { // do i need this if it is always going to be 3
            numberOfTools++;
        }
        for (int j = 0; j < numberOfIngreadients; j++) { // acquire and release ingredients loop
            if (currentRecipe.ingredienceIds[j] >= 1 && currentRecipe.ingredienceIds[j] <= 6) {
                acquire.sem_num = 0; // pantryIndex
                if (semop(semId, &acquire, 1) == -1) {
                    perror("semop acquired failed\n");
                    exit(1);
                }
                // do the stuff
                printf("Baker %s getting the ingrediance in the pantry\n", color);
                release.sem_num = 0;
                if (semop(semId, &release, 1) == -1) {
                    perror("semop release failed\n");
                    exit(1);
                }
            }
            if (currentRecipe.ingredienceIds[j] >= 7 && currentRecipe.ingredienceIds[j] <= 9) {
                acquire.sem_num = 1; // fridgeIndex
                if (semop(semId, &acquire, 1) == -1) {
                    perror("semop acquired failed\n");
                    exit(1);
                }
                // do the stuff
                printf("Baker %s getting the ingrediance in the fridge\n", color);
                release.sem_num = 1;
                if (semop(semId, &release, 1) == -1) {
                    perror("semop release failed\n");
                    exit(1);
                }
            }
        }
        for (int k = 0; k < numberOfTools; k++) { // aquier tools loop
            if (currentRecipe.toolIds[k] == 10) {
                acquire.sem_num = 2; // pantryIndex
                if (semop(semId, &acquire, 1) == -1) {
                    perror("semop acquired failed\n");
                    exit(1);
                }
                // do the stuff
                printf("Baker %s got the mixer\n", color);
            }
            if (currentRecipe.toolIds[k] == 11) {
                acquire.sem_num = 3; // pantryIndex
                if (semop(semId, &acquire, 1) == -1) {
                    perror("semop acquired failed\n");
                    exit(1);
                }
                // do the stuff
                printf("Baker %s got the bowl\n", color);
                
            }
            if (currentRecipe.toolIds[k] == 12) {
                acquire.sem_num = 4; // pantryIndex
                if (semop(semId, &acquire, 1) == -1) {
                    perror("semop acquired failed\n");
                    exit(1);
                }
                // do the stuff
                printf("Baker %s got the spoon\n", color);
                
            }
        }
        printf("Got all of the tools, now mixing the ingerediens together\n");
        for (int l = 0; l < numberOfTools; l++)  {   // release loop
            if (currentRecipe.toolIds[l] == 10) {
                release.sem_num = 2;
                if (semop(semId, &release, 1) == -1) {
                    perror("semop release failed\n");
                    exit(1);
                }
                printf("releaseing the mixer\n");
            }
            if (currentRecipe.toolIds[l] == 11) {
                release.sem_num = 3;
                if (semop(semId, &release, 1) == -1) {
                    perror("semop release failed\n");
                    exit(1);
                }
                printf("releasing the bowl\n");
            }
            if (currentRecipe.toolIds[l] == 12) {
                release.sem_num = 4;
                if (semop(semId, &release, 1) == -1) {
                    perror("semop release failed\n");
                    exit(1);
                }
                printf("releasing the spoon\n");
            }
        }
        if (currentRecipe.toolIds[3] == 13) { // there is only one oven so no need for a looop
            acquire.sem_num = 5; // ovenIndex
            if (semop(semId, &acquire, 1) == -1) {
                perror("semop acquired failed\n");
                exit(1);
            }
            // do the stuff
            printf("Baker %s got the oven and is baking the %s recipe\n", color, currentRecipe.recipeName);
            release.sem_num = 5;
            if (semop(semId, &release, 1) == -1) {
                perror("semop release failed\n");
                exit(1);
            }  
        }
        printf("Baker %s finished the %s recipe\n", color, currentRecipe.recipeName);
    }
    return NULL;
}