// TODO Complete the assignment following the instructions in the assignment
// document. Refer to the Concurrency lecture notes in Canvas for examples.
// You may also find slides 50 and 51 from Ben Brewster's Concurrency slide
// deck to be helpful (you can find Brewster's slide decks in Canvas -> files).

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int myCount = 0;

// Mutex and condition variables
pthread_mutex_t myMutex;
pthread_cond_t myCond1, myCond2;

// Consumer thread function
void* consumer(void* arg) {
    int prevCount = 0;  // Ensure consumer only processes new increments

    do {
        // Lock the mutex 
        pthread_mutex_lock(&myMutex);
        printf("CONSUMER: MUTEX LOCKED\n");

        // Wait until myCount actually changes
        while (myCount <= prevCount) {  
            printf("CONSUMER: WAITING ON CONDITION VARIABLE 1\n");
            pthread_cond_wait(&myCond1, &myMutex);
        }

        // Print the updated count
        printf("Count updated: %d -> %d\n", prevCount, myCount);
        prevCount = myCount; 

        // Signal the producer that the count update is processed
        printf("CONSUMER: SIGNALING CONDITION VARIABLE 2\n");
        pthread_cond_signal(&myCond2);

        // Unlock the mutex so the producer can proceed
        printf("CONSUMER: MUTEX UNLOCKED\n");
        pthread_mutex_unlock(&myMutex);


    }while(myCount !=10);
    return NULL;
}

int main() {
    printf("PROGRAM START\n");

    // Initialize mutex and condition variables
    pthread_t consumer_thread;
    pthread_mutex_init(&myMutex, NULL);
    pthread_cond_init(&myCond1, NULL);
    pthread_cond_init(&myCond2, NULL);

    // Create the consumer thread
    pthread_create(&consumer_thread, NULL, consumer, NULL);
    printf("CONSUMER THREAD CREATED\n");
int i;
    for (i = 0; i < 10; i++) {
        // Lock the mutex before changing the shared variable
        pthread_mutex_lock(&myMutex);
        printf("PRODUCER: MUTEX LOCKED\n");  

        myCount++;  

        // Signal the consumer that myCount has been ++
        printf("PRODUCER: SIGNALING CONDITION VARIABLE 1\n");  
        pthread_cond_signal(&myCond1);

        
        if (myCount < 10) {
            printf("PRODUCER: WAITING ON CONDITION VARIABLE 2\n");  
            pthread_cond_wait(&myCond2, &myMutex);
        }

        // Unlock the mutex so the consumer can continue
        printf("PRODUCER: MUTEX UNLOCKED\n");  
        pthread_mutex_unlock(&myMutex);
    }

    // Make sure the consumer thread finishes
    pthread_join(consumer_thread, NULL);

    // Destroy mutex and condition variables
    pthread_mutex_destroy(&myMutex);
    pthread_cond_destroy(&myCond1);
    pthread_cond_destroy(&myCond2);

    printf("PROGRAM END\n");
    return 0;
}
