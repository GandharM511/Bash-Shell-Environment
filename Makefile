sshell: sshell.c
	gcc -g -Wall -Wextra -Werror -o $@ $^
clean:
	rm -f sshell *.o
