// Shell starter file
// You may make any changes to any part of this file.

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <pwd.h>

#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)
#define HISTORY_DEPTH 10

char history[HISTORY_DEPTH][COMMAND_LENGTH];
int history_count = 0;
int history_length = 0;
char prevDir[COMMAND_LENGTH];

/**
 * Command Input and Processing
 */

/*
 * Tokenize the string in 'buff' into 'tokens'.
 * buff: Character array containing string to tokenize.
 *       Will be modified: all whitespace replaced with '\0'
 * tokens: array of pointers of size at least COMMAND_LENGTH/2 + 1.
 *       Will be modified so tokens[i] points to the i'th token
 *       in the string buff. All returned tokens will be non-empty.
 *       NOTE: pointers in tokens[] will all point into buff!
 *       Ends with a null pointer.
 * returns: number of tokens.
 */
int tokenize_command(char *buff, char *tokens[]) {
	int token_count = 0;
	_Bool in_token = false;
	int num_chars = strnlen(buff, COMMAND_LENGTH);
	for (int i = 0; i < num_chars; i++) {
		switch (buff[i]) {
		// Handle token delimiters (ends):
		case ' ':
		case '\t':
		case '\n':
			buff[i] = '\0';
			in_token = false;
			break;

		// Handle other characters (may be start)
		default:
			if (!in_token) {
				tokens[token_count] = &buff[i];
				token_count++;
				in_token = true;
			}
		}
	}
	tokens[token_count] = NULL;
	return token_count;
}

/**
 * Read a command from the keyboard into the buffer 'buff' and tokenize it
 * such that 'tokens[i]' points into 'buff' to the i'th token in the command.
 * buff: Buffer allocated by the calling code. Must be at least
 *       COMMAND_LENGTH bytes long.
 * tokens[]: Array of character pointers which point into 'buff'. Must be at
 *       least NUM_TOKENS long. Will strip out up to one final '&' token.
 *       tokens will be NULL terminated (a NULL pointer indicates end of tokens).
 * in_background: pointer to a boolean variable. Set to true if user entered
 *       an & as their last token; otherwise set to false.
 */
void read_command(char *buff, char *tokens[], _Bool *in_background, _Bool in_history) {
	*in_background = false;

	// Read input
	if (!in_history) {
		int length = read(STDIN_FILENO, buff, COMMAND_LENGTH-1);

		if (length < 0) {
			perror("Unable to read command from keyboard. Terminating.\n");
			exit(-1);
		}

		// Null terminate and strip \n.
		buff[length] = '\0';
		if (buff[strlen(buff) - 1] == '\n') {
			buff[strlen(buff) - 1] = '\0';
		}
	}

	// Tokenize (saving original command string)
	int token_count = tokenize_command(buff, tokens);
	if (token_count == 0) {
		return;
	}

	// Extract if running in background:
	if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
		*in_background = true;
		tokens[token_count - 1] = 0;
	}
}

void addHistory(char* tokens[], _Bool in_background) {
	if (history_length == HISTORY_DEPTH) {
		for (int i = 0; i < HISTORY_DEPTH - 1; i++) {
			strcpy(history[i], history[i + 1]);
		}
		strcpy(history[history_length-1], "");
		for (int i = 0; tokens[i] != NULL; i++) {
			strcat(history[history_length-1], tokens[i]);
			if (tokens[i+1] != NULL) {
				strcat(history[history_length-1], " ");
			}
		}
		if (in_background) {
			strcat(history[history_length-1], " &");
		}
	} else {
		for (int i = 0; tokens[i] != NULL; i++) {
			strcat(history[history_length], tokens[i]);
			if (tokens[i+1] != NULL) {
				strcat(history[history_length], " ");
			}
		}	
		if (in_background) {
			strcat(history[history_length], " &");
		}
	}

	if (history_length < HISTORY_DEPTH) {
		history_length++;
	}
	history_count++;
}

void checkExternal(char* tokens[], _Bool in_background) {
	pid_t var_pid = fork();

	if(var_pid < 0) { // error
		write(STDERR_FILENO, "fork failed\n", strlen("fork failed\n"));
		exit(EXIT_FAILURE);
	} else if (var_pid==0) { // Child process;
		if (execvp(tokens[0], tokens) < 0) {
			write(STDERR_FILENO, "execvp error\n", strlen("execvp error\n"));
			exit(EXIT_FAILURE);
		}
	} else { // Parent process 
		// Check if need to wait
		if (!in_background) {
			waitpid(var_pid, NULL, 0);
		} else {
			// Clean up zombie
			while (waitpid(-1, NULL, WNOHANG) > 0);
		}
	}
}

