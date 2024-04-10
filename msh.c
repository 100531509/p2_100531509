//P2-SSOO-23/24

//  MSH main file
// Write your msh source code here
void execute_simple_command(char ***argvv, int in_background) {
    pid_t pid = fork();
    if (pid == 0) { // Child
        execvp(argvv[0][0], argvv[0]);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // Parent
        if (!in_background) {
            wait(NULL); // Wait for the child to finish if not in background
        } else {
            printf("[%d]\n", pid); // Print PID for background process
        }
    } else {
        perror("fork failed");
    }
}

void setup_redirection(char *filev[3]) {
    if (strcmp(filev[0], "0") != 0) { // Input redirection
        int fd_in = open(filev[0], O_RDONLY);
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }
    if (strcmp(filev[1], "0") != 0) { // Output redirection
        int fd_out = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }
    if (strcmp(filev[2], "0") != 0) { // Error redirection
        int fd_err = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        dup2(fd_err, STDERR_FILENO);
        close(fd_err);
    }
}

void execute_command_sequence(char ***argvv, int num_commands, char *filev[3], int in_background) {
    int pipefds[2 * (num_commands - 1)];
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            perror("Couldn't Pipe");
            exit(EXIT_FAILURE);
        }
    }

    int pid;
    for (int i = 0; i < num_commands; i++) {
        pid = fork();
        if (pid == 0) {
            // If not the first command, get input from the previous pipe
            if (i != 0) {
                dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
            }
            // If not the last command, output to the next pipe
            if (i != num_commands - 1) {
                dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
            }
            // Close all pipe fds
            for (int j = 0; j < 2 * (num_commands - 1); j++) {
                close(pipefds[j]);
            }
            execvp(argvv[i][0], argvv[i]);
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
    }
    // Parent closes all of its copies at the end
    for (int i = 0; i < 2 * (num_commands - 1); i++) {
        close(pipefds[i]);
    }
    // Wait for children to exit if not in background
    if (!in_background) {
        for (int i = 0; i < num_commands; i++) {
            wait(NULL);
        }
    }
}

void execute_mycalc(char **args) {
    if (!args[1] || !args[2] || !args[3]) {
        printf("[ERROR] Usage: mycalc <operand1> <add|mul|div> <operand2>\n");
        return;
    }

    int op1 = atoi(args[1]);
    int op2 = atoi(args[3]);
    int result;

    if (strcmp(args[2], "add") == 0) {
        result = op1 + op2;
        printf("[OK] %d + %d = %d\n", op1, op2, result);
        // Update Acc environment variable if needed
    } else if (strcmp(args[2], "mul") == 0) {
        result = op1 * op2;
        printf("[OK] %d * %d = %d\n", op1, op2, result);
    } else if (strcmp(args[2], "div") == 0) {
        if (op2 == 0) {
            printf("[ERROR] Division by zero\n");
            return;
        }
        int quotient = op1 / op2;
        int remainder = op1 % op2;
        printf("[OK] %d / %d = %d; Remainder %d\n", op1, op2, quotient, remainder);
    } else {
        printf("[ERROR] Invalid operation\n");
    }
}

#define HISTORY_SIZE 20
struct command history[HISTORY_SIZE];
int history_count = 0;

void execute_myhistory(char **args) {
    if (args[1]) {
        int cmd_number = atoi(args[1]);
        // Validate cmd_number and execute the command from history
        printf("Running command %d: %s\n", cmd_number, history[cmd_number].cmd);
    } else {
        // List last 20 commands
        for (int i = 0; i < history_count; i++) {
            printf("%d %s\n", i, history[i].cmd);
        }
    }
}

void add_to_history(char ***argvv, char **filev, int in_background) {
    // Add the command to the history array
}


//#include "parser.h"
#include <stddef.h>			/* NULL */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_COMMANDS 8


// files in case of redirection
char filev[3][64];

//to store the execvp second parameter
char *argv_execvp[8];

void siginthandler(int param)
{
	printf("****  Exiting MSH **** \n");
	//signal(SIGINT, siginthandler);
	exit(0);
}

/* myhistory */

/* myhistory */

