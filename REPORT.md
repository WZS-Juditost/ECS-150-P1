# SSHELL: Simple Shell

## Summary

`sshell` is a program that receives a Linux command input from the user and
executes  it. Also, It returns the resulting status value and context string to
the user.

## Implementation

Implementation of shell follows 3 steps:

1. Get the command from the command line and check if any error exists.
    If there is an error, the shell will restart it.
2. if there is no error, check the command line and execute the command
    according to the condition.
3. Get the command execution result and return the command execution
    status and context string.

### Command input and parsing the command line
The first thing in the shell's cycle is to take the command line from the user 
and detach it. Ideally, command lines are separated by a space bar or symbols: 
a pipe, an input sign, and an output sign. But we know that some users are 
lazy. They don't separate the command lines very well and send them all at 
once. This behavior interferes with parsing. So before parsing, we call 
function `spacer` to add a spacebar next to the three symbols. Thanks to this, 
we can easily parse any command. The first symbol to parse is the pipe. If we 
don't have a pipe sign in input, this would be a simple command. But if we have 
a pipe symbol, we store it in `pipe_num`. It's an important variable that 
affects how many child we make later on.

### Command execution
After parsing the command, now we need to execute the command and pass the 
arguments to the exec() function. This time, we will use `execvp()` to execute 
the command because this is the most simple function in exec() family.

At first, we will check the `pipe_num`. If it is greater than 1, then we will 
make pipelines. The following will be explained in more detail in the pipeline 
section below.

If not, we will create child[0] and execute the command. There are some given 
command such as `ls` and `date` functions that we don't need to implement. But 
there are some other command that we need to implement in our shell.c 

#### built in command
we have three built in command: `exit`, `pwd` and `cd`. The `exit` command will 
close the shell. The `pwd` command shows the current working directory. The `cd` 
command changes the working directory to the input directory.

#### Directory stack
Directory stack is a stack thac contains all directory we visited. stack is 
implemented with the linked list. At beginning of sshell, we initialize the 
stack with the current directory and push to the stack. The command `dirs` shows 
whole directory stack elements. The command `pushd` changes the current 
directory to input and push the new directory to the stack. If we can not move 
to the new directory, then we shows error message. The command `popd` pops one 
stack element and changes the directory to the top of the stack. If we can not 
pops the stack element, then we show error message.

### redirection

sshell contains two kinds of redirections: output redirection and input 
redirection. The output redirection is indicated by meta-character `>` and 
followed by a file name. Such operation relocates the standard output to the 
designated file so that the command before `>` sends the output to that file. 
We implement it by fist calling `num_detect` to detect `>` in the command line 
and return an interger `out_num`. We check if `out_num == 1` to determine 
whether the function finds the meta_character or not. If it does, then we 
introduces a different parsing procsss. We parse the command line by `>`. Then, 
pass the first arguments, which is the commands, and the second arguments, 
which is the file name into `Output_redirection`. Inside this function, we 
acquire the file discriptor of the file and duplicate it to standard output. 
Later in the program, we will check `out_num` again when we call 
`create_a_child_for_redi` which is the function that creates the child when 
there is a redirection.

The input redirection is indicated by meta-character `<` and followed by a file 
name. Such operation relocates the standard input to the designated file so 
that instead of reading form standard input,the command before `<` reads from 
that file. The implementation of input redirection is similar to the output 
redirection. `num_detect` returns `in_num` which we use to decide to parse the 
command line by `<`. Then we pass arguments into `Input_redirection` to get the 
file discriptor and duplicate it to standard input. The rest of the part is the 
same. We use `in_num` again to determine to call `create_a_child_for_redi`.

### Pipeline

Using a pipeline, several commands may be linked to one another inside the same 
command line. The meta-character `|` designates the pipe sign. A pipe symbol tells 
the shell that the output of the command that comes before it has to be linked to 
the input of the command that comes after it. The sshell allows a maximum of three 
pipe symbols on a single command line to link together different commands. Built-in 
commands are not supported as part of the pipeline.

We parse the command line entered by the user by `|` and pass each argument to 
another parsing process. We stored everything we get from parsing by ` ` into 
correspond structs. Then we create all the child processes by passing the structs 
into `create_a_child` in a for loop and duplicate standard input and standard 
output to the file discriptor we create for the pipes. For the parent, we also 
apply a for loop to call `waitpid` to make the parent waits for every child. 
Finally, there is a function `close_all_pipes` to close all the pipes we create.

### return value

If `execvp` executes successfully(which means there is no error in the command line 
entered by user and froking process succeeds), the output will usually be printed 
the standard output and displays on the screen(exception: redirection). This a what 
happens in the child process. We use `waitpid` in the parent process to obtain each 
child's status and `fprintf` to print the message confirming the command's 
successful execution and the state of each child's return to standard error.

### Erorr checking

sshell has many input constraints. This protects the shell from incorrect
input from the user and only provides correct input. Adhering to these
principles is important because if we do not adhere to these principles we
will not be able to provide adequate output. Our sshell has many error codes,
and most of these errors are filtered through the `error_check` function and
the `missing command` function.

In most cases, we will know if there is an error before we run the command,
but in some cases, we won't know until we run it. To cover this case, some
functions return an int value. In the main function, the returned value is
stored in the  `error_flag`, an integer variable. The shell judges the
situation through this  stored flag value and returns the appropriate 
information to the screen.

the function `sing_check` is used to determine whether we have input and output 
signs in the proper position. if it is not true, then we do not run the command 
and return an error message.

In the function `create_a_child_for_redi`, we have a flag variable `redi_flag`.
This variable is used to determine whether our input and output redirection is
successful or not. if it is not true, then we do not run the command and stop
fprint.

Copyright 2022, Young Cheol Ko, ZeSheng Wang
