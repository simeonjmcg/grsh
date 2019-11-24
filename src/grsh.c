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

void delete_path(pathnode_t **head, int *len) {
	pathnode_t *current = (*head);
	while (current != NULL) {
		pathnode_t *next = current->next;
		free(current->path);
		free(current);
		current = next;
	}
	(*head) = NULL;
	(*len) = 0;
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

pathnode_t * add_path(const char *str, int *len) {
	int str_len = strlen(str);
	if (str_len > *len) {
		(*len) = str_len;
	}
	pathnode_t *n = (pathnode_t *) malloc(sizeof(pathnode_t));
	n->path = (char*) malloc(sizeof(char) * str_len + 1);
	n->next = NULL;
	strcpy(n->path, str);
	return n;
}

void pathjoin(char *dest, const char *path1, const char *path2, int len) {
	strncpy(dest, path1, len);
	len -= strlen(path1);
	strncat(dest, "/", len);
	len -= 1;
	strncat(dest, path2, len);
}

void get_full_path(char *dest, int len, pathnode_t *path, char *cmd) {
	pathnode_t *node = path;
	if (access(cmd, X_OK) == 0) {
		strncpy(dest, cmd, len);
		return;
	} else {
		while (node != NULL) {
			pathjoin(dest, node->path, cmd, len);
			if (access(dest, X_OK) == 0) {
				return;
			}
			node = node->next;
		}
	}
	// has not been found, reset first byte so strlen=0
	dest[0] = 0;
}

int count_words (char *str) {
	if (str == NULL) {
		return 0;
	}
	int numWords = 0;
	bool isWord = false;
	for (int i=0, l=strlen(str); i<l; i++) {
		if (isspace(str[i]) && isWord) {
			isWord = false;
		} else if (!isspace(str[i]) && !isWord) {
			numWords ++;
			isWord = true;
		}
	}
	return numWords;
}

void split_args (char ** dest, int len, char *cmd, char *argstr) {
	if (len < 2) {
		return;
	}
	dest[0] = cmd;
	dest[len-1] = NULL;
	if (argstr == NULL) {
		return;
	}
	int idx = 1;
	char *saveptr = NULL;
	char * arg = strtok_r(argstr, WHITESPACE, &saveptr);
	while (arg != NULL && idx < len - 1) {
		dest[idx++] = arg;
		arg = strtok_r(NULL, WHITESPACE, &saveptr);
	}
}

void execute_cmd (char *full_cmd, char **argv, char *output, bool async) {
	char *out = strdup(output);
	int id = fork();
	if (id < 0) {
		char error_message[30] = "An error has occurred\n";
		write(STDERR_FILENO, error_message, strlen(error_message));
	}
	if (id == 0) { // child
		if (out != NULL && strlen(out) > 0) {
			int id = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			dup2(id, STDOUT_FILENO);
			dup2(id, STDERR_FILENO);
		}
		int err = execv(full_cmd, argv);
		exit(err);
		return;
	} else {
		if (!async) {
			waitpid(id, NULL, 0);
		}
	}
}

void process_cmd (pathnode_t **path, int *max_path, char *cmd, char *argstr, bool async) {
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
		delete_path(path, max_path);

		pathnode_t * curr_node = NULL;
		char *saveptr = NULL;
		char *arg = strtok_r(argstr, WHITESPACE, &saveptr);
		while (arg != NULL) {
			pathnode_t * node = add_path(arg, max_path);
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
		int full_path_len = (*max_path) + 1 + strlen(cmd) + 1;
		char *full_path = (char *) malloc (sizeof (char) * full_path_len);
		get_full_path(full_path, full_path_len, *path, cmd);

		if (strlen(full_path) == 0) {
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

		int len = count_words(args) + 2;
		char **argv = (char **) malloc(sizeof(char *) * len);
		split_args(argv, len, cmd, args);
		execute_cmd(full_path, argv, output_file, async);
		free(argv);
		free(full_path);
	}
}

int main(int argc, char *argv[]) {
	if (argc > 2) {
		char error_message[30] = "An error has occurred\n";
		write(STDERR_FILENO, error_message, strlen(error_message));
		return 1;
	}
	int max_path = 0;
	pathnode_t *pathnodes = add_path("/bin", &max_path);

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
				process_cmd(&pathnodes, &max_path, cmd, rest, nextcmd != NULL);
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