struct command
{
  // Store the number of commands in argvv
  int num_commands;
  // Store the number of arguments of each command
  int *args;
  // Store the commands
  char ***argvv;
  // Store the I/O redirection
  char filev[3][64];
  // Store if the command is executed in background or foreground
  int in_background;
};

int history_size = 20;
struct command * history;
int head = 0;
int tail = 0;
int n_elem = 0;

void free_command(struct command *cmd)
{
    if((*cmd).argvv != NULL)
    {
        char **argv;
        for (; (*cmd).argvv && *(*cmd).argvv; (*cmd).argvv++)
        {
            for (argv = *(*cmd).argvv; argv && *argv; argv++)
            {
                if(*argv){
                    free(*argv);
                    *argv = NULL;
                }
            }
        }
    }
    free((*cmd).args);
}

void store_command(char ***argvv, char filev[3][64], int in_background, struct command* cmd)
{
    int num_commands = 0;
    while(argvv[num_commands] != NULL){
        num_commands++;
    }

    for(int f=0;f < 3; f++)
    {
        if(strcmp(filev[f], "0") != 0)
        {
            strcpy((*cmd).filev[f], filev[f]);
        }
        else{
            strcpy((*cmd).filev[f], "0");
        }
    }

    (*cmd).in_background = in_background;
    (*cmd).num_commands = num_commands-1;
    (*cmd).argvv = (char ***) calloc((num_commands) ,sizeof(char **));
    (*cmd).args = (int*) calloc(num_commands , sizeof(int));

    for( int i = 0; i < num_commands; i++)
    {
        int args= 0;
        while( argvv[i][args] != NULL ){
            args++;
        }
        (*cmd).args[i] = args;
        (*cmd).argvv[i] = (char **) calloc((args+1) ,sizeof(char *));
        int j;
        for (j=0; j<args; j++)
        {
            (*cmd).argvv[i][j] = (char *)calloc(strlen(argvv[i][j]),sizeof(char));
            strcpy((*cmd).argvv[i][j], argvv[i][j] );
        }
    }
}


/**
 * Get the command with its parameters for execvp
 * Execute this instruction before run an execvp to obtain the complete command
 * @param argvv
 * @param num_command
 * @return
 */
void getCompleteCommand(char*** argvv, int num_command) {
	//reset first
	for(int j = 0; j < 8; j++)
		argv_execvp[j] = NULL;

	int i = 0;
	for ( i = 0; argvv[num_command][i] != NULL; i++)
		argv_execvp[i] = argvv[num_command][i];
}


/**
 * Main sheell  Loop  
 */
int main(int argc, char* argv[])
{
	/**** Do not delete this code.****/
	int end = 0; 
	int executed_cmd_lines = -1;
	char *cmd_line = NULL;
	char *cmd_lines[10];

	if (!isatty(STDIN_FILENO)) {
		cmd_line = (char*)malloc(100);
		while (scanf(" %[^\n]", cmd_line) != EOF){
			if(strlen(cmd_line) <= 0) return 0;
			cmd_lines[end] = (char*)malloc(strlen(cmd_line)+1);
			strcpy(cmd_lines[end], cmd_line);
			end++;
			fflush (stdin);
			fflush(stdout);
		}
	}

	/*********************************/

	char ***argvv = NULL;
	int num_commands;

	history = (struct command*) malloc(history_size *sizeof(struct command));
	int run_history = 0;

	while (1) 
	{
		int status = 0;
		int command_counter = 0;
		int in_background = 0;
		signal(SIGINT, siginthandler);

		if (run_history)
    {
        run_history=0;
    }
    else{
        // Prompt 
        write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

        // Get command
        //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
        executed_cmd_lines++;
        if( end != 0 && executed_cmd_lines < end) {
            command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
        }
        else if( end != 0 && executed_cmd_lines == end)
            return 0;
        else
            command_counter = read_command(&argvv, filev, &in_background); //NORMAL MODE
    }
		//************************************************************************************************


		/************************ STUDENTS CODE ********************************/
	   if (command_counter > 0) {
			if (command_counter > MAX_COMMANDS){
				printf("Error: Maximum number of commands is %d \n", MAX_COMMANDS);
			}
			else {
				// Print command
				print_command(argvv, filev, in_background);
			}
		}
	}
	
	return 0;
}
