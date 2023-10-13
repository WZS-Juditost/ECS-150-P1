#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "dir_stack.c"

#define CMDLINE_MAX 512
#define PRG_ARG_MAX 16
#define TKN_LEN_Max 32
#define PIP_MAX 3

typedef struct shl_st {
	int fd, num, in_meta_num, out_meta_num;
	pid_t pid;
	int pipe_fd[3][2];

    char cmd[CMDLINE_MAX];
    char **args;
	char **in_out_redi;

}shl_st;

enum {
	TOO_MUCH_ARGS,
	MISSING_CMD,
	CMD_NOT_FOUND,
	NO_OUT_FILES,
	NO_IN_FILES,
	FAIL_TO_OPEN_IN,
	FAIL_TO_OPEN_OUT,
	MISLOCATED_INPUT,
	MISLOCATED_OUTPUT,
	CANNOT_CD_TO_DIR,
	NO_DIR,
	STK_EMPTY
};

void error_message(int error_code) {
	switch (error_code) {
		case TOO_MUCH_ARGS:
			fprintf(stderr, "Error: too many process arguments\n");
			break;
		case MISSING_CMD:
			fprintf(stderr, "Error: missing command\n");
			break;
		case CMD_NOT_FOUND:
			fprintf(stderr, "Error: command not found\n");
			break;
		case NO_OUT_FILES:
			fprintf(stderr, "Error: no output file\n");
			break;
		case NO_IN_FILES:
			fprintf(stderr, "Error: no input file\n");
			break;
		case FAIL_TO_OPEN_IN:
			fprintf(stderr, "Error: cannot open input file\n");
			break;
		case FAIL_TO_OPEN_OUT:
			fprintf(stderr, "Error: cannot open output file\n");
			break;
		case MISLOCATED_INPUT:
			fprintf(stderr, "Error: mislocated INput redirection\n");
			break;
		case MISLOCATED_OUTPUT:
			fprintf(stderr, "Error: mislocated output redirection\n");
			break;
		case CANNOT_CD_TO_DIR:
			fprintf(stderr, "Error: cannot cd into directory\n");
			break;
		case NO_DIR:
			fprintf(stderr, "Error: no such directory\n");
			break;
		case STK_EMPTY:
			fprintf(stderr, "Error: directory stack empty\n");
			break;
	}
}

void copy_array (shl_st *dest, shl_st *src) {
	int i = 0;
	while (src->in_out_redi[i] != NULL) {
		strcpy(dest->args[i], src->in_out_redi[i]);
		i++;
	}
	dest->args[i] = NULL;
}

/* parse */
char **parse (char *cmd, const char *deli) {
	char **tkn_List = malloc(PRG_ARG_MAX*sizeof(char*));
	char *tkn = strtok(cmd, deli);

	int i = 0;
	while (tkn != NULL) {
		tkn_List[i] = tkn;
		tkn = strtok(NULL,deli);
		i++;
	}
	tkn_List[i] = NULL;

	return tkn_List;
}

void close_all_pipes(int fd1[3][2]) {
	close(fd1[0][0]);
	close(fd1[0][1]);
	close(fd1[1][0]);
	close(fd1[1][1]);
	close(fd1[2][0]);
	close(fd1[2][1]);
}

int num_detect (char *cmd, char target) {
    int count = 0;
    int i = 0;
    while (cmd[i]!='\0') {
        if (cmd[i]==target){
            count++;
        }
        i++;
    }
    return count;    
}

int sign_pos_finder (shl_st *cmd, char c) {
	int index;
	for(int i = 0; cmd->args[i]!=NULL; i++){
		index = num_detect(cmd->args[i], c);
		if (index == 1){
			return i;
		}
	}
	return -1;
}

int missing_command (shl_st *cmdLine) {
    char *sign[3] = {"|",">","<"};
	int k = 0;
	while (k < 3){
		if (!strcmp(cmdLine->args[0],sign[k])) {
			error_message(MISSING_CMD);
			return 1;
		}
		int pos = sign_pos_finder(cmdLine, *sign[k]);
		if (pos != -1) {
			if (!strcmp(cmdLine->args[pos-1],sign[k])) {
				error_message(MISSING_CMD);
				return 1;
			}
			else if ((pos+1==cmdLine->fd)) {
				if (k==1) {
					error_message(NO_OUT_FILES);
					return 1;
				}
				if (k==2) {
					error_message(NO_IN_FILES);
					return 1;
				}
				else {
					error_message(MISSING_CMD);
					return 1;
				}
			}
			else if (!strcmp(cmdLine->args[pos+1],sign[k])){
				error_message(MISSING_CMD);
				return 1;
			}
		}
		k++;
	}
	return 0;
}

