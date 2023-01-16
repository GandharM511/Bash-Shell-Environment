#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

int main(void)
{
        char cmd[CMDLINE_MAX];

        while (1) {
                char *nl;
                char *copy;
                char *token;
                pid_t pid;
                char *args[17];
                const char delimiter[2]= " ";
                int i;
                

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
                        
                        copy = strdup(cmd);
                        token = strtok(copy, delimiter);
                        for(i=0; token != NULL; i++){
                                args[i] = token;
                                token = strtok(NULL, delimiter);      
                        }
                        args[i] = NULL;

                        pid = fork();
                        if (pid == 0){
                                /* Child */
                                int ret = execvp(args[0], args); //execvp since it searches the command utilizing $PATH
                                fprintf(stdout,"%d\n", ret);
                                perror("execvp");
                                exit(40);              
                        } else if (pid > 0) {
                                /* Parent */
                                int status;
                                waitpid(pid, &status, 0);
                                fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(status));

                        }
                }

               
        }

        return EXIT_SUCCESS;
}
