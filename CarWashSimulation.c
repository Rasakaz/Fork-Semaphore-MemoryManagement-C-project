/*
	Niv Azenkot - 204702740
*/

#include <stdio.h> // I/O
#include <stdlib.h> // rand functions, dynamic memory allocation
#include <math.h>  // math functions - fmod, log
#include <time.h> // using clock, and count the runtime
#include <stdbool.h> // using boolean
#include <sys/sem.h> // using semaphore
#include <unistd.h> // for the fork
#include <wait.h> // for wait
#include <sys/types.h> // using types of data such as key_t, pid_t ..
#include <sys/ipc.h> // for shared memory
#include <sys/shm.h> // using share memory
#include <errno.h> // for using perror..
#include <signal.h> // for using signals

#define SEMPERM 0600
#define TRUE 1
#define FALSE 0

/* ===== Implementation of the cars queue ===== */
/*
	node of a cars queue -  
	this cars queue is linked list.
*/
struct car {
	int car_id;
	clock_t await_time; // the time that the car was wait in queue
	struct car *next; // a pointer to the next node
};

struct car *cars_queue = NULL; // a pointer to the head of the list

/*
	insert - insert link at the end location
	get - an car process id and insert it to the queue
*/
void insert(int id) {
	//create a link
	struct car *car_to_add = (struct car*)malloc(sizeof(struct car));
	car_to_add->car_id = id;
	car_to_add->await_time = time(NULL);
	if(!cars_queue){
		cars_queue = car_to_add;
	} else {
		struct car *ptr = cars_queue; // set a pointer to run on the list 
		while(ptr->next) { // get the last node to insert a new one
			ptr = ptr->next;
		}
		ptr->next = car_to_add; // add the car at the last position of the list
	} 
}

/* 
	pop - function that delete first item
*/
struct car* pop() {
	//save reference to first car
	struct car *tempLink = cars_queue;
	
	//mark next to first car as first 
	cars_queue = cars_queue->next;

	tempLink->await_time = time(NULL) - tempLink->await_time;
	//return the deleted car
	return tempLink;
}


/* ----- end of queue part ----- */

/* ===== washing machine part ===== */
/*
	washing_machine - a struct that represent the washing machines 
*/
struct washing_machines {
	int N; // the number of washing machines in the station
	int totalCars; // counting the total cars that got washed in the simlation
	struct car car_wash[5]; // a pointer to array of wash machines.
	clock_t time;
	clock_t start_time;
	key_t semkey;
	pid_t pid;
};

struct washing_machines *washings = NULL;

int getCarsInWashingMachines() {
	int count = 0;
	for(int i = 0; i < washings->N; i++){
		if(washings->car_wash[i].car_id != 0){
			count++;
		}
	}
	return count;
}

int getTheNextFreeMachine(){
	for(int i = 0; i < washings->N; i++){
		if(washings->car_wash[i].car_id == 0){
			return i;
		}
	}
	return -1;
}
/* ----- end washing machine part ----- */

/* ===== Implemantation of the times part ===== */
/*
/*
	nextTime - a function that calculate the next time
	 get -  a rate parameter.
	 return - next time, calculate it by poisson distribution algorithm.
*/
float nextTime(float rateParameter) {
	return -log(1.0f - (float)rand() / (RAND_MAX)) / rateParameter;
}

/*
	setRunTime - set the run time of the simulation.
	return - the runtime a constraint number between 29 - 31
*/
float getRunTime() {
	float runTime;
	do{
		runTime = nextTime((float)rand() / (RAND_MAX));
	} while (runTime < 29 || runTime > 31);
	return runTime;
}

/*
	getCarArrivalTime - function that calculate the time for the next vehical to arrriv the station.
	return - a constraint number between 1 - 2
*/
float getCarArrivalTime(){
	float time;
	do{
		time = nextTime((float)rand() / (RAND_MAX));
	} while (time < 1 || time > 2);
	return time;
}

/*
	getCarWashTime - function that calculate the time for washing vehical
	return - a constraint number between 2 - 3.5
*/
float getCarWashTime(){
	float time;
	do{
		time = nextTime((float)rand() / (RAND_MAX));
	} while (time < 2 || time > 3.5);
	return time;
}

/*
	getAllInputs - function that sets all the parameters.
	get - 3 opinters for the parameters and initiate them by user inputs.
*/
int getWashingMachines() {
	int washing_machines_N;
	printf("Please enter number of washing machines(the maximum 5): \n");
	scanf("%d", &washing_machines_N);
	if(washing_machines_N > 5) {
		washing_machines_N = 5;
	}
	return washing_machines_N;
}
/* ----- end times part ----- */

/* ===== Implementation of the semaphores archetectore ===== */
/*
	an union for semaphore
*/
typedef union _semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
} semun;

