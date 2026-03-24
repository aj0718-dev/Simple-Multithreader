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
 // int state; //0: Running, 1: Waiting
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

void remove_space(char* str){
 int start = 0;
 int end = strlen(str) - 1;
 // printf("%d\n", end);

 while (isspace(str[start])){
 start++;
 }
 while (end >= 0 && isspace(str[end])){
 end--;
 }
 for (int i = 0; i <= end - start; i++){
 str[i] = str[i + start];
 }
 str[end - start + 1] = '\0';
}

int create_process_and_run(char* cmd){
 char* args[100];
 int cnt = 0;
 char *token;

 token = strtok(cmd, " ");

 while(token != NULL && cnt < 100){
 args[cnt] = token;
 cnt++;
 token = strtok(NULL, " ");
 }

 if (cnt < 100){
 args[cnt] = NULL;
 }

 int ret = fork();
 if (ret < 0){
 perror("fork error");
 return -1;
 }
 else if(ret == 0){
 printf("command: %s\n", args[0]);
 execvp(args[0], args);
 perror("exec failed1");
 exit(1);
 }
 else{
 wait(NULL);
 }
 return 1;
}

char* read_user_input(){
 char* inp = malloc(1024 * sizeof(char));
 fgets(inp, 1024, stdin);

 remove_space(inp);
 return inp;
}

void print_terminated_queue() {
 for (int i = 0; i < terminated_queue->rear; i++){
 Process process = terminated_queue->processes[i];
 printf("Executable: %s\tPID: %d\tWaiting time: %ld\tExecution time: %ld\n", process.name, process.pid, process.total_waitingtime, process.total_runtime);
 }
}

volatile sig_atomic_t ready_signal_received = 0; // For signal handling

void signal_handler(int signum) {
 ready_signal_received = 1; // Set flag when signal is received
}

