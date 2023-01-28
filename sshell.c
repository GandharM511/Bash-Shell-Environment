#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CMDLINE_MAX 512
#define TOKLEN_MAX 32
#define ARGS_MAX 16
#define TRUE 1
#define FALSE 0
#define PIPE_MAX 4

typedef struct{
        char* cmds[CMDLINE_MAX];
        int background;
        pid_t pid[4];
        int commandNum;
        pid_t pidret[4];
        char fullcommand[CMDLINE_MAX];
        int done;
}Job;

typedef struct{
        char instruction[TOKLEN_MAX];
        char* arguments[ARGS_MAX];
        int outputfd;
}Command;

void completedFunction(char *cmd, int* status, int commandNumber){
        switch (commandNumber)
        {
        case 1:
                fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(status[0]));
                break;
        case 2:
                fprintf(stderr, "+ completed '%s' [%d][%d]\n", cmd, WEXITSTATUS(status[0]),WEXITSTATUS(status[1]));
                break;
        case 3:
                fprintf(stderr, "+ completed '%s' [%d][%d][%d]\n", cmd, WEXITSTATUS(status[0]),WEXITSTATUS(status[1]),WEXITSTATUS(status[2]));
                break;
        case 4:
                fprintf(stderr, "+ completed '%s' [%d][%d][%d][%d]\n", cmd, WEXITSTATUS(status[0]),WEXITSTATUS(status[1]),WEXITSTATUS(status[2]),WEXITSTATUS(status[3]));
                break;
        }
        
}

int pipeParser(char* cmd, Job *job){
        char copy[CMDLINE_MAX];
        char* token;
        const char delimiter[2]= "|";
        int i;


        strcpy(copy,cmd);
        

        token = strtok(copy, delimiter);
        for(i=0; token != NULL; i++){
                (job->cmds)[i] = token;
                token = strtok(NULL, delimiter);
                
        }


        return i;
        

}


int parser(Command *command, char* cmd, int index, char* copy) {

        
        char *token;
        char *filename;
        int i;
        const char delimiter[2]= " ";

        strcpy(copy,cmd);

        char* orindex = strchr(copy, '>');
        char* orindex2 = strrchr(copy, '>');
        if((orindex && orindex2)&&(orindex2 == orindex+1)){
                strcpy(copy,strtok(cmd,">>"));
                strcpy(copy,strtok(NULL,">>"));
                filename = strtok(copy, delimiter);
                (*(command+index)).outputfd = open(filename,O_WRONLY|O_APPEND|O_CREAT,0644);
        }
        else if(orindex){
                strcpy(cmd,strtok(copy,">"));
                filename = strtok(NULL, delimiter);
                (*(command+index)).outputfd = open(filename,O_WRONLY|O_CREAT,0644);

        }
        
        token = strtok(cmd, delimiter);
        if((!strcmp(token, ">")) || (!strcmp(token, "<")) || (!strcmp(token, "|"))){
                fprintf(stderr,"Error: missing command\n");
                return 1;
        }
        strcpy(command[index].instruction, token);
        for(i=0; token != NULL; i++){
                command[index].arguments[i] = token;
                token = strtok(NULL, delimiter);      
                if (i>15){
                        fprintf(stderr,"Error: too many process arguments\n");
                        return 1;
                }
        }
        if(!strcmp((*(command+index)).arguments[i-1], ">")){
                fprintf(stderr,"Error: no output file\n");
                return 1;
        } else if((!strcmp((*(command+index)).arguments[i-1], "<")) || (!strcmp((*(command+index)).arguments[i-1], "|"))){
                fprintf(stderr,"Error: missing command\n");
                return 1;
        } 
        (command[index].arguments)[i] = NULL;
       
        return 0;
        
}


