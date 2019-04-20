/*******************************************/
/*	Name: IOS - Projekt 2 VUT FIT		   */
/*  Project based on synchronization	   */
/*	Author: Tomáš Sasák					   */
/*******************************************/

#include "proj2.h"

#define SEMAPHORE1_NAME "/xsasak01-writing"
#define SEMAPHORE2_NAME "/xsasak01-busStop"
#define SEMAPHORE3_NAME "/xsasak01-busWaiting"
#define SEMAPHORE4_NAME "/xsasak01-busBoarded"
#define SEMAPHORE5_NAME "/xsasak01-traveling"
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define MUTEX 1
#define LOCKED 0
#define ERROR -1
#define INTERVAL_START 0
#define INTERVAL_MAX 1000
/**
 * Function takes arguments from program and checks if they are correct, function returns the arguments in structure.
 */

// Semaphores
sem_t *writing = NULL;
sem_t *busStop = NULL;
sem_t *busWaiting = NULL;
sem_t *busBoarded = NULL;
sem_t *traveling = NULL;

// Pointer to the file proj2.out
FILE *proj2out = NULL;

// Shared memory
shared *status = NULL;

// Statuses for processes to finish
int busStatus = 0;
int ridersStatus = 0;
int parentRiderStatus = 0;

pid_t bus;

/*
 * Function which takes arguments, checks if they are correct and return structure with these arguments.
 */
args_t get_args(int argc,char *argv[])
{
	if (argc != 5)
	{
		fprintf(stderr,"Wrong input, you must run the program with 4 arguments, in this order: Riders, Capacity, Timespawning Riders, Timeriding Bus.\n");
		exit(EXIT_FAILURE);
	}

	args_t args;
	args.R = strtol(argv[1],NULL,10);
	args.C = strtol(argv[2],NULL,10);
	args.ART = strtol(argv[3],NULL,10);
	args.ABT = strtol(argv[4],NULL,10);

	if (!(args.R > 0))
	{
		fprintf(stderr,"Wrong input, number of riders must be > 0\n");
		exit(EXIT_FAILURE);
	}
	else if(!(args.C > 0))
	{
		fprintf(stderr,"Wrong input, number of capacity must be > 0\n");
		exit(EXIT_FAILURE);
	}
	else if(!((args.ART >= 0) && (args.ART <= 1000)))
	{
		fprintf(stderr,"Wrong input, time of generating riders must be >= 0 && <= 1000\n");
		exit(EXIT_FAILURE);
	}
	else if(!((args.ABT >= 0) && (args.ABT <= 1000)))
	{
		fprintf(stderr,"Wrong input, time of bus ride must be >= 0 && <= 1000\n");
		exit(EXIT_FAILURE);
	}

	return args;
}

/*
 * Function initialize the resource needed for program to run correctly.
 */
void prog_init()
{
	//Trying to unlink first the semaphores, if program crashed before.
	sem_unlink(SEMAPHORE1_NAME);
	sem_unlink(SEMAPHORE2_NAME);
	sem_unlink(SEMAPHORE3_NAME);
	sem_unlink(SEMAPHORE4_NAME);
	sem_unlink(SEMAPHORE5_NAME);

	// Opening/creating file for output.
	proj2out = fopen("proj2.out","w");
	if(proj2out == NULL)
	{
		fprintf(stderr,"Cannot create/open proj2.out file! \n");
		exit(EXIT_FAILURE);
	}

	// Creating all semaphores needed.
	if ((writing = sem_open(SEMAPHORE1_NAME, O_CREAT | O_EXCL, 0666, MUTEX)) == SEM_FAILED)
	{
		fprintf(stderr,"Cannot create semaphore!\n %s\n",strerror(errno));
		exit(EXIT_FAILURE);
	}
	if ((busStop = sem_open(SEMAPHORE2_NAME, O_CREAT | O_EXCL, 0666, MUTEX)) == SEM_FAILED)
	{
		fprintf(stderr,"Cannot create semaphore!\n");
		exit(EXIT_FAILURE);
	}
	if ((busWaiting = sem_open(SEMAPHORE3_NAME, O_CREAT | O_EXCL, 0666, LOCKED)) == SEM_FAILED)
	{
		fprintf(stderr,"Cannot create semaphore!\n");
		exit(EXIT_FAILURE);
	}

	if ((busBoarded = sem_open(SEMAPHORE4_NAME, O_CREAT | O_EXCL, 0666, LOCKED)) == SEM_FAILED)
	{
		fprintf(stderr,"Cannot create semaphore!\n");
		exit(EXIT_FAILURE);
	}
	if ((traveling = sem_open(SEMAPHORE5_NAME, O_CREAT | O_EXCL, 0666, MUTEX)) == SEM_FAILED)
	{
		fprintf(stderr,"Cannot create semaphore!\n");
		exit(EXIT_FAILURE);
	}

	// Creating shared memory.
	status = create_mmap(sizeof(shared));
	if(status == MAP_FAILED)
	{
		fprintf(stderr,"Cannot create shared memory!\n");
		exit(EXIT_FAILURE);
	}
}

