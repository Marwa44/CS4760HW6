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
#define MAXPAGES 256
#define MAXSLAVES 18
#define SYSSLAVES 12

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
char errmsg[200];
FILE *fp;
int shmidTime;
int shmidChild;
int shmidTerm;
int shmidPage;
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
	/* Send a message to stderr */
	snprintf(errmsg, sizeof(errmsg), "OSS: Caught SIGINT! Killing all child processes.");
	perror(errmsg);	
	
	/* Deallocate shared memory */
	errno = shmdt(shmTime);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmTime)");
		perror(errmsg);	
	}
	
	errno = shmctl(shmidTime, IPC_RMID, NULL);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidTime)");
		perror(errmsg);	
	}
	
	errno = shmdt(shmChild);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmChild)");
		perror(errmsg);	
	}
	
	errno = shmctl(shmidChild, IPC_RMID, NULL);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidChild)");
		perror(errmsg);	
	}
	
	errno = shmdt(shmTerm);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmTerm)");
		perror(errmsg);	
	}
	
	errno = shmctl(shmidTerm, IPC_RMID, NULL);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidTerm)");
		perror(errmsg);	
	}
	
	errno = shmdt(shmPage);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmPage)");
		perror(errmsg);	
	}
	
	errno = shmctl(shmidPage, IPC_RMID, NULL);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidPage)");
		perror(errmsg);	
	}
	
	/* Close Semaphore */
	sem_unlink("semDead");   
    sem_close(semDead);
	sem_unlink("semTerm");
	sem_close(semTerm);
	sem_unlink("semChild");
	sem_close(semChild);
	sem_unlink("semPage");
	sem_close(semPage);
	/* Close Log File */
	fclose(fp);
	/* Exit program */
	exit(signum);
}

int main (int argc, char *argv[]) {
int o;
int i;
int j;
int k;
int m;
int children[18] = {0}; 
int maxSlaves = 1;
int numSlaves = 0;
int numProc = 0;
int maxTime = 10;
char *sParam = NULL;
char *lParam = NULL;
char *tParam = NULL;
char timeArg[33];
char childArg[33];
char indexArg[33];
char termArg[33];
char pageArg[33];
char buf[200];
pid_t pid = getpid();
key_t keyTime = 8675;
key_t keyChild = 5309;
key_t keyTerm = 1138;
key_t keyPage = 8311;
char *fileName = "./msglog.out";
int verbose = 0;
signal(SIGINT, sigIntHandler);
time_t start;
time_t stop;
struct timer nextProc = {0};
int numShared = 0;
int cycles = 0;
int pKill = -1; 
int usedPages = 0;

/* Seed RNG */
srand(pid * time(NULL));

/* Options */
while ((o = getopt (argc, argv, "hs:l:t:v")) != -1)
{
	switch (o)
	{
		case 'h':
			snprintf(errmsg, sizeof(errmsg), "oss.c simulates a primitive operating system that spawns processes and logs resource requests, releases, and deadlocks at various times to a file using a simulated clock.\n\n");
			printf(errmsg);
			snprintf(errmsg, sizeof(errmsg), "oss.c options:\n\n-h\tHelp option: displays options and their usage for oss.c.\nUsage:\t ./oss -h\n\n");
			printf(errmsg);
			snprintf(errmsg, sizeof(errmsg), "-s\tSlave option: this option sets the number of slave processes from 1-19 (default 5).\nUsage:\t ./oss -s 10\n\n");
			printf(errmsg);
			snprintf(errmsg, sizeof(errmsg), "-l\tLogfile option: this option changes the name of the logfile to the chosen parameter (default msglog.out).\nUsage:\t ./oss -l output.txt\n\n");
			printf(errmsg);
			snprintf(errmsg, sizeof(errmsg), "-t\tTimeout option: this option sets the maximum run time allowed by the program in seconds before terminating (default 20).\nUsage:\t ./oss -t 5\n");
			printf(errmsg);
			snprintf(errmsg, sizeof(errmsg), "-v\tVerbose option: this option affects what information is saved in the log file.\nUsage:\t ./oss -v\n");
			printf(errmsg);
			exit(1);
			break;
		case 's':
			sParam = optarg;
			break;
		case 'l':
			lParam = optarg;
			break;
		case 't':
			tParam = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
			if (optopt == 's' || optopt == 'l' || optopt == 't')
			{
				snprintf(errmsg, sizeof(errmsg), "OSS: Option -%c requires an argument.", optopt);
				perror(errmsg);
			}
			return 1;
		default:
			break;
	}	
}

/* Set maximum number of slave processes */
if(sParam != NULL)
{
	maxSlaves = atoi(sParam);
}
if(maxSlaves < 0)
{
	maxSlaves = 5;
}
if(maxSlaves > SYSSLAVES)
{
	maxSlaves = SYSSLAVES;
}

/* Set name of log file */
if(lParam != NULL)
{
	fp = fopen(lParam, "w");
	if(fp == NULL)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: fopen(lParam).");
		perror(errmsg);
	}
}
else
{
	fp = fopen(fileName, "w");
	if(fp == NULL)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: fopen(fileName).");
		perror(errmsg);
	}
}

