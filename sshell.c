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

typedef struct
{
        /*
        This struct is used to process Jobs.
        It includes many characteristics used in parsing the input, and keeping track of background processes.
        */
        char *cmds[CMDLINE_MAX];
        int background;
        pid_t pid[4];
        int commandNum;
        pid_t pidReturn[4];
        char fullcommand[CMDLINE_MAX];
        int done;
}Job;

typedef struct
{
        /*
        This struct is used to process each individual command.
        It includes the command instruction, the arguments which include flags, filenames, etc.
        Finally it also includes the outputfd, which is the output file descripter for the specific command.
        */
        char instruction[TOKLEN_MAX];
        char *arguments[ARGS_MAX];
        int outputfd;
}Command;

void completedFunction(char *cmd, int *status, int commandNumber)
{
        switch (commandNumber) {
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

int pipeParser(char *cmd, Job *job)
{
        /*
        This function takes in the command line input via cmd, and the job object
        and parses cmd for pipes and adds each token which signifies a command
        to the array of strings at jobs->cmd[]. It returns the amount of commands in 
        the command line.
        */
        char copy[CMDLINE_MAX];
        char *token;
        const char delimiter[2] = "|";
        int i;

        strcpy(copy, cmd);

        token = strtok(copy, delimiter); 
        /*
        Creating a token from the start of the string until the first pipe, and if 
        no pipes are found then strok() returns NULL.
        */
        for (i = 0; token != NULL; i++) {
                (job->cmds)[i] = token;
                token = strtok(NULL, delimiter);
        }
        return i;
}


int parser(Command *command, char *cmd, int index, char *copy)
{
        /*
        This function takes in the command object, the command line input, the index that 
        refers to the current command being parsed, and a copy of the command line input and
        outputs whether there is a parsing error or not. 
        */
        char *token;
        char *filename;
        int i;
        const char delimiter[2] = " ";

        strcpy(copy, cmd);
        /*
        Check for the append or output redirection meta characters, and generate the file 
        descriptors accordingly.
        */
        char *orindex = strchr(copy, '>');
        char *orindex2 = strrchr(copy, '>');
        if ((orindex && orindex2)&&(orindex2 == orindex+1)) {
                strcpy(copy, strtok(cmd, ">>"));
                strcpy(copy, strtok(NULL, ">>"));
                filename = strtok(copy, delimiter);
                command[index].outputfd = open(filename, O_WRONLY | O_APPEND | O_CREAT,0644);
        } else if (orindex) {
                strcpy(cmd, strtok(copy, ">"));
                filename = strtok(NULL, delimiter);
                command[index].outputfd = open(filename, O_WRONLY | O_CREAT, 0644);

        }
        /*
        Check that the first character is not a meta character used in output redirection
        or piping.
        */
        token = strtok(cmd, delimiter);
        if ((!strcmp(token, ">")) || (!strcmp(token, "<")) || (!strcmp(token, "|"))) {
                fprintf(stderr, "Error: missing command\n");
                return 1;
        }
        /*
        Parse string for instructions and arguments for errors.
        */
        strcpy(command[index].instruction, token);
        for (i = 0; token != NULL; i++) {
                command[index].arguments[i] = token;
                token = strtok(NULL, delimiter);      
                if (i > 15) {
                        fprintf(stderr, "Error: too many process arguments\n");
                        return 1;
                }
        }
        if (!strcmp(command[index].arguments[i-1], ">")) {
                fprintf(stderr, "Error: no output file\n");
                return 1;
        } else if ((!strcmp(command[index].arguments[i-1], "<")) || (!strcmp(command[index].arguments[i-1], "|"))) {
                fprintf(stderr, "Error: missing command\n");
                return 1;
        } 
        (command[index].arguments)[i] = NULL;
       
        return 0;
}


void execute(char *cmd, char *copy, Job *job)
{
        /*
        This function takes in the command line input, copy of the command line input, and
        job object. It calls on both of the parsers and executes the commands stored in the 
        Command struct. Calls the completion messages function and monitors background jobs. 
        It also includes the inbuilt function of cd.
        */
        
        int backstatus[PIPE_MAX];
        /*
        If exit is called when a background job may be active, check if background job has 
        been terminated, and release the appropriate completion messages.
        */
        if (!strcmp("\0", cmd)) {
                
                if ((job->pid)[0] != 1) {
                        for (int i = 0; i < job->commandNum; i++) {
                                (job->pidReturn)[i] = waitpid((job->pid)[i], &(backstatus[i]), WNOHANG);
                                if (!(job->pidReturn)[i]) {
                                        fprintf(stderr, "Error: active jobs still running\n");
                                        fprintf(stderr, "+ completed 'exit' [%d]\n", 1);
                                        break;
                                }
                                else if (i == (job->commandNum)-1) {
                                        completedFunction(job->fullcommand, backstatus, job->commandNum);
                                        job->pid[0] = 1;
                                        fprintf(stderr, "Bye...\n");
                                        fprintf(stderr, "+ completed 'exit' [%d]\n", 0);
                                }
                        }
                        
                }
                return;
        } 
        
        /*
        Check if previous background jobs have been completed.
        */
        if ((job->pid)[0] != 1) {
                for (int i = 0; i < job->commandNum; i++) {
                       (job->pidReturn)[i] = waitpid((job->pid)[i], &(backstatus[i]), WNOHANG);
                       if (!(job->pidReturn)[i]) {
                                break;
                       }
                       else if (i == (job->commandNum)-1) {
                                job->done = TRUE;
                       }
                }
        }
        
        int commandNumber = pipeParser(cmd, job);

        int fd[commandNumber-1][2]; // commandNumber -1 because if we have 2 commands, we would have 1 pipe etc
        pid_t pid[commandNumber];
        int parsingerror;

        Command *commands;
        commands = malloc(sizeof(Command) * commandNumber);
        /*
        Loop through the commands calling parser on each command.
        */
        for (int i = 0; i < commandNumber; i++) {
                commands[i].outputfd = 1;
                parsingerror = parser(commands, (job->cmds)[i], i, copy);
                if(parsingerror) return;
        }
        /*
        CD Implementation and Error Management
        */
        if (!strcmp(commands[0].instruction, "cd")) {
                int error_check;
                if (commands[0].arguments[1] == NULL) {
                        error_check = -1;
                }
                else if (!strcmp(commands[0].arguments[1], ".")) {
                        error_check = 0;
                }
                else {
                       error_check = chdir(commands[0].arguments[1]);
                }
                if (error_check == -1) {
                        fprintf(stderr, "Error: cannot cd into directory\n");
                        error_check = 1;
                }
                fprintf(stderr, "+ completed '%s' [%d]\n", cmd, error_check);
                return;    
        }
        /*
        Initialize pipes for pipeline commands.
        */
        int j;
        for (j = 0; j < commandNumber-1; j++) {
                pipe(fd[j]);
        }

        /*
        Execute commands
        */
        for (int i = 0; i < commandNumber; i++) { 
                pid[i] = fork();
                if (pid[i] == 0) {
                        /*
                        Output redirected single command
                        */
                        if (commandNumber == 1 && commands[i].outputfd != 1) {
                                dup2(commands[i].outputfd, STDOUT_FILENO);   
                        /*
                        Regular command execution
                        */
                        } else if (commandNumber == 1) {
                                execvp(commands[i].instruction, commands[i].arguments); //execvp since it searches the command utilizing $PATH
                                exit(1);
                        /*
                        First command in pipeline 
                        */    
                        } else if (i == 0) {
                                dup2(fd[i][1], STDOUT_FILENO);
                                for (int j = 0; j < commandNumber-1; j++){
                                        close(fd[j][0]);
                                        close(fd[j][1]);
                                }
                        /*
                        Last command in pipeline.
                        */
                        } else if (i == commandNumber-1) {
                                dup2(fd[i-1][0], STDIN_FILENO);
                                for (int j = 0; j < commandNumber-1; j++) {
                                        close(fd[j][0]);
                                        close(fd[j][1]);
                                }
                                if (commands[i].outputfd != 1) {
                                        dup2(commands[i].outputfd, STDOUT_FILENO);   
                                }
                        /*
                        Middle commands in pipeline.
                        */
                        } else {
                                dup2(fd[i-1][0], STDIN_FILENO);
                                dup2(fd[i][1], STDOUT_FILENO);
                                for (int j = 0; j < commandNumber-1; j++) {
                                        close(fd[j][0]);
                                        close(fd[j][1]);
                                }
                        }   
                        execvp(commands[i].instruction, commands[i].arguments); //execvp since it searches the command utilizing $PATH
                        exit(1);           
                } 
        }

        /*
        Close all pipes.
        */
        int status[commandNumber];
        for (int i = 0; i < commandNumber-1; i++) {
                close(fd[i][1]);
                close(fd[i][0]);
        }
        
        /*
        If not background job        
        */
        if (!(job->background)) {
                /*
                Check and wait for pid and status of child.
                */
                for (int i = 0; i < commandNumber; i++) {
                        waitpid(0, &(status[i]), 0);
                }
                /*
                Check for cat error.
                */
                for (int i = 0; i < commandNumber; i++) {
                        if (WEXITSTATUS(status[i]) == 1 && strcmp(commands[i].instruction, "cat")) {
                                fprintf(stderr, "Error: command not found\n");
                        }
                }
                /*
                Send out completion message for previous background jobs.
                */
                if (job->done && job->pid[0] != 1) {
                        completedFunction(job->fullcommand, backstatus, job->commandNum);
                        job->pid[0] = 1;

                }
                /*
                Send out completion messages for regular jobs.
                */
                completedFunction(cmd, status, commandNumber);
        /*
        Store background jobs information and set background jobs flags.
        */
        } else if (job->background) {
                for (int i = 0; i < commandNumber; i++) {
                        (job->pid)[i] = pid[i];
                }
                job->commandNum = commandNumber;
                job->done = FALSE;
        }
        free(commands);
}

int main(void)
{
        char cmd[CMDLINE_MAX];
        char copy[CMDLINE_MAX];
        char path[CMDLINE_MAX];
        Job job;
        job.pid[0] = 1;
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
                        /*
                        Check for previous or current background jobs.
                        */
                        if (!(job.done)) {
                                execute("\0", "\0", &job);
                                if(job.pid[0] == 1){
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
                        /*
                        Check for background job character.
                        */
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