/**
 * Function which creates shared memory based on the structure "shared".
 */ 
shared *create_mmap(size_t size)
{
	return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0);
}


/**
 * Function generates random number in range param:from to param:to. Function needs seed to seed the generator.
 */
int rand_num(int from, int to,int seed)
{
	srand(time(0) + seed);
	return from + rand() / (RAND_MAX / (to - from + 1) + 1);
}

/**
 * Main function which creates the riders, which are slept first and then sent to the bus stop.
 */
void riders_gen(int count,int ridersWait) 
{
	for (int i = 1; i<=count; i++)
	{
		pid_t rider = fork();

		if (rider == 0)
		{
			usleep((rand_num(INTERVAL_START,ridersWait,i))); // Spawning process in random time from interval

			sem_wait(writing);
			// Critical section
			status->line++;
			fprintf(proj2out,"%d 		: RID %d 		: start\n",status->line,i);
			fflush(proj2out);
			// End of critical section
			sem_post(writing);

			go_on_busstop(i); // Rider is supposed to go on busstop

			sem_wait(busWaiting); // Rider got on busstop, now he needs to wait for bus to come

			get_in_bus(i); // Bus came, rider needs to get in.

			sem_post(busBoarded); // Rider got in, freeing semaphore so other rider can join the bus

			sem_wait(traveling); // Rider is in the bus and is traveling to destination

			sem_wait(writing);
			// Critical section
			status->line++;
			fprintf(proj2out,"%d 		: RID %d 		: finish\n",status->line,i);
			fflush(proj2out);
			// End of critical section
			sem_post(writing);

			sem_post(traveling); // Rider is getting out of the bus

			exit (EXIT_SUCCESS);
		}
		else if(rider < 0)
		{
			printf("Cannot create more process RIDER!\n");
			kill(bus,SIGKILL);
			prog_fin();
			exit(EXIT_FAILURE);
		}
	}
}

/*
 * Function represents simulation of process RIDER going on bus stop.
 */
void go_on_busstop(int id)
{
	sem_wait(busStop); // Rider wants to go on bus stop, if bus is currently on stop, rider gets blocked and need to wait
	// Critical section - Bus is not on stop, rider is on stop

	status->ridersOnBStop++;  // Incrementing count of riders on stop

	sem_wait(writing);
	// Critical section
	status->line++;
	fprintf(proj2out,"%d 		: RID %d 		: enter: %d\n",status->line,id,status->ridersOnBStop);
	fflush(proj2out);
	// End of critical section
	sem_post(writing);

	// End of critical section - riders count on bus is incremented and process now needs to wait to get on bus
	sem_post(busStop);

}

/*
 * Function simulates getting on bus by process RIDER
 */