void execute(char* cmd, char* copy, Job *job){
        int backstatus[PIPE_MAX];
        if(!strcmp("\0", cmd)){
                
                if ((job->pid)[0]!=1){
                        for (int i = 0; i < job->commandNum; i++){
                                (job->pidret)[i] = waitpid((job->pid)[i],&(backstatus[i]), WNOHANG);
                                if(!(job->pidret)[i]){
                                        fprintf(stderr, "Error: active jobs still running\n");
                                        fprintf(stderr, "+ completed 'exit' [%d]\n", 1);
                                        break;
                                }
                                else if(i == (job->commandNum)-1){
                                        completedFunction(job->fullcommand, backstatus, job->commandNum);
                                        job->pid[0] = 1;
                                        fprintf(stderr, "Bye...\n");
                                        fprintf(stderr, "+ completed 'exit' [%d]\n", 0);
                                }
                        }
                        
                }
                return;
        } 
        
        
        if ((job->pid)[0]!=1){
                
                for (int i = 0; i < job->commandNum; i++){
                       (job->pidret)[i] = waitpid((job->pid)[i],&(backstatus[i]), WNOHANG);
                       if(!(job->pidret)[i]){
                                break;
                       }
                       else if(i == (job->commandNum)-1){
                                job->done = TRUE;
                                //job->pid[0] = 1;
                       }
                }

        }
        
        int commandNumber = pipeParser(cmd,job);

        int fd[commandNumber-1][2]; // commandNumber -1 because if we have 2 commands, we would have 1 pipe etc
        pid_t pid[commandNumber];
        int parsingerror;

        Command *commands;
        commands = malloc(sizeof(Command) * commandNumber);
        
        for(int i = 0; i < commandNumber; i++){
                commands[i].outputfd = 1;
                parsingerror = parser(commands,(job->cmds)[i], i, copy);
                if(parsingerror) return;
        }

        if(!strcmp(commands[0].instruction, "cd")){
                int error_check;
                if(commands[0].arguments[1]==NULL){
                        error_check = -1;
                }
                else if(!strcmp(commands[0].arguments[1], ".")){
                        error_check = 0;
                }
                else{
                       error_check = chdir(commands[0].arguments[1]);
                }
                if(error_check == -1){
                        fprintf(stderr, "Error: cannot cd into directory\n");
                        error_check = 1;
                }
                fprintf(stderr, "+ completed '%s' [%d]\n", cmd, error_check);
                return;    
        }

        int j;
        for(j = 0; j < commandNumber-1; j++){
        pipe(fd[j]);
        }


        for (int i = 0; i < commandNumber; i++){ 
                pid[i] = fork();
                if (pid[i] == 0){
                        if(commandNumber == 1 && commands[i].outputfd != 1){
                                dup2(commands[i].outputfd, STDOUT_FILENO);   
                                
                        }
                        else if(commandNumber == 1){
                                execvp(commands[i].instruction, commands[i].arguments); //execvp since it searches the command utilizing $PATH
                                exit(1);
                                
                        }
                        else if(i == 0){
                                dup2(fd[i][1], STDOUT_FILENO);
                                for (int j = 0; j < commandNumber-1; j++){
                                        close(fd[j][0]);
                                        close(fd[j][1]);
                                }
        
                                
                        }
                        else if(i == commandNumber-1){
                                dup2(fd[i-1][0], STDIN_FILENO);
                                for (int j = 0; j < commandNumber-1; j++){
                                        close(fd[j][0]);
                                        close(fd[j][1]);
                                }
                                if(commands[i].outputfd != 1) {
                                        dup2(commands[i].outputfd, STDOUT_FILENO);   
                                }
                        }
                        else{
                                dup2(fd[i-1][0], STDIN_FILENO);
                                dup2(fd[i][1], STDOUT_FILENO);
                                for (int j = 0; j < commandNumber-1; j++){
                                        close(fd[j][0]);
                                        close(fd[j][1]);
                                }
                                
                        }   
                        execvp(commands[i].instruction, commands[i].arguments); //execvp since it searches the command utilizing $PATH
                        exit(1);           
                } 
        }

       
        int status[commandNumber];
        for(int i = 0; i < commandNumber-1; i++){
                close(fd[i][1]);
                close(fd[i][0]);
        }
        
        if(!(job->background)){
                
                for (int i = 0; i < commandNumber; i++){
                        waitpid(0, &(status[i]), 0);
                }
                for (int i = 0; i < commandNumber; i++){
                        if(WEXITSTATUS(status[i]) == 1 && strcmp(commands[i].instruction, "cat")){
                                fprintf(stderr, "Error: command not found\n");
                        }
                }
                if (job->done && job->pid[0]!= 1){
                        completedFunction(job->fullcommand, backstatus, job->commandNum);
                        job->pid[0] = 1;

                }
                completedFunction(cmd, status, commandNumber);
        }
        else if(job->background){
                for(int i = 0; i < commandNumber; i++){
                        (job->pid)[i] = pid[i];
                }
                job->commandNum = commandNumber;
                job->done = FALSE;
                
        }
        free(commands);
 
}

int main(void){

        char cmd[CMDLINE_MAX];
        char copy[CMDLINE_MAX];
        char path[CMDLINE_MAX];
        //int backgroundJobs = 0;
        //int pid = 1;
        //int status;
        Job job;
        job.pid[0]=1;
        job.background = FALSE;
        job.done = TRUE;

        while (1) {
                
                
                char *nl;
                /* Print prompt */
                printf("sshell@ucd$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                /* Builtin commands */
                if (!strcmp(cmd, "exit")) {
                        if(!(job.done)){
                                execute("\0", "\0", &job);
                                if(job.pid[0]==1){
                                        break;
                                }
                                else{
                                        continue;
                                }
                        }
                        fprintf(stderr, "Bye...\n");
                        fprintf(stderr, "+ completed '%s' [%d]\n", cmd, 0);
                        break;
                }

                if (!strcmp(cmd, "pwd")) {
                        fprintf(stdout, "%s\n", getcwd(path, CMDLINE_MAX));
                        fprintf(stderr, "+ completed '%s' [%d]\n", cmd, 0);
                }

                /* Regular command */
                else{
                        if(*(nl-1) == '&'){
                                strcpy(job.fullcommand, cmd);
                                *(nl-1) = '\0';
                                job.background = TRUE;
                        }
                        execute(cmd, copy, &job);
                        job.background = FALSE;
                        
                }

                
        }

        return EXIT_SUCCESS;
}
