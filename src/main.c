#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

void process_cmd(const char *cmd_line, size_t line_len) {
	printf("Got line of size %zu: %s\n", line_len, cmd_line);
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
			process_cmd(line, line_len);
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