// Create function for forking and internal commands for organization
int checkInternal(char* tokens[]) {
	// Shell prompt and internal commands
	if (strcmp(tokens[0], "exit") == 0) {
		if (tokens[1] == NULL) {
			exit(EXIT_SUCCESS);
		} else { // Provided extra args
			write(STDOUT_FILENO, "Too many arguments\n", strlen("Too many arguments\n"));
		}
		// 1 means there was an internal command end current iter of loop
		return 1;
	} else if (strcmp(tokens[0], "pwd") == 0) {
		if (tokens[1] == NULL) { // Check if args
			char buf[COMMAND_LENGTH];
			if (getcwd(buf, sizeof(buf)) != NULL) {
				write(STDOUT_FILENO, buf, strlen(buf));
				write(STDOUT_FILENO, "\n", strlen("\n"));
			} else {
				write(STDOUT_FILENO, "getcwd Failed\n", strlen("getcwd Failed\n"));
			}
		} else { // Provided extra args
			write(STDOUT_FILENO, "Too many arguments\n", strlen("Too many arguments\n"));
		}
		return 1;
	} else if (strcmp(tokens[0], "cd") == 0) {
		char* path = tokens[1];
		// Check if path is empty or ~ and change to home
		if (path == NULL || strcmp(path, "~") == 0) {
			char* homedir = getenv("HOME");
			if (homedir == NULL) {
				struct passwd* pwd = getpwuid(getuid());
				if (pwd != NULL) {
					homedir = pwd->pw_dir;
				}
			} if (homedir != NULL) {
				// Try to get prevDir before calling chdir
				if (getcwd(prevDir, sizeof(prevDir)) == NULL) {
					write(STDOUT_FILENO, "getcwd failed for prevDir\n", strlen("getcwd failed for prevDir\n"));
				} 
				if (chdir(homedir) != 0) {
					write(STDOUT_FILENO, "chdir to home directory failed\n", strlen("chdir to home directory failed\n"));
				}
			}
		} else if (strcmp(path,"-") == 0) {
			// Also need to get prev dir incase cd - is called again
			char newPrev[COMMAND_LENGTH];
			strcpy(newPrev,prevDir);
			if (getcwd(prevDir,sizeof(prevDir)) == NULL) {
				write(STDOUT_FILENO, "getcwd failed for prevDir\n", strlen("getcwd failed for prevDir\n"));
			} 
			if (chdir(newPrev) != 0) {
				write(STDOUT_FILENO, "chdir to prev directory failed\n", strlen("chdir to prev directory failed\n"));
			}
		}
		// If we get here it means we have a path to cd to
		else if (tokens[2] == NULL) {
			// Try to get prevDir before calling chdir
			if (getcwd(prevDir,sizeof(prevDir)) == NULL) {
				write(STDOUT_FILENO, "getcwd failed for prevDir\n", strlen("getcwd failed for prevDir\n"));
			}
			if (path[0] == '~') {
				char* homedir = getenv("HOME");
				if (homedir == NULL) {
					struct passwd* pwd = getpwuid(getuid());
					if (pwd != NULL) {
						homedir = pwd->pw_dir;
					}
				}
				if (homedir != NULL) {
					// Create new path with home dir
					char new_path[COMMAND_LENGTH];
					snprintf(new_path, sizeof(new_path), "%s%s", homedir, path+1);
					if (chdir(new_path) != 0) {
						write(STDOUT_FILENO, "Error changing directory with ~\n", strlen("Error changing directory with ~\n"));
					}
				} else { // Home dir not found
					write(STDOUT_FILENO, "Unable to find home directory\n", strlen("Unable to find home directory\n"));
				}
			} else if (chdir(tokens[1]) != 0) { // Checking if path is legal and ~ wasn't used
				write(STDOUT_FILENO, "Error changing directory\n", strlen("Error changing directory\n"));
			}
		} else { //provided extra args
			write(STDOUT_FILENO, "Too many arguments\n", strlen("Too many arguments\n"));
		}
		return 1;
	} else if (strcmp(tokens[0], "help") == 0) {
		if (tokens[1] == NULL) {
			write(STDOUT_FILENO, "exit: Exits the shell\n", strlen("exit: Exits the shell\n"));
			write(STDOUT_FILENO, "pwd: Prints the current working directory\n", strlen("pwd: Prints the current working directory\n"));
			write(STDOUT_FILENO, "cd: Changes the current working directory\n", strlen("cd: Changes the current working directory\n"));
			write(STDOUT_FILENO, "help: Prints a list of internal commands\n", strlen("help: Prints a list of internal commands\n"));
		} else if (tokens[2] != NULL) {
			write(STDOUT_FILENO, "Too many arguments\n", strlen("Too many arguments\n"));
		} else if (strcmp(tokens[1],"exit") == 0) {
			write(STDOUT_FILENO, "exit: Exits the shell\n", strlen("exit: Exits the shell\n"));
		} else if (strcmp(tokens[1],"pwd") == 0) {
			write(STDOUT_FILENO, "pwd: Prints the current working directory\n", strlen("pwd: Prints the current working directory\n"));
		} else if (strcmp(tokens[1],"cd") == 0) {
			write(STDOUT_FILENO, "cd: Changes the current working directory\n", strlen("cd: Changes the current working directory\n"));
		} else if (strcmp(tokens[1],"help") == 0) {
			write(STDOUT_FILENO, "help: Prints a list of internal commands\n", strlen("help: Prints a list of internal commands\n"));
		} else {
			char temp[256];
			snprintf(temp, sizeof(temp), "'%s' is an external command or application\n", tokens[1]);
			write(STDOUT_FILENO, temp, strlen(temp));
		}
		return 1;
	} else if (strcmp(tokens[0], "history") == 0) {
		if (tokens[1] != NULL) {
			write(STDOUT_FILENO, "Too many arguments\n", strlen("Too many arguments\n"));
		} else {
			for (int i = history_length - 1; i >= 0; i--) {
				char temp[2056];
				snprintf(temp, sizeof(temp), "%d: %s\n", history_count - history_length + i, history[i]);
				write(STDOUT_FILENO, temp, strlen(temp));
			}
		}
		return 1;
	} else if (tokens[0][0] == '!') {
		if (tokens[1] != NULL) {
			write(STDOUT_FILENO, "Too many arguments\n", strlen("Too many arguments\n"));
		} else if (tokens[0][1] == '!') {
			if (history_length == 0) {
				write(STDOUT_FILENO, "No commands in history\n", strlen("No commands in history\n"));
			} else {
				char *tokens[NUM_TOKENS];
				_Bool in_background = false;

				write(STDOUT_FILENO, history[history_length - 1], strlen(history[history_length - 1]));
				write(STDOUT_FILENO, "\n", strlen("\n"));

				read_command(history[history_length - 1], tokens, &in_background, true);

				addHistory(tokens, in_background);

				if (checkInternal(tokens) == -1) {
					checkExternal(tokens, in_background);
				}
			}
		} else if (tokens[0][1] == '-') {
			if (tokens[1] != NULL) {
				write(STDOUT_FILENO, "Too many arguments\n", strlen("Too many arguments\n"));
			} else {
				history_count = 0;
				for (int i = 0; i < history_length; i++) {
					strcpy(history[i], "");
				}
				history_length = 0;
			}
		} else {
			int num; 

			if (tokens[0][1] == '0') {
				num = 0;
			} else {
				num = atoi(tokens[0] + 1);
				if (num == 0) {
					write(STDOUT_FILENO, "Invalid history number\n", strlen("Invalid history number\n"));
					return 1;
				}
			}

			if (num > history_count || num < history_count - history_length) {
				write(STDOUT_FILENO, "Invalid history number\n", strlen("Invalid history number\n"));
			} else {
				char *tokens[NUM_TOKENS];
				_Bool in_background = false;

				write(STDOUT_FILENO, history[history_length - (history_count - num)], strlen(history[history_length - (history_count - num)]));
				write(STDOUT_FILENO, "\n", strlen("\n"));

				read_command(history[history_length - (history_count - num)], tokens, &in_background, true);

				addHistory(tokens, in_background);

				if (checkInternal(tokens) == -1) {
					checkExternal(tokens, in_background);
				}
			}
		}
		return 1;
	
	}
	// Got to end and no internal commands were sent, return -1 to tell shell to fork
	return -1;
}

