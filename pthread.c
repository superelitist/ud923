// Write a multi-threaded C program that gives readers priority over writers concerning a shared (global) variable.
// Essentially, if any readers are waiting, then they have priority over writer threads -- 
// writers can only write when there are no readers. This program should adhere to the following constraints:
// Multiple readers/writers must be supported (5 of each is fine)
// Readers must read the shared variable X number of times
// Writers must write the shared variable X number of times
// Readers must print:
// The value read
// The number of readers present when value is read
// Writers must print:
// The written value
// The number of readers present were when value is written (should be 0)
// Before a reader/writer attempts to access the shared variable it should wait some random amount of time
// Note: This will help ensure that reads and writes do not occur all at once
// Use pthreads, mutexes, and condition variables to synchronize access to the shared variable

#include <stdio.h>
#include <pthread.h>
#include <unistd.h> // for sleep
#include <stdlib.h> // because safety? 
#include <string.h> // for string manip
#include <stdbool.h> // what makes this different from #define true 1 I don't know...
#include <time.h>
#include <math.h>

#define NUMBER_OF_READERS 10
#define NUMBER_OF_WRITERS 5
#define NUMBER_OF_READS 5
#define NUMBER_OF_WRITES 3

__u_long shared_variable = 0;

pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER; // init statically
pthread_cond_t read_phase = PTHREAD_COND_INITIALIZER, write_phase = PTHREAD_COND_INITIALIZER; // same
int resource_counter;
__u_long readers_waiting;

unsigned GetFromUniformIntegerDistribution(unsigned min, unsigned max) { // should be self-explanatory
    srand((unsigned int)time(NULL)); // seed our PRNG. This should normally go at the top of main(), but I have my reasons...
    return (drand48() * (max - min + 1)) + min;
}

void *ReaderThread (void *vargp) {
    __u_long pthread_id = *((__u_long*)vargp); // get our id once when we init
    for (unsigned i = 1; i < NUMBER_OF_READS + 1; i++) { // as defined above
        usleep(GetFromUniformIntegerDistribution(1, 10000)); // wait some time before acting
        pthread_mutex_lock(&counter_mutex); // acquire the mutex
        readers_waiting++;
        while (resource_counter == -1) pthread_cond_wait(&read_phase, &counter_mutex); // wait for a read_phase
        resource_counter++; // indicate that we are about to begin reading the shared_variable
        readers_waiting--;
        pthread_mutex_unlock(&counter_mutex); // release the mutex

        usleep(GetFromUniformIntegerDistribution(1, 1000)); // fake work
        printf("%lu(i=%u) read: %lu, resource_counter: %i\n", pthread_id, i, shared_variable, resource_counter);
        
        pthread_mutex_lock(&counter_mutex); // acquire the mutex
        resource_counter--; // indicate that we are no longer reading
        if (resource_counter == 0) pthread_cond_signal(&write_phase); // if no other readers are reading, signal to a waiting write thread that it may unblock and execute
        pthread_mutex_unlock(&counter_mutex); // release the mutex
    }
    return NULL;
}

void *WriterThread(void *vargp) {
    __u_long pthread_id = pthread_self(); // take your pick--the former is probably faster though?
    for (unsigned i = 1; i < NUMBER_OF_WRITES + 1; i++) { // as defined above
        usleep(GetFromUniformIntegerDistribution(1, 1000)); // wait some time before acting
        pthread_mutex_lock(&counter_mutex); // acquire the mutex
        while (resource_counter != 0) pthread_cond_wait(&write_phase, &counter_mutex); // wait for a write_phase
        resource_counter = -1; // indicate that we are about to write to the shared_variable
        pthread_mutex_unlock(&counter_mutex); // release the mutex

        usleep(GetFromUniformIntegerDistribution(1, 100)); // fake work
        shared_variable = pthread_id; // do the thing.
        printf("%lu(i=%u) WRITE: %lu, resource_counter: %i\n", pthread_id, i, shared_variable, resource_counter);
        
        pthread_mutex_lock(&counter_mutex); // acquire the mutex
        resource_counter = 0; // indicates that no one is currently reading or writing
        pthread_mutex_unlock(&counter_mutex); // release the mutex

        pthread_cond_broadcast(&read_phase); // broadcast to all the waiting read threads that they may unblock and execute
        pthread_cond_signal(&write_phase); // signal to a waiting write thread that it may unblock and execute
    }
    return NULL;
}

int main (void) {
    pthread_t reader_id[NUMBER_OF_READERS], writer_id[NUMBER_OF_WRITERS]; // implicit init
    int error;
    for (int i = 0; i < NUMBER_OF_READERS; i++) { // create our reader threads
        error = pthread_create(&reader_id[i], NULL , &ReaderThread, &reader_id[i]);
        if (error) { printf("\nThread can't be created :[%s]\n", strerror(error)); }
    }
    for (int i = 0; i < NUMBER_OF_WRITERS; i++) { // create our writer threads
        error = pthread_create(&writer_id[i], NULL , &WriterThread, &writer_id[i]); // why can I use the address of WriterThread!?
        if (error) { printf("\nThread can't be created :[%s]\n", strerror(error)); }
    }
    for (int i = 0; i < NUMBER_OF_READERS; i++) { pthread_join(reader_id[i], NULL); } // join all of our threads back to the main.
    for (int i = 0; i < NUMBER_OF_WRITERS; i++) { pthread_join(writer_id[i], NULL); }
    pthread_mutex_destroy(&counter_mutex); // probably not strictly necessary since all the memory is going to be freed up in the next few microseconds...
    return 0;
}
