#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)
#define CMDS_DELIM ";"

enum redir_mode {
	REDIR_APPEND,
	REDIR_STDIN,
	REDIR_STDOUT
};

void array_delete(char *arr[], size_t begin, size_t del_amount) {
	size_t j = begin+del_amount;
	while ((arr[begin++] = arr[j++]) != NULL);
}

int io_redir(enum redir_mode mode, char *filename) {
	int flags;
	int dup_target;
	if (mode == REDIR_APPEND) {
		flags = O_WRONLY | O_CREAT | O_APPEND;
		dup_target = STDOUT_FILENO;
	} else if (mode == REDIR_STDIN) {
		flags = O_RDONLY;
		dup_target = STDIN_FILENO;
	} else if (mode == REDIR_STDOUT) {
		flags = O_WRONLY | O_CREAT | O_TRUNC;
		dup_target = STDOUT_FILENO;
	} else {
		fprintf(stderr, "Invalid redirection mode\n");
		return -1;
	}

	int fd = open(filename, flags, FILE_MODE);
	if (fd < 0) {
		fprintf(stderr, "Couldn't open %s: ", filename);
		perror(NULL);
		return -1;
	}

	dup2(fd, dup_target);
	close(fd);

	return 0;
}

int do_redirect(char *argv[]) {
	size_t i = 0;

	while (argv[i] != NULL) {
		if (*argv[i] != '<' && *argv[i] != '>') {
			i++;
			continue;
		}

		enum redir_mode mode;

		if (*argv[i] == '>' && argv[i][1] == '>') {
			mode = REDIR_APPEND;
		} else if (*argv[i] == '>') {
			mode = REDIR_STDOUT;
		} else {
			mode = REDIR_STDIN;
		}

		char *filename;
		int to_shift;
		if (mode != REDIR_APPEND) {
			if (argv[i][1] != '\0') {
				filename = &argv[i][1];
				to_shift = 1;
			} else {
				filename = argv[i+1];
				to_shift = 2;
			}
		} else {
			if (argv[i][2] != '\0') {
				filename = &argv[i][2];
				to_shift = 1;
			} else {
				filename = argv[i+1];
				to_shift = 2;
			}
		}

		array_delete(argv, i, to_shift);
		if (io_redir(mode, filename) < 0) {
			return -1;
		}
	}

	return 0;
}

int pipe_exec(char *argv[]) {
	int pipefd[2];
	pid_t f;

	if (pipe(pipefd) < 0) {
		fprintf(stderr, "%s: couldn't create pipe", argv[0]);
		perror(NULL);
		return -1;
	}

	if ((f = fork()) < 0) {
		fprintf(stderr, "%s", argv[0]);
		perror(NULL);
		return -1;
	}

	if (f == 0) {
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[0]);

		if (do_redirect(argv) < 0) {
			exit(EXIT_FAILURE);
		}

		if (execvp(argv[0], argv) < 0) {
			perror("execvp() failed");
			exit(EXIT_FAILURE);
		}
	} else {
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[1]);
	}
	return 0;
}

void execute_cmd(char *argv[]) {
	pid_t f;
	size_t i, j;

	int stdin_saved = fcntl(STDIN_FILENO, F_DUPFD_CLOEXEC, 0);
	if (stdin_saved < 0) {
		perror("Unable to dup stdin");
		return;
	}

	for (i = 0, j = 0; argv[i] != NULL; i++) {
		if (!strcmp(argv[i], "|")) {
			argv[i] = NULL;
			pipe_exec(&argv[j]);
			j = i+1;
		}
	}

	if ((f = fork()) < 0) {
		perror("Couldn't fork");
		return;
	}

	if (f == 0) {
		if (do_redirect(&argv[j]) < 0) {
			exit(EXIT_FAILURE);
		}

		if (execvp(argv[j], &argv[j]) < 0) {
			// TODO Enhance error reporting
			perror("execvp() failed");
			exit(0);
		}
	} else {
		dup2(stdin_saved, STDIN_FILENO);
		close(stdin_saved);
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

	if (argv == NULL || argv[0] == NULL) {
		fprintf(stderr, "Warning: got null argv for command %s\n", cmd);
		return;
	}

	if (!strcmp(argv[0], "cd")) {
		if (argv[1] != NULL) {
			if (chdir(argv[1]) < 0) {
				fprintf(stderr, "cd: %s: ", argv[1]);
				perror(NULL);
			}
		}
	} else {
		execute_cmd(argv);
	}

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