void get_in_bus(int id)
{
	sem_wait(writing);
	// Critical section
	status->line++;
	fprintf(proj2out,"%d 		: RID %d 		: boarding\n",status->line,id);
	fflush(proj2out);
	// End of critical section
	sem_post(writing);

	status->ridersOnBStop--; // Decremeting number of processes on bus stop.
	status->ridersInBus++; // Incrementing number of processes inside bus.
}

/*
 * Function simulates process BUS going on bus stop.
 */
void go_busstop(int capacity)
{
	sem_wait(traveling); // Bus waits for all riders to get out from the previous ride
	sem_wait(busStop);
	// Critical section - bus locks the bus stop and prevents riders to get on station

	sem_wait(writing);
	// Critical section
	status->line++;
	fprintf(proj2out,"%d 		: BUS 		: arrival\n",status->line);
	fflush(proj2out); 
	//End of critical section
	sem_post(writing);

	int cntRiders = 0; // Getting the number of riders how many will board on the bus.
	if(status->ridersOnBStop > capacity)
	{
		cntRiders = capacity; // If there is more riders than capacity of the bus, number of capacity will be how much riders will come to the bus.
	}
	else
	{
		cntRiders = status->ridersOnBStop; // Else, the bus will take all the people on busstop.
	}

	if(cntRiders == 0) // If there is noone on the busstop, bus will depart.
	{
		get_away_from_stop();

		return;
	}

	sem_wait(writing);
	// Critical section
	status->line++;
	fprintf(proj2out,"%d 		: BUS 		: start boarding: %d \n",status->line,status->ridersOnBStop);
	fflush(proj2out);
	// End of critical section
	sem_post(writing);

	start_boarding(cntRiders); // Start boarding the RIDERS.
	
	sem_wait(writing);
	// Critical section
	status->line++;
	fprintf(proj2out,"%d 		: BUS 		: end boarding: %d \n",status->line,status->ridersOnBStop);
	fflush(proj2out);
	// End of critical section
	sem_post(writing);

	sem_wait(writing);
	// Critical section
	status->line++;
	fprintf(proj2out,"%d 		: BUS 		: depart\n",status->line);
	fflush(proj2out); 
	//End of critical section
	sem_post(writing);


	// End of critical section - bus is going away from bus stop, unlocking bus stop
	sem_post(busStop);
}

/* 
 * Function simulates the exit of process BUS from bus station when noone is waiting.
 */
void get_away_from_stop()
{
		sem_wait(writing);
		// Critical section
		status->line++;
		fprintf(proj2out,"%d 		: BUS 		: depart\n",status->line);
		fflush(proj2out); 
		//End of critical section
		sem_post(writing);

		sem_post(busStop); // Bus is unlocking the busstop, moving away from the busstop.
}

/*
 * Function simulates the boarding situation of process BUS
 */
void start_boarding(int cntRiders)
{
	for(int i = 1; i <= cntRiders; i++) // Bus is going to signal all the eligable riders, to get in one by one, and will wait to get them in one by one.
	{
		sem_post(busWaiting); // Bus is here, signaling riders one by one to get in
		sem_wait(busBoarded); // Waiting on signal that one rider got inside the bus, and bus can proceed to take another rider.
	}
}

/*
 * Function clears out the used semaphores, memory and closes the file.
 */