/*
	p - a function for semaphore down 
	get - a semaphore id and down it
	return - 0 if sucess 
*/
int p(int semid) {
  struct sembuf p_buf;
  p_buf.sem_num = 0;
  p_buf.sem_op = -1;
  p_buf.sem_flg = SEM_UNDO;

	//int semop(int semid, struct sembuf *sops, size_t nsops);
  if(semop(semid, &p_buf, 1) == -1 ) {
    perror("Error operation p(semid)");
    exit(1);
  }
  return 0;
}

/*
	v - a function for semaphore up
	get - a semaphore id and up it
	return - 0 if sucess.
*/
int v(int semid) {

    struct sembuf v_buf;
    v_buf.sem_num = 0;
    v_buf.sem_op = 1;
    v_buf.sem_flg = SEM_UNDO;
 
    if(semop(semid, &v_buf, 1) == -1) {
        perror("Error operation v(semid)");
        exit(1);
    }
    return 0;
}

/*
	initsem - a function that initialize semapore with a shared memory
	get - semkey - a share memory key to work with
*/
int initsem(key_t semkey) {
    int status = 0, semid;
	if((semid = semget(semkey, 1, SEMPERM | IPC_CREAT | IPC_EXCL )) == -1) {
	    if(errno == EEXIST) {
	        semid = semget(semkey, 1, 0);
	    }
    } else {
        union _semun arg;
        arg.val = 1;
        status = semctl(semid, 0, SETVAL, arg);
  	}

	if( semid == -1 || status == -1) {
    	perror("Error initsem");
    	exit(-1);
  	}
  	return (semid);
}

/*
	handlesem - function that handle the semaphore and work with the 
				critical section.
*/
void handlesem(key_t skey) {
	int semid;
	pid_t pid = getpid();
	if((semid = initsem(skey)) < 0){
		exit(1);
	}
	printf("Car %d arrive to washing station and add to queue, time left from start:%lusec\n", pid, time(NULL) - washings->start_time);
	p(semid); //down semaphore
	insert(pid); // insert car into the queue
	if(getCarsInWashingMachines() < washings->N){
		float washTime = getCarWashTime();
		sleep(washTime); // the washing time;
	  	int index_free_machine = getTheNextFreeMachine();
	  	washings->car_wash[index_free_machine].car_id = pop()->car_id;
		printf("Car %d inside the washing machine, time left from start:%lusec\n", pid, time(NULL) - washings->start_time);
		washings->totalCars++;
		washings->time += washTime;
	  	washings->car_wash[index_free_machine].car_id = 0;
	}
 	v(semid); // up semaphore
 	printf("Car %d end the washing and leave the station, time left from start:%lusec\n",pid, time(NULL) - washings->start_time);
}
/* ----- end semaphores part ----- */

void handleSignal(){
	while(getpid() != washings->pid){ // wait until the children finish 
		if(errno == ECHILD){
			break;
		}
	}
	printf("\ntotal cars that washed today is: %d\n", washings->totalCars);
	printf("\naverage time for wait in queue: %fsec\n", (time(NULL) - washings->start_time) / (float)washings->totalCars);
	semctl(washings->semkey, IPC_RMID, 0);
	exit(0);
}

int main() {
	srand(time(0)); //use this seed for the rand functin prodeuce new numbers each time.
	clock_t start; //store when the simulation starts
	pid_t pid;
	
	clock_t runtime = getRunTime(); //runtime the total simulation runtime.
	
	key_t semkey;
	semkey = ftok(".", 21);
	signal(SIGINT, handleSignal);
	int N = getWashingMachines();
	int shmid = shmget(semkey, sizeof(struct washing_machines), 0666 | IPC_CREAT);
	if(shmid < 0){
		perror("Error shmget");
		exit(1);
	}
	
	washings = (struct washing_machines*)shmat(shmid, 0, 0); // initial a pointer to washings machines with a shared memory
	washings->N = N;
	washings->totalCars = 0;
	start = time(NULL);
	washings->start_time = start;
	washings->time = start;
	washings->semkey = semkey;
	washings->pid = getpid();
	runtime += start;

	while (start < runtime){ // create the processes
		sleep(getCarArrivalTime());
		start = time(NULL);
		pid = fork();
		
		if(pid < 0) {
			printf("Fork error!\n");
		} else if(pid == 0) {
			break;
		}
	}

	if(pid > 0){ // parent process
		while(pid = waitpid(-1, NULL, 0)){ // wait until the children finish 
			if(errno == ECHILD){
				break;
			}
		}
		printf("\ntotal cars that washed today is: %d\n", washings->totalCars);
		printf("\naverage time for wait in queue: %fsec\n", (time(NULL) - washings->start_time) / (float)washings->totalCars);
		semctl(semkey, IPC_RMID, 0);
		exit(0);

	} else { //chiled process
		handlesem(semkey);
	}
	return 0;
}