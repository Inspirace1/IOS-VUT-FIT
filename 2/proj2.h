#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <string.h>
/**
 * Structure for parameters. *
 */
typedef struct {
	int R; // Number of riders
	int C; // Capacity of bus
	int ART; // Maximum time to generate new process rider. (1000ms max)
	int ABT; // Maximum time of bus driving riders. (1000ms max)
}args_t;

/**
 * Structure for processes to share.
 */
typedef struct {
	int ridersOnBStop;
	int ridersInBus;
	int ridersDelivered;
	int line;
}shared;

/** Function prototypes. */
args_t get_args(int argc,char *argv[]);
int rand_num(int from, int to,int seed);
void riders_gen(int count,int ridersWait);	 //int busSleep);
void prog_init();
void get_away_from_stop();
void start_boarding(int cntRiders);
void prog_fin();
shared *create_mmap(size_t size);
void go_on_busstop(int id);	
void get_in_bus(int id);
void go_busstop(int capacity);