void prog_fin()
{
	int result = munmap(status, sizeof(shared));

	if(result == ERROR)
	{
		fprintf(stderr,"Cannot destroy shared memory!\n");
		exit(EXIT_FAILURE);
	}

	if((sem_close(writing)) == ERROR)
	{
		fprintf(stderr,"Cannot close semaphore!\n");
		exit(EXIT_FAILURE);
	}
	if((sem_close(busStop)) == ERROR)
	{
		fprintf(stderr,"Cannot close semaphore!\n");
		exit(EXIT_FAILURE);
	}
	if((sem_close(busWaiting)) == ERROR)
	{
		fprintf(stderr,"Cannot close semaphore!\n");
		exit(EXIT_FAILURE);
	}
	if((sem_close(busBoarded)) == ERROR)
	{
		fprintf(stderr,"Cannot close semaphore!\n");
		exit(EXIT_FAILURE);
	}
	if((sem_close(traveling)) == ERROR)
	{
		fprintf(stderr,"Cannot close semaphore!\n");
		exit(EXIT_FAILURE);
	}


	if((sem_unlink(SEMAPHORE1_NAME)) == ERROR)
	{
		fprintf(stderr,"Cannot unlink semaphore!\n");
		exit(EXIT_FAILURE);
	}
	if((sem_unlink(SEMAPHORE2_NAME)) == ERROR)
	{
		fprintf(stderr,"Cannot unlink semaphore!\n");
		exit(EXIT_FAILURE);
	}
	if((sem_unlink(SEMAPHORE3_NAME)) == ERROR)
	{
		fprintf(stderr,"Cannot unlink semaphore!\n");
		exit(EXIT_FAILURE);
	}
	if((sem_unlink(SEMAPHORE4_NAME)) == ERROR)
	{
		fprintf(stderr,"Cannot unlink semaphore!\n");
		exit(EXIT_FAILURE);
	}
	if((sem_unlink(SEMAPHORE5_NAME)) == ERROR)
	{
		fprintf(stderr,"Cannot unlink semaphore!\n");
		exit(EXIT_FAILURE);
	}

	int test = fclose(proj2out);

	if(test == EOF)
	{
		fprintf(stderr,"Cannot close file!\n");
		exit(EXIT_FAILURE);
	}
}

int main (int argc, char *argv[])
{
	args_t args = get_args(argc,&(*argv));
	prog_init();
	
	pid_t mainProcess;
	bus = fork(); // Creation of process Bus

	if(bus == 0) // This is bus
	{
		sem_wait(writing);
		// Critical section
		status->line++;
		fprintf(proj2out,"%d 		: BUS 		: start\n",status->line);
		fflush(proj2out); 
		// End of critical section
		sem_post(writing);

		while(status->ridersDelivered != args.R)
		{
			// Bus is going to the bus stop
			go_busstop(args.C);
			// Bus is going from bus stop

			usleep(rand_num(INTERVAL_START,	args.ABT,status->line)); // Sending status->line as random number for seed to generate random number

			sem_post(traveling); // Bus is kicking all the riders out

			status->ridersDelivered += status->ridersInBus; // Bus delivered all the riders, saving the number of them in ridersDelivered
			status->ridersInBus = 0; // Bus is zeroing the current number of riders inside bus

			sem_wait(writing);
			// Critical section
			status->line++;
			fprintf(proj2out,"%d 		: BUS 		: end\n",status->line);
			fflush(proj2out); 
			// End of critical section
			sem_post(writing);

		}

		sem_wait(writing);
		// Critical section
		status->line++;
		fprintf(proj2out,"%d 		: BUS 		: finish\n",status->line);
		fflush(proj2out); 
		//End of critical section
		sem_post(writing);

		exit(EXIT_SUCCESS);
	}

	else if(bus > 0) // This is main process
	{
		pid_t parentRider = fork(); // Creation of parentRider, process which will creater riders.

		if(parentRider == 0) // This is parentRider
		{
			riders_gen(args.R,args.ART);//args.ABT);
			while ((parentRider = wait(&ridersStatus)) > 0);
			exit(EXIT_SUCCESS);
		}

		else if(parentRider > 0) // This is main process, which needs to wait on parentRider and bus to finish.
		{
			while ((mainProcess = wait(&busStatus)) > 0 && (mainProcess = wait(&parentRiderStatus)) > 0);
			prog_fin();
		}
		else if (parentRider < 0) // Cannot create process.
		{
			prog_fin();
			fprintf(stderr,"Cannot create another process!\n");
			exit(EXIT_FAILURE);
		}
	}

	else if(bus < 0) // Cannot create process.
	{
		prog_fin();
		fprintf(stderr,"Cannot create another process!\n");
		exit(EXIT_FAILURE);
	}

	return 0; 
}