int main(int argc, char* argv[]) {
	char input_buffer[COMMAND_LENGTH];
	char *tokens[NUM_TOKENS];

	if (chdir(getenv("HOME")) == -1) {
		write(STDERR_FILENO, "chdir() error\n", strlen("chdir() error\n"));
		exit(EXIT_FAILURE);
	}

	while (true) {
		// Get command
		// Use write because we need to use read() to work with
		// signals, and read() is incompatible with printf().
		char cwd[256];
		if (getcwd(cwd, sizeof(cwd)) != NULL) {
			char temp[258];
			snprintf(temp, sizeof(temp), "%s$ ", cwd);
			write(STDOUT_FILENO, temp, strlen(temp));
        } else {
			write(STDERR_FILENO, "getcwd() error", strlen("getcwd() error"));
            exit(EXIT_FAILURE);
        }

		_Bool in_background = false;
		read_command(input_buffer, tokens, &in_background, false);

		// Add to history
		if (tokens[0][0] != '!') {
			addHistory(tokens, in_background);
		}

		// DEBUG: Dump out arguments:
		/*
		for (int i = 0; tokens[i] != NULL; i++) {
			write(STDOUT_FILENO, "   Token: ", strlen("   Token: "));
			write(STDOUT_FILENO, tokens[i], strlen(tokens[i]));
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}
		if (in_background) {
			write(STDOUT_FILENO, "Run in background.\n", strlen("Run in background.\n"));
		}
		*/

		/**
		 * Steps For Basic Shell:
		 * 1. Fork a child process
		 * 2. Child process invokes execvp() using results in token array.
		 * 3. If in_background is false, parent waits for
		 *    child to finish. Otherwise, parent loops back to
		 *    read_command() again immediately.
		 */

		if (checkInternal(tokens) == -1) {
			checkExternal(tokens, in_background);
		}
	}
	return 0;
}