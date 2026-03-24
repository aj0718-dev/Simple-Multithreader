#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <semaphore.h> 
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <ctype.h>
#include <errno.h>
// #include <dummy_main.h>

#define MAX_PROCESSES 100
#define CMD_LENGTH 256
#define SHM_READY_NAME "/my_shared_ready_queue"
#define SHM_TERMINATED_NAME "/my_shared_terminated_queue"
#define SHM_RUNNING_PROCESS "/my_process"

typedef struct{
    char name[CMD_LENGTH];
    pid_t pid;
    // int state;  //0: Running, 1: Waiting
    struct timeval creation;
    struct timeval start;
    struct timeval end;
    long total_runtime;
    long total_waitingtime;
}Process;

typedef struct{
    Process processes[MAX_PROCESSES];
    int rear;
}Queue;

Queue* ready_queue;
Queue* terminated_queue;

sem_t ready_sem;
sem_t terminated_sem;

pid_t ret;

int NCPU;
int TSLICE;

void enqueue(Queue* queue, Process process) {
    if (queue->rear == MAX_PROCESSES - 1) {
        printf("Queue is full.\n");
        return;
    }
    queue->processes[queue->rear] = process;
    queue->rear++;
}

int dequeue(Queue* queue, Process* process) {
    if (queue->rear <= 0) {
        printf("Queue is empty!\n");
        return -1; // Indicate failure
    }
    *process = queue->processes[0];

    for (int i = 1; i <= queue->rear; i++) {
        queue->processes[i - 1] = queue->processes[i];
    }
    queue->rear--;
    return 0; // Indicate success
}

int size(Queue* queue) {
    return queue->rear;
}

Queue* setup_queue(const char* shm_name, int* shm_fd) {
    // Create a shared memory object
    *shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (*shm_fd < 0) {
        perror("shm_open");
        exit(1);
    }

    // Set the size of the shared memory segment
    if (ftruncate(*shm_fd, sizeof(Queue)) == -1) {
        perror("ftruncate");
        exit(1);
    }

    // Map the shared memory segment to the process's address space
    Queue* queue = mmap(0, sizeof(Queue), PROT_READ | PROT_WRITE, MAP_SHARED, *shm_fd, 0);
    if (queue == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // Initialize the shared memory Queue
    memset(queue, 0, sizeof(Queue)); // Set all values to default (zeroed)

    return queue;
}

int main() {
    int ready_fd, terminated_fd;

    ready_queue = setup_queue(SHM_READY_NAME, &ready_fd);

    ready_queue->rear = 0;

    Process p1 = {
        .name = "abc",
        .pid = 1,
        .creation = {0},
        .start = {0},
        .total_runtime = 0,
        .total_waitingtime = 0,
    };

    Process p2 = {
        .name = "def",
        .pid = 3,
        .creation = {0},
        .start = {0},
        .total_runtime = 0,
        .total_waitingtime = 0,
    };

    enqueue(ready_queue, p1);
    enqueue(ready_queue, p2);

    printf("%s-%d\n", ready_queue->processes[0].name, ready_queue->processes[0].pid);
    printf("%s-%d\n", ready_queue->processes[1].name, ready_queue->processes[1].pid);
    printf("%s-%d\n", ready_queue->processes[2].name, ready_queue->processes[2].pid);

    // Process* p3;

    dequeue(ready_queue, &p1);

    printf("%s-%d\n", ready_queue->processes[0].name, ready_queue->processes[0].pid);
    printf("%s-%d\n", ready_queue->processes[1].name, ready_queue->processes[1].pid);

    // printf("Here\n");

    munmap(ready_queue, sizeof(Queue));

    close(ready_fd);

    shm_unlink(SHM_READY_NAME);

    return 0;
}