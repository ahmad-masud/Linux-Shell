// Shell starter file
// You may make any changes to any part of this file.

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)


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
int tokenize_command(char *buff, char *tokens[])
{
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
void read_command(char *buff, char *tokens[], _Bool *in_background)
{
	*in_background = false;

	// Read input
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

/**
 * Main and Execute Commands
 */

//Create function for forking and internal commands for organization
int checkInternal(char* tokens[]){
	//Shell prompt and internal commands
	if (strcmp(tokens[0],"exit")==0){
		if (tokens[1]==NULL){
			exit(EXIT_SUCCESS);
		}
		else { //provided extra args
			write(STDOUT_FILENO, "Too many arguments",strlen("Too many arguments"));
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}
		//1 means there was an internal command end current iter of loop
		return 1;
	}
	else if (strcmp(tokens[0],"pwd")==0){
		if (tokens[1]==NULL) {// check if args
			char buf[256];
			if (getcwd(buf,sizeof(buf)) != NULL ){
				write(STDOUT_FILENO,buf,strlen(buf));
				write(STDOUT_FILENO, "\n", strlen("\n"));
			}
			else {
				write(STDOUT_FILENO,"getcwd Failed", strlen("getcwd Failed"));
				write(STDOUT_FILENO, "\n", strlen("\n"));
			}
		}
		else { //provided extra args
			write(STDOUT_FILENO, "Too many arguments",strlen("Too many arguments"));
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}
		return 1;
	}
	else if (strcmp(tokens[0],"cd")==0){
		if (tokens[2]==NULL){
			if (chdir(tokens[1])!=-1){ // checking for error in chdir
				//do nothing
			}
			else {
				write(STDOUT_FILENO,"Error changing directory", strlen("Error changing directory"));
				write(STDOUT_FILENO, "\n", strlen("\n"));
			}
		}
		else { //provided extra args
			write(STDOUT_FILENO, "Too many arguments",strlen("Too many arguments"));
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}
		return 1;
	}

	//got to end and no internal commands were sent, return -1 to tell shell to fork
	return -1;
}

int main(int argc, char* argv[])
{
	char input_buffer[COMMAND_LENGTH];
	char *tokens[NUM_TOKENS];
	while (true) {

		// Get command
		// Use write because we need to use read() to work with
		// signals, and read() is incompatible with printf().
		write(STDOUT_FILENO, "$ ", strlen("$ "));
		_Bool in_background = false;
		read_command(input_buffer, tokens, &in_background);

		// DEBUG: Dump out arguments:
		for (int i = 0; tokens[i] != NULL; i++) {
			write(STDOUT_FILENO, "   Token: ", strlen("   Token: "));
			write(STDOUT_FILENO, tokens[i], strlen(tokens[i]));
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}
		if (in_background) {
			write(STDOUT_FILENO, "Run in background.", strlen("Run in background."));
		}

		/**
		 * Steps For Basic Shell:
		 * 1. Fork a child process
		 * 2. Child process invokes execvp() using results in token array.
		 * 3. If in_background is false, parent waits for
		 *    child to finish. Otherwise, parent loops back to
		 *    read_command() again immediately.
		 */

		if (checkInternal(tokens)==1){
			continue;
		}
		else {
			int var_pid;
			int status;
			var_pid = fork();

			if(var_pid<0){
				fprintf(stderr,"fork Failed");
				exit(-1);
			}
			else if (var_pid==0) { //in child process;
				if (execvp(tokens[0],tokens) < 0){
					write(STDERR_FILENO,"Error type ", strlen("Error type"));
					exit(-1);
				}
			}
			else { //parent process 
				//check if need to wait
				if(!in_background){
					waitpid(var_pid,&status,0);
				}
			}
			//clean up zombie
			while (waitpid(-1,NULL,WNOHANG) > 0);
		}
	}
	return 0;
}