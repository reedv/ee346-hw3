/*
 * This program generates a sequence of readers and writers every
 * "time unit", which is defined to be a quarter-second.
 *
 * We refer to a reader or writer as a "player".
 *
 * The players enter and exit a critical section.
 * The critical section involves a global int variable called "critical".
 * This is simply a counter.
 *
 *
 * Readers will read "critical" and print it.
 * Writers will increment "critical" and then print it.
 * Readers and writers will stay in the critical section for a
 * certain duration of time in time units.
 * For example,
 *
 *    A reader may be created at time 0 and have duration 2
 *    A writer may be created at time 1 and have duration 3
 *    A reader may be created at time 2 and have duration 2
 *    and so on
 *
 * This sequence of readers and writers and their duration is uploaded
 * from a file. The file, called the data file, has the following format:
 *
 * The first line is the total amount of readers and writers
 * The second line is the first reader/writer to be created
 * The third line is the second reader/writer to be created
 * etc
 *
 * The second line onwards has the following format
 *
 * A character (R or W) indicating reader or writer <space(s)> time_duration
 *
 * The following is an example:
 *
 * 5
 * R  3
 * R  4
 * W  3
 * R  1
 * W  4
 *
 * Here we have 5 players.  The first is a reader with duration 3,
 * while the fifth is a writer with duration 4.
 *
 * This program will simulate the players as they are created and go 
 * through the critical section.  There is a label SEMAPHORE
 * that determines if there is mutual exclusion, i.e., only one
 * player can be in the critical section at a time.  If SEMAPHORE
 * isn't there then there is no restrictions on the number of
 * players in the critical section.
 *
 * There is also a time2 variable to keep track of time in time units.
 * It increments after teach time unit but stops after MAXTIME.
 *
 * You can compile by "gcc readw.c -pthread"
 */

// conditional compile directives
//#define SEMAPHORE  // Flag to enable mutual exclusion
//#define READER  // Flag to enable reader priority
#define WRITER  // Flag to enable writer priority

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAXTIME    40  /* Max number of ticks of the timer */
#define MAXPLAYERS 100 /* No more than 100 readers/writers */


struct player_info {
   char ptype;
   int duration;
}; 

struct thread_arguments {
   int duration;
   int id;
};

void * reader(void *argument);
void * writer(void *argument);
void * clock2(void *argument);
void delay(int units); 

#ifdef SEMAPHORE
pthread_mutex_t mutexsem;  
#endif

#ifdef READER
pthread_mutex_t mutexsem,  // control reader access
 	 	 	    resource;  // control writer access
int readcount = 0;
#endif

#ifdef WRITER
int readcount = 0,
	writecount = 0;
pthread_mutex_t rmutex,
				wmutex,
				readTry,
				resource;
#endif

int critical = 0;
int time2 = 0;

int main(int argc, char * argv[])
{

	FILE * pFile;

	int numplayers;
	struct player_info player[MAXPLAYERS];
	int duration;

	pthread_t threads[MAXPLAYERS];
	pthread_attr_t attr[MAXPLAYERS];
	struct thread_arguments threadarg[MAXPLAYERS];
	pthread_t threadclock;
	pthread_attr_t attrclock;

	int mtime;
	char cstring[100];
	int i;

	/*
	 * Load player stats
	 */
	pFile = fopen(argv[1],"r");
	fscanf(pFile, "%d", &numplayers);
	for (i=0; i<numplayers; i++) {
	   fscanf(pFile, "%s %d", cstring, &duration);
	   player[i].ptype = cstring[0];
	   player[i].duration = duration;
	}

	printf("\n");  /* Print out stats to verify */
	printf("Data that was loaded\n");
	printf("Number of players = %d\n",numplayers);
	for (i=0; i<numplayers; i++) {
	   printf("Player %d: type=%c duration=%d\n",i,player[i].ptype,player[i].duration);
	}

	/*
	 * Start clock
	 */

	mtime = MAXTIME;
	pthread_attr_init(&attrclock);
	pthread_create(&threadclock,&attrclock,clock2, (void *) &mtime);

	/*
	 * Create player threads
	 */
	printf("\nPlayers are created\n");

	for (i=0; i<numplayers; i++) {
	   threadarg[i].id = i;
	   threadarg[i].duration = player[i].duration;
	   pthread_attr_init(&attr[i]);

	   if (player[i].ptype == 'R') {
		  pthread_create(&threads[i],&attr[i],reader,(void *) &threadarg[i]);
	   }
	   else if (player[i].ptype == 'W') {
		  pthread_create(&threads[i],&attr[i],writer,(void *) &threadarg[i]);
	   }

	   delay(1);
	}

	/*
	 * Wait for all threads to complete
	 */
	for (i=0; i<numplayers; i++) {
	   pthread_join(threads[i], NULL);
	}
	pthread_join(threadclock,NULL);

	exit(EXIT_SUCCESS);

}

/*
 * The reader
 */