char* spacer(char* origin) {
    char *old_out[3] = {"|",">","<"};
    char *new_out[3] = {" | "," > "," < "};
    int oldWlen = strlen(old_out[0]);
    int newWlen = strlen(new_out[0]);
	char* temp = origin;
	char* result;

	int k = 0;
	while (k < 3){
		int i = 0;
		for (i = 0; temp[i] != '\0'; i++) {
			if (strstr(&temp[i], old_out[k]) == &temp[i]) {
				i += oldWlen - 1;
			}
		}
		result = (char*)malloc(i + (3 - 1) + 1);
		i = 0;
		while (*temp) {
			if (strstr(temp, old_out[k]) == temp) {
				strcpy(&result[i], new_out[k]);
				i += newWlen;
				temp += oldWlen;
			}
			else
				result[i++] = *temp++;
		}
		result[i] = '\0';
		temp = result;
		k++;
	}
    return result;
}

int error_check (shl_st *cmdLine) {
	if (cmdLine->cmd[0]=='\0') {
		error_message(MISSING_CMD);
		return 1;
	}

	char* cmd_wSpace = spacer(cmdLine->cmd);
	cmdLine->args = parse(cmd_wSpace, " ");

	int j = 0;
	while (cmdLine->args[j]!=NULL) {
		j++;
	}
	cmdLine->fd = j;

	if (cmdLine->fd > PRG_ARG_MAX) {
		error_message(TOO_MUCH_ARGS);
		return 1;
	}
	return missing_command(cmdLine);
}

int Output_redirection (shl_st *out) {
	char** file_name = parse(out -> args[1], " ");
	out -> fd = open(file_name[0], O_WRONLY | O_CREAT, 0644);
	if (out -> fd != -1) {
		dup2(out -> fd, STDOUT_FILENO);
		close(out -> fd);
			return 0;
	}
	else {
		error_message(FAIL_TO_OPEN_OUT);
		return 1;
	}
}

int Input_redirection (shl_st *in) {
	char** file_name = parse(in -> args[1], " ");
	in -> fd = open(file_name[0], O_RDONLY);
	if (in -> fd != -1) {
		dup2(in -> fd, STDIN_FILENO);
		close(in -> fd);
		return 0;
	}
	else {
		error_message(FAIL_TO_OPEN_IN);
		return 1;
	}
}

void create_a_child (shl_st *child, int pipe_num, int fd[3][2]) {
	pid_t pid = fork();

	child->pid = pid;
	for(int i = 0; i < 3; i++) {
		memcpy(child->pipe_fd[i], fd[i], sizeof(fd[i]));
	}

	if (pid < 0) {
		printf("can't fork, error occured\n");
		perror("Error");
		exit(EXIT_FAILURE);
	} else if (pid == 0) {
		if (pipe_num != 0) {
			if (child->num == 0){
        		dup2(fd[child->num][child->num + 1], STDOUT_FILENO);
    		} else if (child->num == pipe_num) {
        		dup2(fd[child->num -1][0], STDIN_FILENO);
    		} else {
				dup2(fd[child->num - 1][0], STDIN_FILENO);
        		dup2(fd[child->num][1], STDOUT_FILENO);
			}
			close_all_pipes(child->pipe_fd);
		}
		execvp(child->args[0], child->args);
	}
}

int create_a_child_for_redi (shl_st *child, shl_st *redirection, int pipe_num, int fd[3][2]) {
	pid_t pid = fork();

	child->pid = pid;
	for(int i = 0; i < 3; i++) {
		memcpy(child->pipe_fd[i], fd[i], sizeof(fd[i]));
	}

	if (pid < 0) {
		printf("can't fork, error occured\n");
		perror("Error");
		exit(EXIT_FAILURE);
	} else if (pid == 0) {
		if (pipe_num != 0) {
			if (child->num == 0){
        		dup2(fd[child->num][child->num + 1], STDOUT_FILENO);
    		} else if (child->num == pipe_num) {
        		dup2(fd[child->num -1][0], STDIN_FILENO);
    		} else {
				dup2(fd[child->num - 1][0], STDIN_FILENO);
        		dup2(fd[child->num][1], STDOUT_FILENO);
			}
			close_all_pipes(child->pipe_fd);
		}
		
		int redi_flags = 0;
		if(child[0].out_meta_num == 1) {
			redi_flags = Output_redirection(redirection);
		} else if(child[0].in_meta_num == 1) {
            redi_flags = Input_redirection(redirection);
        }
		if (!redi_flags) {
			execvp(child->args[0], child->args);
			return 0;
		}
		exit(1);
	}
	return 1;
}

