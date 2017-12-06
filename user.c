#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#define THRESHOLD 10
#define BOUND 500

struct timer
{
	unsigned int seconds;
	unsigned int ns;
};

struct page
{
	unsigned int procID;
	unsigned int valid;
	unsigned int dirty;
	unsigned int readWrite;
};

int errno;
int myIndex;
pid_t pid;
char errmsg[200];
struct timer *shmTime;
struct page *shmPage;
int *shmChild;
int *shmTerm;
sem_t * semDead;
sem_t * semTerm;
sem_t * semChild;
sem_t * semPage;

/* Insert other shmid values here */

void sigIntHandler(int signum)
{
	int i;
	
	snprintf(errmsg, sizeof(errmsg), "USER %d: Caught SIGINT! Killing process #%d.", pid, myIndex);
	perror(errmsg);	
	
	sem_wait(semTerm);
	shmTerm[myIndex] = 1;
	shmTerm[19]++;
	sem_post(semTerm);
	
	sem_post(semDead);
	
	errno = shmdt(shmTime);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "USER %d: shmdt(shmTime)", pid);
		perror(errmsg);	
	}

	errno = shmdt(shmChild);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "USER %d: shmdt(shmChild)", pid);
		perror(errmsg);	
	}
	
	errno = shmdt(shmTerm);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "USER %d: shmdt(shmTerm)", pid);
		perror(errmsg);	
	}
	
	errno = shmdt(shmPage);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "USER %d: shmdt(shmPage)", pid);
		perror(errmsg);	
	}
	
	exit(signum);
}

int main (int argc, char *argv[]) {
int o;
int i;
int terminate = 0;
struct timer termTime;
struct timer reqlTime;
struct page request;
int timeKey = atoi(argv[1]);
int childKey = atoi(argv[2]);
int index = atoi(argv[3]);
myIndex = index;
int termKey = atoi(argv[4]);
int pageKey = atoi(argv[5]);
key_t keyTime = 8675;
key_t keyChild = 5309;
key_t keyTerm = 1138;
key_t keyPage = 8311;
signal(SIGINT, sigIntHandler);
pid = getpid();
int usedPages = 0;
int numRequests = 0;
int nextTerm;

/* Seed random number generator */
srand(pid * time(NULL));

snprintf(errmsg, sizeof(errmsg), "USER %d: Slave process started!", pid);
perror(errmsg);

/********************MEMORY ATTACHMENT********************/
/* Point shmTime to shared memory */
shmTime = shmat(timeKey, NULL, 0);
if ((void *)shmTime == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "USER: shmat(shmidTime)");
	perror(errmsg);
    exit(1);
}

/* Point shmChild to shared memory */
shmChild = shmat(childKey, NULL, 0);
if ((void *)shmChild == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "USER: shmat(shmidChild)");
	perror(errmsg);
    exit(1);
}

/* Point shmTerm to shared memory */
shmTerm = shmat(termKey, NULL, 0);
if ((void *)shmTerm == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "USER: shmat(shmidTerm)");
	perror(errmsg);
    exit(1);
}

/* Point shmPage to shared memory */
shmPage = shmat(pageKey, NULL, 0);
if ((void *)shmPage == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "USER: shmat(shmidPage)");
	perror(errmsg);
    exit(1);
}

/********************END ATTACHMENT********************/

/********************SEMAPHORE CREATION********************/
/* Open Semaphore */
semDead=sem_open("semDead", 1);
if(semDead == SEM_FAILED) {
	snprintf(errmsg, sizeof(errmsg), "USER %d: sem_open(semDead)...", pid);
	perror(errmsg);
    exit(1);
}

semTerm=sem_open("semTerm", 1);
if(semTerm == SEM_FAILED) {
	snprintf(errmsg, sizeof(errmsg), "USER %d: sem_open(semTerm)...", pid);
	perror(errmsg);
    exit(1);
}

semChild=sem_open("semChild", 1);
if(semTerm == SEM_FAILED) {
	snprintf(errmsg, sizeof(errmsg), "USER %d: sem_open(semChild)...", pid);
	perror(errmsg);
    exit(1);
}

semPage=sem_open("semPage", 1);
if(semPage == SEM_FAILED) {
	snprintf(errmsg, sizeof(errmsg), "USER %d: sem_open(semPage)...", pid);
	perror(errmsg);
    exit(1);
}
/********************END SEMAPHORE CREATION********************/

/* Calculate First Request/Release Time */
/* reqlTime.ns = shmTime->ns + rand()%(BOUND);
reqlTime.seconds = shmTime->seconds;
if (reqlTime.ns > 1000000000)
{
	reqlTime.ns -= 1000000000;
	reqlTime.seconds += 1;
} */

request.procID = myIndex;

while(!terminate)
{
	request.readWrite = (rand()%2)+1;
	/* Write Request == 1 */
		/* Notify oss of request then wait until granted */
		/* Granted requests refresh valid bit and change shmPage[myIndex] values (done by oss) */

	/* Read Request == 2 */
		/* Notify oss of request then wait until granted */
		/* Granted requests refresh the valid bit */
		
	/* Increment Request Count */
	/* Test if Request count is large enough to terminate */
		/* If so, set terminate == 1 */	
}

/* signal the release the process from the running processes */
sem_wait(semTerm);
shmTerm[index] = 1;
sem_post(semTerm);
snprintf(errmsg, sizeof(errmsg), "USER %d: Slave process terminating!", pid);
perror(errmsg);
sem_post(semDead);

/********************MEMORY DETACHMENT********************/
errno = shmdt(shmTime);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "USER: shmdt(shmTime)");
	perror(errmsg);	
}

errno = shmdt(shmChild);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "USER: shmdt(shmChild)");
	perror(errmsg);	
}

errno = shmdt(shmTerm);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "USER: shmdt(shmTerm)");
	perror(errmsg);	
}

errno = shmdt(shmPage);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "USER: shmdt(shmPage)");
	perror(errmsg);	
}

/********************END DETACHMENT********************/
exit(0);
return 0;
}
