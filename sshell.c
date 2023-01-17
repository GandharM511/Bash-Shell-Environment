#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

void singleCommands(char* cmd){
        char *copy;
        char *token;
        char *args[17];
        pid_t pid;
        const char delimiter[4]= " ><";
        int i;

        copy = strdup(cmd);
        token = strtok(copy, delimiter);
        for(i=0; token != NULL; i++){
                args[i] = token;
                token = strtok(NULL, delimiter);      
                if (i>15){
                        fprintf(stderr,"Error: too many process arguments\n");
                        return;
                }
        }
        args[i] = NULL;

        pid = fork();
        if (pid == 0){
                /* Child */
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

struct command{
        char* instruction;
        char* flags[17];
        char* arguments[17];
};



int main(void)
{
        char cmd[CMDLINE_MAX];

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

                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                }


                /* Regular command */
                else{
                        singleCommands(cmd);
                }

               
        }

        return EXIT_SUCCESS;
}
