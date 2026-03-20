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

void handleSigint(int sig) { // helpes with garaceful shutdown
    if (sig == SIGINT) {
        shutdown = 0;
    }
}

int main () {
    int semId, shmId;
    key_t key = IPC_PRIVATE;
    if ((shmId = shmget(key, 1024, IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
        perror("Unable to get shared memory\n");
        exit(1);
    }
    if ((semId = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR)) == -1) { // to get the lest 2 questions I just changed the permissions here
        perror("Unable to get semaphore\n");
        exit(1);
    }
    signal(SIGINT, handleSigint);
    if ((sharedMemoryPtr = (struct sharedData *) shmat(shmId, 0, 0)) == (void*) -1) { // Need to cordinate with readers somehow
        perror("Unable to attach\n");
        exit(1);
    if (semctl(semId, 0, SETVAL, 1) == -1) {
        perror("unable to initialize semaphore\n");
        exit(1);
    }

    }
    return 0;
}
