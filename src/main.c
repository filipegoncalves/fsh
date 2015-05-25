#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CMDS_DELIM ";"

void execute_cmd(char *argv[]) {
	pid_t f;

	if ((f = fork()) < 0) {
		perror("Couldn't fork");
		return;
	}

	if (f == 0) {
		if (execvp(argv[0], argv) < 0) {
			// TODO Enhance error reporting
			perror("execvp() failed");
			exit(0);
		}
	} else {
		waitpid(f, NULL, 0);
	}

}

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
		fprintf(stderr, "Warning: got null argv for command %s\n", cmd);
		return;
	}

	execute_cmd(argv);

	free(argv);
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
