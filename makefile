# Generate executable
sshell: sshell.o dir_stack.o
	gcc -Wall -Wextra -Werror -o sshell sshell.o

# Generate objects files from C files
sshell.o: sshell.c dir_stack.o
	gcc -Wall -Wextra -Werror -c -o sshell.o sshell.c 

dir_stack.o: dir_stack.c dir_stack.h
	gcc -Wall -Wextra -Werror -c -o dir_stack.o dir_stack.c

# Clean generated files
clean:
	rm -f sshell sshell.o dir_stack.o