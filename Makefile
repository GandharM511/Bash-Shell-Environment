sshell: sshell.c
	gcc -Wall -Werror -o $@ $^
clean:
	rm -f sshell *.o