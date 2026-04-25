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
#include <semaphore.h>

//Below are all the structs with resources, ingredients, and recipes
struct pantry { 
    int flourId;
    int sugarId;
    int yeastId;
    int bakingSodaId;
    int saltId;
    int cinnamonId;
};
struct refrigerator { 
    int eggsId;
    int milkId;
    int butterId;
};
struct resource {
    int mixerId;
    struct pantry pantryIngredient;
    struct refrigerator fridge; 
    int bowlId;
    int spoonId;
    int ovenId;
};
struct recipe {
    char recipeName[20];
    int ingredienceIds[10];
    int toolIds[5];
};
struct recipe recipesList[5];

volatile sig_atomic_t shutdown = 1;

void* bakerActivities(void* arg); 
void acquireSemaphore (void);
void releaseSemaphore (void);

void handleSigint(int sig) { // helps with graceful shutdown
    if (sig == SIGINT) {
        shutdown = 0;
    }
}

struct sembuf acquire = { 0, -1, SEM_UNDO}; 
struct sembuf release = { 0, +1, SEM_UNDO};

int semId;

int main () {
    struct resource *kitchen;
    srand(time(NULL));
    int shmId;
    key_t key = IPC_PRIVATE;
    pthread_t thread1[500];  
    int threadStatus1; 
    int *threadNumber;
    char numberOfBakersString[10];
    int bakers;
    char *errorChecking;
    char* noBakers;
    
    printf("Please enter the number of bakers you want: ");
    noBakers = fgets(numberOfBakersString, sizeof(numberOfBakersString), stdin);
    if (noBakers != NULL && strlen(numberOfBakersString) < 1) {
        exit(1);
    }
    numberOfBakersString[strcspn(numberOfBakersString, "\n")] = '\0';
    bakers = (int)strtol(numberOfBakersString, &errorChecking, 10);
    signal(SIGINT, handleSigint);

    recipesList[0] = (struct recipe){"cookies", {1, 2, 8, 9, 0}, {10, 11, 12, 13, 0}};
    recipesList[1] = (struct recipe){"pancakes", {1, 2, 4, 5, 7, 8, 9, 0}, {10, 11, 12, 13, 0}};
    recipesList[2] = (struct recipe){"homemade pizza dough", {3, 2, 5, 0}, {10, 11, 12, 13, 0}};
    recipesList[3] = (struct recipe){"soft pretzels", {1, 2, 5, 3, 4, 7, 0}, {10, 11, 12, 13, 0}};
    recipesList[4] = (struct recipe){"cinnamon rolls", {1, 2, 5, 9, 7, 6, 0}, {10, 11, 12, 13, 0}};

    if ((shmId = shmget(key, 1024, IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
        perror("Unable to get shared memory\n");
        exit(1);
    }
    if ((semId = semget(IPC_PRIVATE, 6, S_IRUSR | S_IWUSR)) == -1) { 
        perror("Unable to get semaphore\n");
        exit(1);
    }
    if ((kitchen = (struct resource *) shmat(shmId, 0, 0)) == (void*) -1) { 
        perror("Unable to attach\n");
        exit(1);
    }
    int maxAmounts[] = {1, 2, 2, 3, 5, 1}; // pantries, fridges, mixers, bowls, spoons, ovens
    for (int i = 0; i < 6; i++) {
        if (semctl(semId, i, SETVAL, maxAmounts[i]) == -1) {
            perror("unable to initialize semaphore\n");
            break;
        }
    }

    kitchen->pantryIngredient.flourId = 1;
    kitchen->pantryIngredient.sugarId = 2;
    kitchen->pantryIngredient.yeastId = 3;
    kitchen->pantryIngredient.bakingSodaId = 4;
    kitchen->pantryIngredient.saltId = 5;
    kitchen->pantryIngredient.cinnamonId = 6;
    kitchen->fridge.eggsId = 7;
    kitchen->fridge.milkId = 8;
    kitchen->fridge.butterId = 9;
    kitchen->mixerId = 10;
    kitchen->bowlId = 11;
    kitchen->spoonId = 12;
    kitchen->ovenId = 13;

    for (int i = 0; i < bakers; i++) {
        threadNumber = malloc(sizeof(int));
        if (!threadNumber) {
            perror("malloc failed.");
            exit(1);
        } 
        *threadNumber = i; // an Id
        threadStatus1 = pthread_create(&thread1[i], NULL,  bakerActivities, threadNumber); 
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
        perror("Unable to deallocate semaphore\n");
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
    int numberOfRecipes = 5;
    char *ingredients[] = {"flour", "sugar", "yeast", "baking soda", "salt", "cinnamon", "eggs", "milk", "butter"};
    char *colors[] = {"red", "gold", "green", "blue", "yellow", "orange", "purple", "pink", "white", "black", "brown", "silver"};
    int id = *((int*)bakerId);
    free(bakerId);
    char *color = colors[id % 12];
    for (int i = 0; i<numberOfRecipes; i++) {
        int ramsied = (rand() % 10) + 1;
        struct recipe currentRecipe = recipesList[i]; // this is a struct that holds a list of the recipes with (name, ingrediens -list, tools-list)
        printf("Baker %s starting the %s recipe\n", color, currentRecipe.recipeName);
        int numberOfIngredients = 0;
        while (currentRecipe.ingredientIds[numberOfIngredients] != 0) {
            numberOfIngredients++;
        }
        
        int pantry = 0;
        int fridge = 0;
        for (int j = 0; j < numberOfIngredients; j++) {
            if (currentRecipe.ingredientIds[j] >= 1 && currentRecipe.ingredientIds[j] <= 6) {
                pantry = 1;
                break;
            }
        }
        if (pantry == 1) {
            acquire.sem_num = 0; // pantryIndex
            acquireSemaphore();
            for (int k = 0; k < numberOfIngredients; k++) {
                int pantryId = currentRecipe.ingredientIds[k];
                if (pantryId >= 1 && pantryId <= 6) {
                    printf("Baker %s is getting the %s ingredient from the pantry\n", color, ingredients[pantryId-1]);
                }
            }
            if (ramsied == 5 && id == 1) {
                printf("Baker %s has been RAMSIED and must start over!\n", color);
                if (pantry == 1) {
                    release.sem_num = 0;
                    releaseSemaphore();
                }
                i -= 1;
                continue;
            }
            release.sem_num = 0;
            releaseSemaphore();
        }
        for (int j = 0; j < numberOfIngredients; j++) {
            if (currentRecipe.ingredientIds[j] >= 7 && currentRecipe.ingredientIds[j] <= 9) {
                fridge = 1;
                break;
            }
        }
        if (fridge == 1) {
            acquire.sem_num = 1; // fridgeIndex
            acquireSemaphore();
            for (int k = 0; k < numberOfIngredients; k++) {
                int fridgeId = currentRecipe.ingredientIds[k];
                if (fridgeId >= 7 && fridgeId <= 9) {
                    printf("Baker %s is getting the %s ingredient from the fridge\n", color, ingredients[fridgeId-1]);
                }  
            }
            if (ramsied == 5 && id == 1) {
                printf("Baker %s has been RAMSIED and must start over!\n", color);
                if (fridge == 1) {
                    release.sem_num = 1;
                    releaseSemaphore();
                }
                i -= 1;
                continue;
            }
            release.sem_num = 1;
            releaseSemaphore();
        }
        for (int k = 0; k < 3; k++) { // acquire tools loop
            
            if (currentRecipe.toolIds[k] == 10) {
                acquire.sem_num = 2; // mixerIndex
                acquireSemaphore();
            }
            if (currentRecipe.toolIds[k] == 11) {
                acquire.sem_num = 3; // bowlIndex
                acquireSemaphore();
            }
            if (currentRecipe.toolIds[k] == 12) {
                acquire.sem_num = 4; // spoonIndex
                acquireSemaphore();
            }
        }
        if (id == 1 && ramsied == 5) {
            printf("Baker %s has been RAMSIED and must start over!\n", color);
            for (int l = 0; l < 3; l++)  { // release loop
                if (currentRecipe.toolIds[l] == 10) {
                    release.sem_num = 2;
                    releaseSemaphore();
                    printf("releaseing the mixer\n");
                }
                if (currentRecipe.toolIds[l] == 11) {
                    release.sem_num = 3;
                    releaseSemaphore();
                    printf("releasing the bowl\n");
                }
                if (currentRecipe.toolIds[l] == 12) {
                    release.sem_num = 4;
                    releaseSemaphore();
                    printf("releasing the spoon\n");
                }
            }
            i -= 1;
            continue;
        } 
        printf("Baker %s got all of the tools, now mixing the ingredients together\n", color);
        for (int l = 0; l < 3; l++)  {   // release loop
            if (currentRecipe.toolIds[l] == 10) {
                release.sem_num = 2;
                releaseSemaphore();
                printf("releasing the mixer\n");
            }
            if (currentRecipe.toolIds[l] == 11) {
                release.sem_num = 3;
                releaseSemaphore();
                printf("releasing the bowl\n");
            }
            if (currentRecipe.toolIds[l] == 12) {
                release.sem_num = 4;
                releaseSemaphore();
                printf("releasing the spoon\n");
            }
        }
        if (currentRecipe.toolIds[3] == 13) { // there is only one oven so no need for a loop
            acquire.sem_num = 5; // ovenIndex
            acquireSemaphore();
            printf("Baker %s got the oven and is baking the %s recipe\n", color, currentRecipe.recipeName);
            release.sem_num = 5;
            releaseSemaphore(); 
        }
        printf("Baker %s finished the %s recipe\n\n", color, currentRecipe.recipeName);
    }
    printf("Baker %s finished all of the recipes\n", color);
    return NULL;
}

void acquireSemaphore (void) {
    if (semop(semId, &acquire, 1) == -1) {
        perror("semop acquired failed\n");
        exit(1);
    }
}

void releaseSemaphore (void) {
    if (semop(semId, &release, 1) == -1) {
        perror("semop release failed\n");
        exit(1);
    }
}