int sign_check (shl_st *child, int pipe_num) {
	int sign_flag = 0;
	for (int i = 1; i <= pipe_num; i++) {
		for (int j = 0; child[i].args[j] != NULL; j++) {
			sign_flag = num_detect(child[i].args[j], '<');
			if (sign_flag == 1) {
				error_message(MISLOCATED_INPUT);
				return 1;
			}
		}
	}

	for (int i = 0; i < pipe_num; i++) {
		for (int j = 0; child[i].args[j] != NULL; j++) {
			sign_flag = num_detect(child[i].args[j], '>');
			if (sign_flag == 1) {
				error_message(MISLOCATED_OUTPUT);
				return 1;
			}
		}
	}
	return 0;
}

int built_in_exit(char* cmd, shl_st *cmdline) {
	if (!strcmp(cmd, "exit")) {
		fprintf(stderr, "Bye...\n");
		fprintf(stderr, "+ completed '%s' [%d]\n", cmdline->cmd, 0);
		return 1;
	}
	return 0;
}

void built_in_pwd(char* dir, char *dirBuffer, shl_st *child) {
	dir = getcwd(dirBuffer, CMDLINE_MAX);
	fprintf(stdout, "%s\n", dir);
	fprintf(stderr, "+ completed '%s' [%d]\n", child[0].args[0], 0);
}

void built_in_cd(shl_st *child, shl_st *cmdline) {
	int status = chdir(child[0].args[1]);
	if (status == -1){
		error_message(CANNOT_CD_TO_DIR);
		fprintf(stderr, "+ completed '%s' [%d]\n", cmdline->cmd, 1);
	} else {
		fprintf(stderr, "+ completed '%s' [%d]\n", cmdline->cmd, WEXITSTATUS(status));
	}
}

void built_in_dirs(struct dir_Stack *directory, shl_st *cmdline) {
	display(directory);
	fprintf(stderr, "+ completed '%s' [%d]\n", cmdline->cmd, 0);
}

void built_in_pushd(struct dir_Stack *directory, shl_st *child, shl_st *cmdline, char* dir, char *dirBuffer) {
	int status = chdir(child[0].args[1]);
	if (status == -1){
		error_message(NO_DIR);
		fprintf(stderr, "+ completed '%s' [%d]\n", cmdline->cmd, 1);
	} else {
		dir = getcwd(dirBuffer, CMDLINE_MAX);
		push(directory, dir);
		fprintf(stderr, "+ completed '%s' [%d]\n", cmdline->cmd, WEXITSTATUS(status));
	}
}

void built_in_popd(struct dir_Stack *directory, shl_st *cmdline) {
	if (directory->stackSize > 1) {
		pop(directory);
		chdir(top(directory));
		fprintf(stderr, "+ completed '%s' [%d]\n", cmdline->cmd, 0);
	}
	else {
		error_message(STK_EMPTY);
		fprintf(stderr, "+ completed '%s' [%d]\n", cmdline->cmd, 1);
	}
}

void initialize_redinum(shl_st *child) {
    for (int i = 0; i < 4; i++) {
        child[i].out_meta_num = 0;
        child[i].in_meta_num = 0;
	}
}

