sshell: sshell.c
	gcc -Wall -Wextra -Werror -o $@ $^
clean:
	rm -f sshell *.o

