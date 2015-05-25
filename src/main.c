#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define CMDS_DELIM ";"


char **build_argv(char *cmd) {
	char **argv = malloc((strlen(cmd)+1)*sizeof(*argv));

	if (argv == NULL) {
		return NULL;
	}

	size_t i = 0;
	char *next_arg;
	char *saveptr = NULL;

	next_arg = strtok_r(cmd, " ", &saveptr);
	while (next_arg != NULL) {
		argv[i++] = next_arg;
		next_arg = strtok_r(NULL, " ", &saveptr);
	}

	argv[i] = NULL;

	return argv;
}


void process_cmd(char *cmd) {
	char **argv = build_argv(cmd);

	if (argv == NULL) {
		printf("Null argv\n");
		return;
	}

	printf("Processed command: %s\n", cmd);
	size_t i;
	for (i = 0; argv[i] != NULL; i++) {
		printf("argv[%zu] = %s\n", i, argv[i]);
	}
}

void process_cmd_line(char *cmd_line) {
	static const char *delim = CMDS_DELIM;
	char *saveptr = NULL;
	char *next_tok;

	if ((next_tok = strtok_r(cmd_line, delim, &saveptr)) == NULL) {
		return;
	}

	process_cmd(next_tok);

	while ((next_tok = strtok_r(NULL, delim, &saveptr)) != NULL) {
		process_cmd(next_tok);
	}
}

int main(void) {
	char *line = NULL;
	size_t buf_sz = 0;
	ssize_t line_len = 0;

	printf("fsh: > ");
	errno = 0;
	while ((line_len = getline(&line, &buf_sz, stdin)) != -1) {
		if (line_len > 0 && line[line_len-1] == '\n') {
			line[line_len-1] = '\0';
			line_len--;
		}

		if (line_len > 0 && line[line_len-1] == '\r') {
			line[line_len-1] = '\0';
			line_len--;
		}

		if (!strcmp(line, "exit")) {
			break;
		}

		if (line_len > 0) {
			process_cmd_line(line);
		}

		printf("fsh: > ");
		errno = 0;
	}

	if (errno != 0) {
		perror("Error reading line from stdin");
	}

	free(line);

	return 0;
}