int main(void)
{
	shl_st cmdLine, in, out;
	shl_st child[4];
	initialize_redinum(child);

	char cmd[CMDLINE_MAX];
	char *dir;
	char dirBuffer[CMDLINE_MAX] = {0,};
	const char space[2] = " ";
	const char pipeS[2] = "|";
	const char inputS[2] = "<";
	const char outputS[2] = ">";

	int fd[3][2];
	int pipe_num, in_num, out_num;

	/* initialize stack*/
	struct dir_Stack *directory = newStack();
	dir = getcwd(dirBuffer, CMDLINE_MAX);
	push(directory, dir);

	while (1) {
		char *nl;
		int error_flag = 0;

		/* Print prompt */
		printf("sshell$ucd ");
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

		strcpy(cmdLine.cmd, cmd);
		pipe_num = num_detect(cmd, '|');
		out_num = num_detect(cmd, '>');
		in_num = num_detect(cmd, '<');

		error_flag = error_check(&cmdLine);

		if (error_flag != 1) {
			/* parsing the command line*/
			if (pipe_num > 0) {
				cmdLine.args = parse(cmd, pipeS);
				strcpy(child[0].cmd, cmdLine.args[0]);
				child[0].args = parse(cmdLine.args[0], space);
			}else {
				strcpy(child[0].cmd, cmd);
				child[0].args = parse(cmd, space);
			}
			child[0].num = 0;
			
			for(int i = 1; i < pipe_num + 1; i++) {
				strcpy(child[i].cmd, cmdLine.args[i]);
				child[i].args = parse(cmdLine.args[i], space);
				child[i].num = i;
			}

			/* Builtin command */
			if (pipe_num == 0) {
				if (built_in_exit(cmd, &cmdLine)) {
					break;
				}
				else if(!strcmp(child[0].args[0], "pwd")) {
					built_in_pwd(dir, dirBuffer, &child[0]);
				}
				else if(!strcmp(child[0].args[0], "cd")) {
					built_in_cd(&child[0], &cmdLine);
				}
				else if(!strcmp(child[0].args[0],"dirs")) {
					built_in_dirs(directory, &cmdLine);
				}
				else if(!strcmp(child[0].args[0],"pushd")) {
					built_in_pushd(directory, &child[0], &cmdLine, dir, dirBuffer);
				}
				else if(!strcmp(child[0].args[0],"popd")) {
					built_in_popd(directory, &cmdLine);
				}
				else{
					int cmd_flag = 0;
					if (out_num == 1) {
						child[0].out_meta_num = out_num;
						out.args = parse(child[0].cmd, outputS);
						out.in_out_redi = parse(out.args[0], space);
						copy_array(&child[0], &out);
						cmd_flag = create_a_child_for_redi(&child[0], &out, pipe_num, fd);
					} else if (in_num == 1) {
                    	child[0].in_meta_num = in_num;
						in.args = parse(child[0].cmd, inputS);
						in.in_out_redi = parse(in.args[0], space);
						copy_array(&child[0], &in);
						cmd_flag = create_a_child_for_redi(&child[0], &in, pipe_num, fd);
                	}else {
						create_a_child(&child[0], pipe_num, fd);
					}
					int status;
					waitpid(child[0].pid, &status, 0);
					if(!cmd_flag){
						fprintf(stderr, "+ completed '%s' [%d]\n", cmdLine.cmd, WEXITSTATUS(status));
					}
				}
			}
			else if (pipe_num > 0 && !sign_check(child , pipe_num)){
				for(int i = 0; i < 3; i++){
					if (pipe(fd[i]) == -1) {
						return 1;
					}
				}
				int cmd_flag = 0;
				if (in_num == 1) {
                	child[0].in_meta_num = in_num;
					in.args = parse(child[0].cmd, inputS);
					in.in_out_redi = parse(in.args[0], space);
					copy_array(&child[0], &in);
					cmd_flag = create_a_child_for_redi(&child[0], &in, pipe_num, fd);
            	}else {
					create_a_child(&child[0], pipe_num, fd);
            	}
				for(int i = 1; i < pipe_num; i++) {
					create_a_child(&child[i], pipe_num, fd);
				}
				if (out_num == 1) {
					child[pipe_num].out_meta_num = out_num;
					out.args = parse(child[pipe_num].cmd, outputS);
					out.in_out_redi = parse(out.args[0], space);
					copy_array(&child[pipe_num], &out);
					cmd_flag = create_a_child_for_redi(&child[pipe_num], &out, pipe_num, fd);
				} else {
					create_a_child(&child[pipe_num], pipe_num, fd);
				}
				int status[4];
				close_all_pipes(fd);
				for(int i = 0; i < pipe_num + 1; i++) {
					waitpid(child[i].pid, &status[i], 0);
				}
				if(!cmd_flag){
					// fprint
					fprintf(stderr, "+ completed '%s' ", cmdLine.cmd);
					for (int i = 0; i < pipe_num + 1; i++) {
						fprintf(stderr, "[%d]", WEXITSTATUS(status[i]));
					}
					fprintf(stderr, "\n");
				}
			}
		}
	}
	return EXIT_SUCCESS;
}