void run_process(int process_number){
 // Process running_process;

 int shm_fd = shm_open(SHM_RUNNING_PROCESS, O_CREAT | O_RDWR, 0666);
 if (shm_fd == -1) {
 perror("shm_open failed");
 exit(EXIT_FAILURE);
 }
 
 // Configure the size of the shared memory segment
 if (ftruncate(shm_fd, sizeof(Process)) == -1) {
 perror("ftruncate failed");
 exit(EXIT_FAILURE);
 }

 // Map the shared memory segment
 Process *running_process = mmap(NULL, sizeof(Process), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
 if (running_process == MAP_FAILED) {
 perror("mmap failed");
 exit(EXIT_FAILURE);
 }

 sem_wait(&ready_sem);
 dequeue(ready_queue, running_process);
 sem_post(&ready_sem);

 // New process
 if (running_process->pid == 0){
 pid_t process_pid = fork();
 if (process_pid < 0){
 perror("Fork failed");
 exit(0);
 }
 //child process
 else if (process_pid == 0){
//  printf("%d\n", 1);
 char* executable_name = strtok(running_process->name, "./");
 gettimeofday(&running_process->start, NULL);
 long int waited_time = (running_process->start.tv_sec-running_process->creation.tv_sec)*1000 + (running_process->start.tv_usec-running_process->creation.tv_usec) / 1000;
 running_process->total_waitingtime += waited_time;
 execlp(running_process->name, executable_name, NULL);
 perror("Exec failed2");
 exit(0);
 }
 else{
 usleep(TSLICE*1000);
 int status = waitpid(process_pid, NULL, WNOHANG);
 // Process still running
 if (status == 0){
//  printf("%d\n", 2);
 kill(process_pid, SIGSTOP);
//  printf("PID Stopped: %d\n", process_pid);
 running_process->pid = process_pid;
 gettimeofday(&running_process->end, NULL);
 long int executed_time = (running_process->end.tv_sec - running_process->start.tv_sec)*1000 + (running_process->end.tv_usec - running_process->start.tv_usec) / 1000;
 running_process->total_runtime += executed_time;
 sem_wait(&ready_sem);
 enqueue(ready_queue, *running_process);
 sem_post(&ready_sem);
//  kill(ret, SIGUSR1);
 }
 // Process terminated
 else {
//  printf("%d\n", 3);
 running_process->pid = process_pid;
 gettimeofday(&running_process->end, NULL);
 long int executed_time = (running_process->end.tv_sec - running_process->start.tv_sec)*1000 + (running_process->end.tv_usec - running_process->start.tv_usec) / 1000;
 running_process->total_runtime += executed_time;
 sem_wait(&terminated_sem);
 enqueue(terminated_queue, *running_process);
 sem_post(&terminated_sem);
 }

 }
 }
 else {
 gettimeofday(&running_process->start, NULL);
 long int waited_time = (running_process->start.tv_sec - running_process->end.tv_sec)*1000 + (running_process->start.tv_usec - running_process->end.tv_usec)/1000;
 running_process->total_waitingtime += waited_time;
//  printf("PID Continues: %d\n", running_process->pid);
 kill(running_process->pid, SIGCONT);

//  printf("%d\n", 4);
 // printf("Status: %d\n", kill(running_process->pid, 0));
 usleep(TSLICE*1000);

 int status = kill(running_process->pid, 0);
//  printf("Status: %d\n", status);

 // Process still running
 if (status == 0){
//  printf("%d\n", 5);
 kill(running_process->pid, SIGSTOP);
 gettimeofday(&running_process->end, NULL);
 long int executed_time = (running_process->end.tv_sec - running_process->start.tv_sec)*1000 + (running_process->end.tv_usec - running_process->start.tv_usec) / 1000;
 running_process->total_runtime += executed_time;
 sem_wait(&ready_sem);
 enqueue(ready_queue, *running_process);
 sem_post(&ready_sem);
//  kill(ret, SIGUSR1);
 }
 // Process terminated
 else if (errno == ESRCH) {
//  printf("%d\n", 6);
 gettimeofday(&running_process->end, NULL);
 long int executed_time = (running_process->end.tv_sec - running_process->start.tv_sec)*1000 + (running_process->end.tv_usec - running_process->start.tv_usec) / 1000;
 running_process->total_runtime += executed_time;
 sem_wait(&terminated_sem);
 enqueue(terminated_queue, *running_process);
 sem_post(&terminated_sem);
 }
 }
//  printf("%d\n", 7);
//  printf("Size: %d\n", ready_queue->rear);
 if (munmap(running_process, sizeof(Process)) == -1) {
 perror("munmap failed");
 exit(EXIT_FAILURE);
 }

 // Close the shared memory file descriptor
 if (close(shm_fd) == -1) {
 perror("close failed");
 exit(EXIT_FAILURE);
 }

 // Unlink the shared memory segment
 if (shm_unlink(SHM_RUNNING_PROCESS) == -1) {
 perror("shm_unlink failed");
 exit(EXIT_FAILURE);
 }

}

void run_scheduler() {
 // printf("Inside scheduler\n");
 while (1) {
 sem_wait(&ready_sem);
 int size = ready_queue->rear;
 sem_post(&ready_sem);
 if (size > 0){
 // There are processes in the ready queue
 // printf("Starting to execute processes...\n");
 sem_wait(&ready_sem);
 int NCPU_consume = (ready_queue->rear <= NCPU) ? ready_queue->rear : NCPU;
 // printf("NCPU_consume: %d\n", NCPU_consume);
 sem_post(&ready_sem);
 for (int i = 0; i < NCPU_consume; i++){
 pid_t pid = fork();
 if (pid == 0){
//  printf("Running scheduler\n");
 run_process(i);
 exit(1);
 }
 else if (pid > 0) {
 continue;
 }
 else {
 perror("Fork failed");
 exit(0);
 }
 }
 usleep((TSLICE)*1000);
 } else {
 // No processes, sleep until a signal is received
//  printf("Waiting for signal\n");
//  pause(); // Wait for a signal
//  printf("Signal receiver\n");
usleep(TSLICE*1000);
 }
 }
}

int launch(char* cmd){
 int status;

 if(strcmp(cmd, "exit") == 0){
 print_terminated_queue();
 printf("Shell ended Successfully!\n");
 status = 0;
 }
 else if (strncmp(cmd, "submit ", 7) == 0){
 cmd += 7;
 sem_wait(&ready_sem);
 gettimeofday(&ready_queue->processes[ready_queue->rear].creation, NULL);
 strcpy(ready_queue->processes[ready_queue->rear].name, cmd);
 ready_queue->rear++;
 sem_post(&ready_sem);

//  kill(ret, SIGUSR1);
 status = 1;
 }
 // Unpiped command
 // else if (strchr(cmd, '|') == NULL){
 // status = create_process_and_run(cmd);
 // }
 // // Piped command
 // else{
 // printf("Piped commands aren't supported\n");
 // }
 return status;
}

void shell_loop(){
 int status;
 do {
 printf("simpleShell:~$ ");
 char* cmd = read_user_input(); 
 status = launch(cmd);
 free(cmd);
 } while(status);
}

int main(int argc, char** argv) {
 if (argc != 3) {
 fprintf(stderr, "Usage: %s <ncpu> <tslice>\n", argv[0]);
 return EXIT_FAILURE;
 }

 NCPU = atoi(argv[1]);
 TSLICE = atoi(argv[2]);

 int ready_fd, terminated_fd;

 ready_queue = setup_queue(SHM_READY_NAME, &ready_fd);
 terminated_queue = setup_queue(SHM_TERMINATED_NAME, &terminated_fd);

 ready_queue->rear = 0;
 terminated_queue->rear = 0;

 sem_init(&ready_sem, 1, 1);
 sem_init(&terminated_sem, 1, 1);

 struct sigaction sa;
 sa.sa_handler = signal_handler;
 sa.sa_flags = 0; // No special flags
 sigaction(SIGUSR1, &sa, NULL);

 //logic
 ret = fork();
 if (ret == 0){
 run_scheduler();
 }
 else if (ret > 0){
 shell_loop();
 kill(ret, SIGSTOP);
 }
 else {
 perror("Fork failed\n");
 exit(0);
 }

 sem_destroy(&ready_sem);
 sem_destroy(&terminated_sem);

 munmap(ready_queue, sizeof(Queue));
 munmap(terminated_queue, sizeof(Queue));

 close(ready_fd);
 close(terminated_fd);

 shm_unlink(SHM_READY_NAME);
 shm_unlink(SHM_TERMINATED_NAME);

 return 0;
}