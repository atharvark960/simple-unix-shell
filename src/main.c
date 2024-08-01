#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MYSH_RL_BUFSIZE 1024
#define MYSH_TOK_BUFSIZE 64
#define MYSH_TOK_DELIM " \t\r\n\a"

void mysh_loop(void);
char* mysh_read_line(void);
char** mysh_split_line(char *);
int mysh_launch(char**);
int mysh_help(char**);
int mysh_cd(char**);
int mysh_exit(char**);
int mysh_execute(char**);

int main(int argc, char **argv) {
    mysh_loop();

    return 0;
}

void mysh_loop() {
    char *line;
    char **args;
    int status;

    do {
        printf("$ ");
        line = mysh_read_line();
        args = mysh_split_line(line);
        status = mysh_execute(args);

        free(line);
        free(args);
    } while (status);
}

char* mysh_read_line() {
    int bufsize = MYSH_RL_BUFSIZE;
    int position = 0;
    char* buffer = (char *) malloc(sizeof(char) * bufsize);
    int c;

    if (buffer == NULL) {
        fprintf(stderr, "mysh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        c = getchar();

        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position++] = c;
        }

        if (position >= bufsize) {
            bufsize += MYSH_RL_BUFSIZE;
            buffer = (char *) realloc(buffer, bufsize);

            if (buffer == NULL) {
                fprintf(stderr, "mysh: allocation error\n");
                exit(EXIT_FAILURE);
            }
		}
    }
}

char** mysh_split_line(char* line) {
	int bufsize = MYSH_TOK_BUFSIZE;
	int position = 0;
	char** tokens = malloc(bufsize * sizeof(char*));
	char* token;

    if (tokens == NULL) {
        fprintf(stderr, "mysh: allocation error\n");
        exit(EXIT_FAILURE);
    }

	token = strtok(line, MYSH_TOK_DELIM);

	while (token != NULL) {
		tokens[position++] = token;

		if (position >= bufsize) {
			bufsize += MYSH_TOK_BUFSIZE;
			tokens = realloc(tokens, bufsize * sizeof(char*));

			if (tokens == NULL) {
                fprintf(stderr, "mysh: allocation error\n");
                exit(EXIT_FAILURE);
            }
		}

		token = strtok(NULL, MYSH_TOK_DELIM);
	}

	tokens[position] = NULL;
	return tokens;
}

int mysh_launch(char** args) {
	pid_t pid;
	int status;

	pid = fork();

	if (pid == 0) {
		execvp(args[0], args);
		fprintf(stderr, "mysh: %s: command not found\n", args[0]);
		exit(EXIT_FAILURE);
	} else if (pid < 0) {
		perror("mysh: fork()");
	}

	do {
		waitpid(pid, &status, WUNTRACED);
	} while (!WIFEXITED(status) && !WIFSIGNALED(status));

	return 1;
}

char *builtin_str[] = {
	"help",
	"cd",
	"exit"
};

int (*builtin_func[]) (char**) = {
	&mysh_help,
	&mysh_cd,
	&mysh_exit
};

int mysh_num_builtins() {
	return sizeof(builtin_str) / sizeof(char*);
}

int mysh_help(char** args) {
	printf("mysh, A simple unix shell\n");
	printf("These shell commands are defined internally. Type `help` to see this list\n");
	printf("\n");

	for (int i = 0; i < mysh_num_builtins(); i++) {
		printf("%s\n", builtin_str[i]);
	}

	printf("\n");
	printf("Use `man`command for information on other commands and utilities\n");

	return 1;
}

int mysh_cd(char** args) {
	if (args[1] == NULL) {
		fprintf(stderr, "mysh: cd: too few arguments\n");
	} else {
		if (chdir(args[1]) != 0) {
			perror("mysh: cd");
		}
	}

	return 1;
}

int mysh_exit(char** args) {
	return 0;
}

int mysh_execute(char** args) {
	if (args[0] == NULL) {
		return 1;
	} else {
		for (int i = 0; i < mysh_num_builtins(); i++) {
			if (strcmp(args[0], builtin_str[i]) == 0) {
				return (*builtin_func[i])(args);
			}
		}
	}

	return mysh_launch(args);
}
