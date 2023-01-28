# SSHELL: Simple Shell 

## Summary 
This program, `sshell`, provides a shell
environment where one could run a variety of commands, programs, and scripts.
`sshell` can support from no argument commands up to a pipeline of four
commands. It also supports background jobs and output redirection, with an
appending option.

## Implementation 
The implementation of `sshell` follows these distinct steps:
Fetching the command line input Parsing the input for separate commands,
arguments, and/or output files Creating child processes to execute each command
Monitoring child processes’ exit statuses and printing a completion message for
the user

## Parsing 
After the command line input is fetched, `sshell` parses the input for
pipes using `strtok()` to differentiate multiple commands, then parses for the
output redirection metacharacter `>`, and finally parses through each command to
differentiate instructions and arguments. Lastly `sshell` builds a data
structure for commands, called `struct Command`,  and populates each object with
its instructions, arguments, and its output file descriptor. While doing these
steps, `sshell` checks for a variety of parsing errors to ensure the user input
is valid. Additionally, `sshell` checks if an input command has a background
flag ‘&’.

## Built-in Commands 
`sshell` has three built-in commands, meaning commands that
are executed within the shell by the parent process. These builtin-commands are
`pwd`, `cd`, and `exit`. When `sshell` receives a `pwd` command it performs a
syscall to find the current directory from the kernel, and prints it to the
command line using `getcwd(buf,buf_size)`, which takes in a buffer to store the
path and the size of said buffer.  To implement `cd`, `sshell` utilizes the
`chdir()` system function, to change the directory to the specified input path.
`chdir()` takes in the pathway in the form of a string, and returns -1 if it
fails, which `sshell` uses to tell the user it cannot cd into the specified
directory. `sshell` uses an error_check variable to keep track of if the process
is running as meant to be, and the values of error_check match the format for
the values thrown by `chdir()` to allow for one error code that `sshell` needs
to check. Finally, `sshell` supports shortcuts such as `cd ..`, and `cd .`. 

When the user calls `exit` to the command line, `sshell` terminates itself. It
does this by breaking from the loop that `sshell` is running on, and printing
out an exit status to show a successful termination. However, when there are
background jobs running, `sshell` will check for their existence utilizing
`waitpid()` with the WNOHANG flag. and stop the user from exiting the shell. Once
the background job has finished running, and if the user calls `exit` again,
`sshell` will ensure that the background jobs are done, and then send out the
completion message for the background job, and finally terminating itself after
printing its own `exit` completion message.

## Executable commands 
If the input is none of the built-in commands, the shell
will move on in one of 4 separate ways depending on what type of input it’s
received.  

### Regular commands 
If the shell has received a regular,
non-pipelined, non-background, and non-output redirected input, it will fork the
parent process onto a child process that will execute the command along with its
flags through the `execvp()` system call. The parent process will then wait for
the child to complete execution through the `waitpid()` function, and capture its
exit status. Once this is done, the parent will check for any finished previous
background jobs, which have their information stored in a separate struct data
structure named `struct Job`, and if these have terminated, which has been
checked previously through waitpid and the option WNOHANG, it will print their
completion message, that contains the exit status and the full original input of
that job, as well as its own completion message through a completion function,
that receives the full input, the exit status(es) and the amount of commands
being executed at once, where it will switch between options depending on the
amount of commands.  

### Pipelined commands 
In the case the parent receives
pipelined commands, it will create a pipe for each `|` contained in the input
and then it will iterate through a loop, forking the process once for every
command present in the struct of commands, with the child process connecting
their standard outputs and inputs to their respective pipes if necessary,
depending on their position within the input, so the information can be passed
from one command to the next. Note that all commands are executed concurrently,
each by a separate child, to ensure no buffer overflow in the pipes. Once the
parent process is done iterating it will close all open pipes and use waitpid to
wait for the child processes to terminate and capture their exit statuses, and
then proceed to check for previously finished background jobs and print the
completion messages and previously explained.  

### Output-redirected and Background commands 
Since the `Command` struct contains a member, `outputfd`,
specifically for changing where the output of the child is written, the child
process will always redirect the output to this variable, however,  as it is
defaulted to the STDOUT_FILENO which is connected to the terminal, it will keep
standard output the same value if outputfd has not been changed. It will then
proceed to execute the child and print its completion message the same way as a
regular command.  The same idea follows for background commands, where the only
primary difference between this and regular commands is that the parent process
will not wait for the child processes to finish, freeing the shell to receive
more inputs, and will instead add their pid to the `Job` data structure so that
it can be observed and have it’s completion message printed in the future by the
shell once it’s received more inputs.
