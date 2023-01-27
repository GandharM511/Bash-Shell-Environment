#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CMDLINE_MAX 512
#define TOKLEN_MAX 32
#define ARGS_MAX 16

typedef struct{
        char* cmds[CMDLINE_MAX];
}Job;

typedef struct{
        char* fullcommand;
        char instruction[TOKLEN_MAX];
        char* arguments[ARGS_MAX];
        int outputfd;
}Command;



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
        if(orindex){
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


void execute(char* cmd, char* copy){

        if(!strcmp("\0", cmd)){
                return;
        } 

        
       
        

        
        Job job;

        int commandNumber = pipeParser(cmd,&job);

        int fd[commandNumber-1][2]; // commandNumber -1 because if we have 2 commands, we would have 1 pipe etc
        pid_t pid[commandNumber];
        int parsingerror;

        Command *commands;
        commands = malloc(sizeof(Command) * commandNumber);
        
        for(int i = 0; i < commandNumber; i++){
                commands[i].outputfd = 1;
                parsingerror = parser(commands,job.cmds[i], i, copy);
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
                        else if(i == 0){
                                dup2(fd[i][1], STDOUT_FILENO);
                                close(fd[i][0]);
                                close(fd[i][1]);
                                
                                
                        }
                        else if(i == commandNumber-1){
                                dup2(fd[i-1][0], STDIN_FILENO);
                                fprintf(stderr, "%d input: %d\n",i-1, fd[i-1][0]);
                                fprintf(stderr, "===========\n");
                                close(fd[i-1][0]);
                                close(fd[i-1][1]);
                                if(commands[i].outputfd != 1) {
                                        dup2(commands[i].outputfd, STDOUT_FILENO);   
                                }
                               


                        }
                        else{
                                dup2(fd[i-1][0], STDIN_FILENO);
                                dup2(fd[i][1], STDOUT_FILENO);
                                fprintf(stderr, "%d input: %d\n",i, fd[i-1][0]);
                                fprintf(stderr, "%d output: %d\n",i, fd[i][1]);
                                fprintf(stderr, "===========\n");

                                close(fd[i-1][0]);
                                close(fd[i-1][1]);
                                close(fd[i][0]);
                                close(fd[i][1]);
                                
                        }   
                        
                        execvp(commands[i].instruction, commands[i].arguments); //execvp since it searches the command utilizing $PATH
                        fprintf(stderr, "wrong\n");
                        exit(1);           
                } 
                
                
        }

       
        
        
        

        /*pipe(fd[i]);
        pid[i] = fork();
        if (pid == 0){
                if(commandNumber > 1){
                        close(fd[0]);
                        dup2(fd[1], STDOUT_FILENO);
                        close(fd[1]);
                }else{
                        if(commands[0].outputfd != 1) {
                                dup2(commands[0].outputfd, STDOUT_FILENO);   
                        }   
                }   
                execvp(commands[0].instruction, commands[0].arguments); //execvp since it searches the command utilizing $PATH
                exit(1);           
        } else if (pid > 0 ) {
                if(commandNumber > 1){
                        pid2 = fork();
                        if(pid2 == 0) {
                                close(fd[1]);
                                dup2(fd[0], STDIN_FILENO);
                                close(fd[0]);
                                
                                if(commands[1].outputfd != 1) {
                                        dup2(commands[1].outputfd, STDOUT_FILENO);   
                                } 
                                execvp(commands[1].instruction, commands[1].arguments);
                                fprintf(stderr, "hello2\n");
                                exit(20);
                        }
                }
        }*/
        
        int status;
        for(int i = 0; i < commandNumber-1; i++){
        close(fd[i][1]);
        close(fd[i][0]);
        }

        for (int i = 0; i < commandNumber; i++){
                //waitpid(pid[i], &status, 0);
                
                //wait(NULL);
                wait(NULL);

                

        }
        
        if(WEXITSTATUS(status) == 1 && strcmp(commands[0].instruction, "cat")){
                fprintf(stderr, "Error: command not found\n");
        }
        fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(status));

        free(commands);
 
}

int main(void){

        char cmd[CMDLINE_MAX];
        char copy[CMDLINE_MAX];
        char path[CMDLINE_MAX];

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
                        
                        execute(cmd, copy);
                }

                
        }

        return EXIT_SUCCESS;
}