/* Set maximum alloted run time in real time seconds */
if(tParam != NULL)
{
	maxTime = atoi(tParam);
}

/********************MEMORY ALLOCATION********************/
/* Create shared memory segment for a struct timer */
shmidTime = shmget(keyTime, sizeof(struct timer), IPC_CREAT | 0600);
if (shmidTime < 0)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmget(keyTime...)");
	perror(errmsg);
	exit(1);
}

/* Point shmTime to shared memory */
shmTime = shmat(shmidTime, NULL, 0);
if ((void *)shmTime == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmat(shmidTime)");
	perror(errmsg);
    exit(1);
}

/* Create shared memory segment for a child array */
shmidChild = shmget(keyChild, sizeof(int)*18, IPC_CREAT | 0600);
if (shmidChild < 0)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmget(keyChild...)");
	perror(errmsg);
	exit(1);
}

/* Point shmChild to shared memory */
shmChild = shmat(shmidChild, NULL, 0);
if ((void *)shmChild == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmat(shmidChild)");
	perror(errmsg);
    exit(1);
}

/* Create shared memory segment for a child termination status */
shmidTerm = shmget(keyTerm, sizeof(int)*19, IPC_CREAT | 0600);
if (shmidTerm < 0)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmget(keyTerm...)");
	perror(errmsg);
	exit(1);
}

/* Point shmTerm to shared memory */
shmTerm = shmat(shmidTerm, NULL, 0);
if ((void *)shmTerm == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmat(shmidTerm)");
	perror(errmsg);
    exit(1);
}

/* Create shared memory segment for a Page Table */
shmidPage = shmget(keyPage, sizeof(struct page)*256, IPC_CREAT | 0600);
if (shmidPage < 0)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmget(keyPage...)");
	perror(errmsg);
	exit(1);
}

/* Point shmPage to shared memory */
shmPage = shmat(shmidPage, NULL, 0);
if ((void *)shmPage == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmat(shmidPage)");
	perror(errmsg);
    exit(1);
}
/********************END ALLOCATION********************/

/********************INITIALIZATION********************/
/* Convert shmTime and shmChild keys into strings for EXEC parameters */
sprintf(timeArg, "%d", shmidTime);
sprintf(childArg, "%d", shmidChild);
sprintf(termArg, "%d", shmidTerm);
sprintf(pageArg, "%d", shmidPage);

/* Set the time to 00.00 */
shmTime->seconds = 0;
shmTime->ns = 0;

/* Set shmChild array indices to 0 */
for(i =0; i<maxSlaves; i++)
{
	shmChild[i] = 0;
	shmTerm[i] = 0;
}
/********************END INITIALIZATION********************/

/********************SEMAPHORE CREATION********************/
/* Open Semaphore */
semDead=sem_open("semDead", O_CREAT | O_EXCL, 0644, 0);
if(semDead == SEM_FAILED) {
	snprintf(errmsg, sizeof(errmsg), "OSS: sem_open(semDead)...");
	perror(errmsg);
    exit(1);
}

semTerm=sem_open("semTerm", O_CREAT | O_EXCL, 0644, 1);
if(semTerm == SEM_FAILED) {
	snprintf(errmsg, sizeof(errmsg), "OSS: sem_open(semTerm)...");
	perror(errmsg);
	exit(1);
}    
semChild=sem_open("semChild", O_CREAT | O_EXCL, 0644, 1);
if(semChild == SEM_FAILED) {
	snprintf(errmsg, sizeof(errmsg), "OSS: sem_open(semChild)...");
	perror(errmsg);
	exit(1);
}    
semPage=sem_open("semPage", O_CREAT | O_EXCL, 0644, 1);
if(semChild == SEM_FAILED) {
	snprintf(errmsg, sizeof(errmsg), "OSS: sem_open(semPage)...");
	perror(errmsg);
	exit(1);
}    
/********************END SEMAPHORE CREATION********************/

