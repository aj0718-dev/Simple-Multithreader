#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <semaphore.h>

//Defining max values
#define MAX_INPUT_LENGTH 1024
#define MAX_LINE_LENGTH 10000

//Structure definition of process
typedef struct process{
   
    char* com_name;
    char* com[MAX_INPUT_LENGTH];
    pid_t pid;
    int priority_no;
    struct timespec start_time;
    struct timespec end_time;
    double exec_time ;
    double waiting_time ;
    int f1;
}Process;

//Structure defintion of a node in a queue
typedef struct node{
    Process * process_data;
    struct node* next;
}Node;

//Structure defintion of a queue
typedef struct queue{
    Node* front;
    Node* end;
    int tslice;
}Queue;

//Structure defintion of the shared memory
typedef struct shm_t {
    int is_shell_exit;
    pid_t scheduler_pid;
    pid_t shell_pid;
    int size;
    int n_process;
    char process_name[256][64][64];
    int ncpus_shm ;
    double tslice_shm;
}shm_t;