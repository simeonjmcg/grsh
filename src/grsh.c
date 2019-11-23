#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#include <fcntl.h>

#define WHITESPACE " \t\n\v\f\r"

typedef struct pathnode {
	char *path;
	struct pathnode *next;
} pathnode_t;

void deletePath(pathnode_t *n) {
	while (n != NULL) {
		pathnode_t *next = n->next;
		free(n->path);
		free(n);
		n = next;
	}
}

void trim_whitespace(char **str) {
	// trim spaces
	while (isspace((*str)[0])) {
		(*str) ++;
	}
	for (int i=strlen(*str)-1; i >= 0 && isspace((*str)[i]); i--) {
		(*str)[i] = 0;
	}
}

pathnode_t * addPath(const char *str) {
	pathnode_t *n = (pathnode_t *) malloc(sizeof(pathnode_t));
	n->path = (char*) malloc(sizeof(char) * strlen(str));
	strcpy(n->path, str);
	return n;
}

char * pathjoin(char *path1, char *path2) {
	char *dest = (char *)malloc(sizeof(char) * (strlen(path1) + strlen(path2) + 1));
	strcpy(dest, path1);
	strcat(dest, "/");
	strcat(dest, path2);
	return dest;
}

char * get_full_path(pathnode_t *path, char *cmd) {
	pathnode_t *node = path;
	if (access(cmd, X_OK) == 0) {
		return cmd;
	} else {
		while (node != NULL) {
			char *full = pathjoin(node->path, cmd);
			if (access(full, X_OK) == 0) {
				return full;
			}
			free (full);
			node = node->next;
		}
	}
	return NULL;
}

int count_words (char *str) {
	if (str == NULL) {
		return 0;
	}
	int numWords = 0;
	bool isWord = false;
	while(*str != 0) {
		if (isspace(*str) && isWord) {
			numWords ++;
			isWord = false;
		} else if (!isspace(*str) && !isWord) {
			isWord = true;
		}
		str++;
	}
	return numWords;
}

char ** split_args (char *cmd, char *argstr) {
	int len = count_words(argstr) + 1;
	char **args = (char **) malloc(sizeof(char *) * len + 1);
	
	char *saveptr = NULL;
	args[0] = cmd;
	args[len] = NULL;
	if (argstr == NULL) {
		return args;
	}
	int idx = 1;
	char * arg = strtok_r(argstr, WHITESPACE, &saveptr);
	while (arg != NULL && idx <= len) {
		args[idx++] = arg;
		arg = strtok_r(NULL, WHITESPACE, &saveptr);
	}
	return args;
}

void execute_cmd (char *full_cmd, char **argv, char *output, bool async) {
	char *out = strdup(output);
	int id = fork();
	if (id == 0) { // child
		if (out != NULL && strlen(out) > 0) {
			close(STDOUT_FILENO);
			close(STDERR_FILENO);
			open(out, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			open(out, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		}
		execv(full_cmd, argv);
	} else {
		 if (!async) {
			 waitpid(id, NULL, 0);
		 }
	}
}

void process_cmd (pathnode_t **path, char *cmd, char *argstr, bool async) {
	if (strcmp(cmd, "exit") == 0) {
		exit(0);
		return;
	} else if (strcmp(cmd, "cd") == 0) {
		if (argstr != NULL) {
			trim_whitespace(&argstr);
		}
		if (chdir(argstr) == -1) {
			char error_message[30] = "An error has occurred\n";
			write(STDERR_FILENO, error_message, strlen(error_message));
		}
	} else if (strcmp(cmd, "path") == 0) {
		deletePath(*path);
		(*path) = NULL;
		pathnode_t * curr_node = NULL;
		char *saveptr = NULL;
		char *arg = strtok_r(argstr, WHITESPACE, &saveptr);
		while (arg != NULL) {
			pathnode_t * node = addPath(arg);
			if (curr_node == NULL) {
				(*path) = node;
				curr_node = (*path);
			} else {
				curr_node->next = node;
				curr_node = curr_node->next;
			}
			arg = strtok_r(NULL, WHITESPACE, &saveptr);
		}
	} else if(strlen(cmd) > 0) {
		char *full_path = get_full_path(*path, cmd);
		
		if (full_path == NULL) {
			char error_message[30] = "An error has occurred\n";
			write(STDERR_FILENO, error_message, strlen(error_message));
			return;
		}

		char *rest = NULL;
		char *args = NULL;
		char *output_file = NULL;
		while (isspace(argstr[0])) {
			argstr++;
		}
		if (argstr[0] == '>') {
			output_file = argstr + 1;
		} else {
			args = strtok_r(argstr, ">", &rest);
			output_file = rest;
		}
		if (output_file != NULL) {
			trim_whitespace(&output_file);
		}

		char **argv = split_args(cmd, args);
		execute_cmd(full_path, argv, output_file, async);
		free(argv);
	}
}

int main(int argc, char *argv[]) {
	pathnode_t *pathnodes = addPath("/bin");

	char *buff = NULL;
	size_t size;

	FILE *f = stdin;
	int isCLI = argc == 1;
	if (isCLI) {
		printf("grsh> ");
	} else {
		f = fopen(argv[1], "r");
		if (f == NULL) {
			char error_message[30] = "An error has occurred\n";
			write(STDERR_FILENO, error_message, strlen(error_message));
			return 1;
		}
	}
	while (getline(&buff, &size, f) != -1) {
		char *saveptr = NULL;
		char *nextcmd = strtok_r(buff, "&", &saveptr);
		char *cmd = nextcmd;
		nextcmd = strtok_r(NULL, "&", &saveptr);
		while (cmd != NULL) {
			trim_whitespace(&cmd);
			char *rest = NULL;
			char *token = strtok_r(cmd, WHITESPACE, &rest);
			if (token != NULL) {
				process_cmd(&pathnodes, cmd, rest, nextcmd != NULL);
			}
			cmd = nextcmd;
			nextcmd = strtok_r(NULL, "&", &saveptr);
		}
		if (isCLI) {
			printf("grsh> ");
		}
	}
	return 0;
}
