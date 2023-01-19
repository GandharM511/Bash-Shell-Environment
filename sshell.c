#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

/*struct command{
        char instruction[32];
        //char* flags[17];
        char* arguments[17];
};*/




/*struct command parser(char* cmd){
        struct command obj2;
        int i;
        //int j = 0;
        int k = 0;
        char *copy;
        char *token;
        const char delimiter[4]= " ><";


        copy = strdup(cmd);
        token = strtok(copy, delimiter);
        for(i=0; token != NULL; i++){
                if(i == 0){
                        strcpy(obj2.instruction, token);
                }
                if(token[0] == '-'){
                        strcpy(obj2.flags[j], token);
                        j += 1;
                }
                else{
                        strcpy(obj2.arguments[k], token);
                        k += 1;
                }
                token = strtok(NULL, delimiter);      
                if (i>15){
                        fprintf(stderr,"Error: too many process arguments\n");
                        struct command nullobj;
                        strcpy(nullobj.instruction, "BREAK");
                        return nullobj;
                }
        }
        obj2.arguments[k] = NULL;
        return obj2;
}*/


void singleCommands(char* cmd){
        char *copy;
        char *token;
        char *args[17];
        pid_t pid;
        int i;
        const char delimiter[2]= " ";
        
        int error_check;


        copy = strdup(cmd);
        token = strtok(copy, delimiter);
        if(!strcmp(token, ">")){
                fprintf(stderr,"Error: missing command\n");
                return;
        } else if(!strcmp(token, "<")){
                fprintf(stderr,"Error: missing command\n");
                return;
        } else if(!strcmp(token, "|")){
                fprintf(stderr,"Error: missing command\n");
                return;
        }

        for(i=0; token != NULL; i++){
                args[i] = token;
                token = strtok(NULL, delimiter);      
                if (i>15){
                        fprintf(stderr,"Error: too many process arguments\n");
                        return;
                }
        }

        // ask if double pipes will be tested

        if(!strcmp(args[i-1], ">")){
                fprintf(stderr,"Error: no output file\n");
                return;
        } else if(!strcmp(args[i-1], "<")){
                fprintf(stderr,"Error: missing command\n");
                return;
        } else if(!strcmp(args[i-1], "|")){
                fprintf(stderr,"Error: missing command\n");
                return;
        }
        args[i] = NULL;
        
        /*struct command obj1;
        obj1 = parser(cmd);
        if(!strcmp(obj1.instruction, "BREAK")){
                return;
        }*/

        if(!strcmp(args[0], "cd")){
                if(args[1]==NULL){
                        error_check = -1;
                }
                else if(!strcmp(args[1], ".")){
                        error_check = 0;
                }
                else{
                       error_check = chdir(args[1]);
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
                //PUT EXIT INTO CHILD            
                execvp(args[0], args); //execvp since it searches the command utilizing $PATH
                exit(1);              
        } else if (pid > 0) {
                /* Parent */
                int status;
                waitpid(pid, &status, 0);
                if(WEXITSTATUS(status) == 1){
                        fprintf(stderr, "Error: command not found\n");
                }
                fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(status));

        }
} 




int main(void)
{
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
                        break;
                }

                if (!strcmp(cmd, "pwd")) {
                        fprintf(stdout, "%s\n", getcwd(path, 512));
                }



                /* Regular command */
                else{
                        singleCommands(cmd);
                }

                
        }

        return EXIT_SUCCESS;
}
