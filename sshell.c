#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CMDLINE_MAX 512

struct Command{
        char instruction[32];
        char* arguments[17];
        int outputfd;
}command;

int parser(struct Command *command, char* cmd) {

        char *copy;
        char *lhscmd;
        char *token;
        char *filename;
        int i;
        const char delimiter[2]= " ";

        copy = strdup(cmd);

         

        char* orindex = strchr(copy, '>');
        if(orindex){
                lhscmd = strtok(copy,">");
                filename = strtok(NULL, delimiter);
                command->outputfd = open(filename,O_WRONLY|O_CREAT,0644);

        }
        else{
                lhscmd = strdup(cmd);
        }
        token = strtok(lhscmd, delimiter);
        if((!strcmp(token, ">")) || (!strcmp(token, "<")) || (!strcmp(token, "|"))){
                fprintf(stderr,"Error: missing command\n");
                return 1;
        }
        strcpy(command->instruction, token);
        for(i=0; token != NULL; i++){
                (command->arguments)[i] = token;
                token = strtok(NULL, delimiter);      
                if (i>15){
                        fprintf(stderr,"Error: too many process arguments\n");
                        return 1;
                }
        }
        if(!strcmp((command->arguments)[i-1], ">")){
                fprintf(stderr,"Error: no output file\n");
                return 1;
        } else if((!strcmp((command->arguments)[i-1], "<")) || (!strcmp((command->arguments)[i-1], "|"))){
                fprintf(stderr,"Error: missing command\n");
                return 1;
        } 
        (command->arguments)[i] = NULL;

        return 0;
        
}


void execute(char* cmd){

        if(!strcmp("\0", cmd)){
                return;
        } 

        pid_t pid;
        int error_check;
        int parsingerror;      
        
        struct Command obj;
        obj.outputfd = 1;
        parsingerror = parser(&obj,cmd);
        if(parsingerror) return;

        if(!strcmp(obj.instruction, "cd")){
                if(obj.arguments[1]==NULL){
                        error_check = -1;
                }
                else if(!strcmp(obj.arguments[1], ".")){
                        error_check = 0;
                }
                else{
                       error_check = chdir(obj.arguments[1]);
                }
                if(error_check == -1){
                        fprintf(stderr, "Error: cannot cd into directory\n");
                        error_check = 1;
                }
                fprintf(stderr, "+ completed '%s' [%d]\n", cmd, error_check);
                return;    
        }

        pid = fork();
        if (pid == 0){
                /* Child */  
                if(obj.outputfd != 1) {
                        dup2(obj.outputfd, STDOUT_FILENO);   
                }        
                execvp(obj.instruction, obj.arguments); //execvp since it searches the command utilizing $PATH
                exit(1);              
        } else if (pid > 0) {
                /* Parent */
                int status;
                waitpid(pid, &status, 0);
                if(WEXITSTATUS(status) == 1 && strcmp(obj.instruction, "cat")){
                        fprintf(stderr, "Error: command not found\n");
                }
                fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(status));

        }
} 


int main(void){

        char cmd[CMDLINE_MAX];
        char path[512];

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
                        fprintf(stdout, "%s\n", getcwd(path, 512));
                        fprintf(stderr, "+ completed '%s' [%d]\n", cmd, 0);
                }

                /* Regular command */
                else{
                        execute(cmd);
                }

                
        }

        return EXIT_SUCCESS;
}
