#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

typedef struct{
    char command[1024]; 
    time_t start_time;
    double execution_time;
    pid_t pid;
}history_t;

history_t HistoryList[100];
int historyCnt = 0;
char* History[100];
int cnt = 0;

void show_history(){
    for (int i = 0; i < cnt; i++){

        printf("%s\n", History[i]);
    }
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

int history_piped_commands(char*cmd){
    char *save_state; // pointer for strtok_r
    char *tok;
    int pipe_fd[2]; //pipe for IPC
    int input_fd=0;
    pid_t pid;
    int cmd_cnt=0;
    // tokenize the input
    tok= strtok_r(cmd,"|", &save_state);
// processing each command
    while(tok!=NULL){
        remove_space(tok);
        // creating a pipe
        if(pipe(pipe_fd)==-1){
            perror("Pipe error");
            exit(EXIT_FAILURE);
        }

        // fork and executing the command
        pid=fork();
        if(pid<0){
            perror("fork error");
            exit(EXIT_FAILURE);
        }
        else if(pid==0){
            // child process
            if(input_fd!=0){
                dup2(input_fd,STDIN_FILENO);
                close(input_fd);
                // read from previous pipe, if not the first command
            }
            if(save_state!=NULL){
                dup2(pipe_fd[1],STDOUT_FILENO);
                // write to the current pipe 
            }
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            // closing the ends of the pipe

            char*args[100];
            int cnt=0;
            char*arg_tok=strtok(tok, " ");
            // tokenizing for exec
            while(arg_tok!=NULL && cnt<100){
                args[cnt++]=arg_tok;
                arg_tok=strtok(NULL, " ");
            }
            args[cnt]=NULL;
            execvp(args[0], args);
            // execute
            perror("exec failed");
            exit(EXIT_FAILURE);
        }
        else{
            // parent process
            close(pipe_fd[1]);
            // closing the write end
            if(historyCnt<100){
                strncpy(HistoryList[historyCnt].command, tok, 1024);
                HistoryList[historyCnt].pid=pid;
                time(&HistoryList[historyCnt].start_time);
                HistoryList[historyCnt].execution_time=0.0;
                // execcution time will be updated later
                historyCnt++;
            }
            else{
                printf("history is full");
            }
            waitpid(pid,NULL,0);
            // waiting for the child process to finish
            if(input_fd!=0){
                close(input_fd);
            }
            input_fd=pipe_fd[0];
            // read end is moved to input_fd
        }
        tok=__strtok_r(NULL, "|", &save_state);
        cmd_cnt++;
    }
    if(input_fd!=0){
        close(input_fd);
        // closing the last pipes end in PP
    }
    if(historyCnt>0){
        HistoryList[historyCnt-1].execution_time=difftime(time(NULL),HistoryList[historyCnt-1].start_time);

    }
    return 1;   
}

void command_history(){
    if(historyCnt==0){
        printf("No commands in history\n");
        return;
    }
    // printf("Command History: \n");
    int i=0;
    while(i<historyCnt){
        printf("Command [%d]:\n",i+1);
        printf("PID: %d\n", HistoryList[i].pid);
        printf("Command: %s\n", HistoryList[i].command);
        printf("Start time: %s", ctime(&HistoryList[i].start_time));
        printf("Execution Duration: %.2f seconds\n", HistoryList[i].execution_time);
        printf("-----------------------------\n");
        i++;
    }

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
        if (strcmp(cmd, "history") == 0){
            show_history();
            exit(0);
        }
        execvp(args[0], args);
        perror("exec failed");
        exit(1);
    }
    else{
        if (historyCnt>= 100){
            for (int i = 1; i < 100; i++){
                HistoryList[i-1] = HistoryList[i];
            }
            historyCnt--;
        }

        time(&HistoryList[historyCnt].start_time);
        HistoryList[historyCnt].pid = ret;
        strncpy(HistoryList[historyCnt].command, cmd, 1024);
        wait(NULL);
    }
    HistoryList[historyCnt].execution_time = difftime(time(NULL), HistoryList[historyCnt].start_time);
    historyCnt++;
    return 1;
}

char* read_user_input(){
    char* inp = malloc(1024 * sizeof(char));
    fgets(inp, 1024, stdin);

    remove_space(inp);
    char* temp = malloc(strlen(inp));
    strncpy(temp, inp, strlen(inp));
    History[cnt++] = temp;
    return inp;
}
int run_piped_process(char* cmd1, char* cmd2, char* cmd){
    pid_t pid1, pid2;
    int fd[2];

    pipe(fd);

    pid1 = fork();
    if (pid1 < 0){
        perror("fork error");
    }
    else if (pid1 == 0){
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        system(cmd);
        exit(1);
    }
    else{
        pid2 = fork();

        if (pid2 < 0){
            perror("fork error");
        }
        else if (pid2 == 0){
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            system(cmd);
            exit(1);
        }
        else{
            close(fd[0]);
            close(fd[1]);
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
        }
    }
    history_piped_commands(cmd);
    return 1;
}

int launch(char* cmd){
    int status;

    if(strcmp(cmd, "exit") == 0){
        command_history();
        printf("Shell ended Successfully!\n");
        exit(0);
    }
    // Unpiped command
    else if (strchr(cmd, '|') == NULL){
        status = create_process_and_run(cmd);
    }
    // Piped command
    else{
        char* cpy = malloc(strlen(cmd)+1);
        strcpy(cpy, cmd);
        char* token = strtok(cmd, "|");

        char* cmd1;
        char* cmd2;

        while (token != NULL){
            cmd1 = token;
            token = strtok(NULL, "|");
        }
        remove_space(cmd1);
        remove_space(cmd2);
        // printf("C1: %s, C2: %s", cmd1, cmd2);
        status = run_piped_process(cmd1, cmd2, cpy);
    }

    return status;
}
// Signal handler for SIGINT (Ctrl+C)
void sigint_handler(int signum){
    printf("\nCtrl-c pressed\n");
    printf("\nExiting the shell...........\n");
    printf("Command History:\n");
    command_history();  // Calling the function to display the command history 
    exit(0);  
}

// Setting the signal handler for Ctrl+C using sigaction
void set_signal_handler(){
    struct sigaction sa;  
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1){
        perror("sigaction error");  
        exit(EXIT_FAILURE);  
    }
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

int main(){
    set_signal_handler();
    shell_loop();
    return 0;
}
