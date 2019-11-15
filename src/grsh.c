#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define CMD_DEL " \t\n"

typedef struct pathnode {
	char *path;
	struct pathnode *next;
} pathnode_t;

pathnode_t *pathnodes = NULL;

void executecmd(char *cmd, char *args, bool async) {
	printf("async: %s, command: %s, args: %s\n", async ? "true" : "false", cmd, args);
	pathnode_t *node = pathnodes;
	while (node != NULL) {
		printf("%s\n", node->path);
		node = node->next;
	}
}

void addPath(const char *str) {
	pathnode_t *node = (pathnode_t *) malloc(sizeof(pathnode_t));
	node->path = (char*) malloc(sizeof(char) * strlen(str));
	node->next = pathnodes;
	strcpy(node->path, str);
	pathnodes = node;
}

int main(int argc, char *argv[]) {
	addPath("/bin");

	char *buff = NULL;
	size_t size;
	FILE *f = stdin;
	int isCLI = argc == 1;
	if (isCLI) {
		printf("grsh> ");
	} else {
		f = fopen(argv[1], "r");
		if (f == NULL) {
			printf("grsh: cannot open file\n");
			return 1;
		}
	}
	while (getline(&buff, &size, f) != -1) {
		char *saveptr = NULL;
		char *nextcmd = strtok_r(buff, "&", &saveptr);
		char *cmd = nextcmd;
		nextcmd = strtok_r(NULL, "&", &saveptr);
		while (cmd != NULL) {
			while (cmd[0] == ' ') {
				cmd ++;
			}
			char *savecmdptr = NULL;
			char *token = strtok_r(cmd, CMD_DEL, &savecmdptr);
			if (token != NULL) {
				if (strcmp(token, "exit") == 0) {
					return 0;
				} else if (strcmp(token, "cd") == 0) {
					token = strtok_r(NULL, CMD_DEL, &savecmdptr);
					if (chdir(token) == -1) {
						printf("cd: command failed.");
					}
				} else if (strcmp(token, "path") == 0) {
					while ((token = strtok_r(NULL, CMD_DEL, &savecmdptr)) != NULL) {
						addPath(token);
					}
				} else {
					executecmd(token, &cmd[strlen(token)+1], true);
				}
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