void * reader(void *arg)
{
	struct thread_arguments * targ;

	targ = arg;

	printf("** Reader %d is created, time=%d\n", targ->id,time2);

	#ifdef SEMAPHORE
	// The mutex object referenced shall be locked by calling pthread_mutex_lock().
	//   If the mutex is already locked, the calling thread shall block until the mutex becomes available.
	pthread_mutex_lock (&mutexsem);
	#endif

#ifdef READER
	pthread_mutex_lock(&mutexsem);
	/* ------ begin critical section */
	readcount++;  // Indicate that you are a reader trying to enter the Critical Section
	if (readcount == 1){  // Checks if you are the first reader trying to enter CS
		// If you are the first reader, lock the resource from writers.
		// Resource stays reserved for subsequent readers
		pthread_mutex_lock(&resource);
		printf("LOCK: resource locked to writers by reader %d\n", targ->id);
	}
	/* ------ end critical section */
	pthread_mutex_unlock(&mutexsem);
#endif

#ifdef WRITER
	/* ------ entry section */
	pthread_mutex_lock(&readTry);  // indicate a reader is trying to enter
	pthread_mutex_lock(&rmutex);  // lock entry section to avoid race with other readers

	// report self as reader
	readcount++;
	// if this is first reader, lock the shared resource from writers
	if(readcount == 1) {
		pthread_mutex_lock(&resource);
	}

	pthread_mutex_unlock(&rmutex);  // release entry section to other readers
	pthread_mutex_unlock(&readTry);  // indicate done trying to access readcount
#endif

	/* ------ begin critical section (SEMAPHORE and WRITER case) */
	printf("-> Reader %d enters critical section, duration=%d, time=%d\n",targ->id,targ->duration,time2);
	delay(targ->duration);
	printf("     <- Reader %d exits critical section, critical = %d, time=%d\n",targ->id, critical,time2);
	/* ----- end critical section */

	#ifdef SEMAPHORE
	pthread_mutex_unlock (&mutexsem);
	#endif

#ifdef READER
	pthread_mutex_lock(&mutexsem);
	/* ------ begin critical section */
	readcount--;  // Indicate that you are no longer needing the shared resource. One less readers
	if (readcount == 0){  // Checks if you are the last (only) reader who is reading the shared file
		// If you are last reader, then you can unlock the resource.
	    // This makes it available to writers.
		pthread_mutex_unlock(&resource);
		printf("UNLOCK: resource unlocked to writers by reader %d\n", targ->id);
	}
	/* ----- end critical section */
	pthread_mutex_unlock(&mutexsem);
#endif

#ifdef WRITER
	/* ------ exit section */
	pthread_mutex_lock(&rmutex);  // reserve exit section to avoid race with other readers

	// indicate that this reader is leaving
	readcount--;
	// if this is last reader leaving, unlock the resource to writers
	if(readcount == 0) {
		pthread_mutex_unlock(&resource);
	}

	pthread_mutex_unlock(&rmutex);  // release exit section to other readers
#endif
	pthread_exit(0);
}

/*
 * The writer
 */
void * writer(void *argument)
{
	struct thread_arguments * targ;

	targ = argument;

	printf("** Writer %d is created, time=%d\n", targ->id,time2);

	#ifdef SEMAPHORE
	pthread_mutex_lock (&mutexsem);
	#endif

#ifdef READER
	pthread_mutex_lock(&resource);
#endif

#ifdef WRITER
	pthread_mutex_lock(&wmutex);  // reserve entry section for writers to avoid race

	writecount++;
	if(writecount == 1){
		pthread_mutex_lock(&readTry);
	}

	pthread_mutex_unlock(&wmutex);


	pthread_mutex_lock(&resource);  // reserve resource for this writer only
#endif

	/* ------ begin critical section */
	printf("-> Writer %d enters critical section, duration=%d, time=%d\n",targ->id,targ->duration,time2);
	critical++;
	delay(targ->duration);
	printf("     <- Writer %d exits critical section, critical = %d, time=%d\n",targ->id, critical,time2);
	/* ----- end critical section */

	#ifdef SEMAPHORE
	pthread_mutex_unlock (&mutexsem);
	#endif

#ifdef READER
	pthread_mutex_unlock(&resource);
#endif

#ifdef WRITER
	pthread_mutex_unlock(&resource);  // release shared resource for other writers

	/* ------ exit section */
	pthread_mutex_lock(&wmutex);  // reserve exit section

	// indicate this writer is leaving
	writecount--;
	// if this is last writer, unlock the shared resource to the readers
	if(writecount == 0){
		pthread_mutex_unlock(&readTry);
	}

	pthread_mutex_unlock(&wmutex);  // release exit section to other writers
#endif

	pthread_exit(0);
}

/*
 * Clock to increment time variable
 */

void * clock2(void *argument)
{
	int i;
	int * mtimeptr;
	int mtime;

	mtimeptr = argument;
	mtime = * mtimeptr;
	for (i=0; i<mtime; i++) {
	   delay(1);
	   time2++;
	}
	pthread_exit(0);
}

/*
 * Delays by the number of time units by going to sleep. Each time
 * unit is a quarter-second.
 */
void delay(int units)
{
	int i;
	for (i=0; i<units; i++) usleep(250000);
}