/* Schedule next process */
nextProc.seconds = 0;
nextProc.ns = rand()%500000001;

/* Start the timer */
start = time(NULL);


/**********************Main Program Loop**********************/
do
{
	/* Has enough time passed to spawn a child? */
	if(shmTime->seconds >= nextProc.seconds && shmTime->ns >= nextProc.ns)
	{
		/* Check shmChild for first unused process */
		sem_wait(semChild);
		for(i = 0; i < maxSlaves; i++)
		{
			
			if(shmChild[i] == 0)
			{
				sprintf(indexArg, "%d", i);
				break;
			}
			
		}
		sem_post(semChild);
		
		/* Check that number of currently running processes is under the limit (maxSlaves) */
		if(numSlaves < maxSlaves)
		{
			nextProc.ns += rand()%500000001;
			if(nextProc.ns >= 1000000000)
			{
				nextProc.seconds += 1;
				nextProc.ns -= 1000000000;
			}
			numSlaves += 1;
			numProc += 1;
			pid = fork();
			if(pid == 0)
			{
				pid = getpid();
				shmChild[i] = pid;
				execl("./user", "user", timeArg, childArg, indexArg, termArg, pageArg, (char*)0);
			}
		}
	}
	
	/* Check for memory requests */
		/* Iterate through child processes and compare requests to the page table */
		/* if the request can be granted, do so */
			/* post to the child's semaphore */
			/* increment timer by 10ns */
			/* log request information */
		/* if the request is denied due to a page fault*/
			/* suspend the process */
			/* increment the timer by 15ms */
			/* log the page fault */
	
	/* Check Page Table Capacity */
		/* if 25 or fewer pages remain unoccupied, run the daemon */
			/* Daemon */
				/* Using a pointer that retains its position, cycle through pages 0-255, marking valid == 1 */
				/* bits to 0, and stopping on the first valid == 0 bit page. */
				/* Remove the page from the table */
				/* If the pointer reaches page 255, loop around to page 0 (circular array - clock algorithm) */
	
	/* Check for terminating children */
	sem_wait(semTerm);
	for(i = 0; i < maxSlaves; i++)
	{
		
		if(shmTerm[i] == 1)
		{
			/* Release all memory from terminating process */
			/* Log any terminated processes */
			shmChild[i] = 0;
			numSlaves--;
			shmTerm[i] = 0;
		}
		
	}
	sem_post(semTerm);
	
	/* Update the clock */
	shmTime->ns += (rand()%10000) + 1;
	if(shmTime->ns >= 1000000000)
	{
		shmTime->ns -= 1000000000;
		shmTime->seconds += 1;
	}
	
	/* Check if another second has passed, then print and log a memory map (valid and dirty) */
	/* for all 256 pages in the table */
	
	cycles++;
	stop = time(NULL);
}while(stop-start < maxTime);
/********************End Main Program Loop********************/

sleep(1);
/* Kill all slave processes */
for(i = 0; i < maxSlaves; i++)
{
	if(shmChild[i] != 0)
	{
		printf("Killing process #%d, PID = %d\n", i, shmChild[i]);
		kill(shmChild[i], SIGINT);
		sem_wait(semDead);
		wait(shmChild[i]);
	}
	/*else
	{
		printf("Not killing process #%d, PID = %d\n", i, shmChild[i]);
	} */
}
sleep(1);
printf("Program complete!\n");

/********************DEALLOCATE MEMORY********************/
errno = shmdt(shmTime);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmTime)");
	perror(errmsg);	
}

errno = shmctl(shmidTime, IPC_RMID, NULL);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidTime)");
	perror(errmsg);	
}

errno = shmdt(shmChild);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmChild)");
	perror(errmsg);	
}

errno = shmctl(shmidChild, IPC_RMID, NULL);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidChild)");
	perror(errmsg);	
}

errno = shmdt(shmTerm);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmTerm)");
	perror(errmsg);	
}

errno = shmctl(shmidTerm, IPC_RMID, NULL);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidTerm)");
	perror(errmsg);	
}

errno = shmdt(shmPage);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmPage)");
	perror(errmsg);	
}

errno = shmctl(shmidPage, IPC_RMID, NULL);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidPage)");
	perror(errmsg);	
}
/********************END DEALLOCATION********************/

/* Close Semaphore */
sem_unlink("semDead");   
sem_close(semDead);
sem_unlink("semTerm");
sem_close(semTerm);
sem_unlink("semChild");
sem_close(semChild);
sem_unlink("semPage");
sem_close(semPage);
/* Close Log File */
fclose(fp);
return 0;
}
