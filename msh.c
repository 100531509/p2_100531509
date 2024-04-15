// P2-SSOO-23/24

// #include "parser.h"
#include <stddef.h> /* NULL */
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
#include <stdbool.h>

#define MAX_COMMANDS 8

//  MSH main file
// Write your msh source code here
void execute_command(char *argv[], char *input_file, char *output_file, char *error_file, int background)
{
    int pid, status;
    int in_fd, out_fd, err_fd;

    pid = fork();
    if (pid == 0)
    {
        // Child process

        // Input redirection
        if (input_file)
        {
            in_fd = open(input_file, O_RDONLY);
            if (in_fd == -1)
            {
                perror("Failed to open input file");
                exit(EXIT_FAILURE);
            }
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }

        // Output redirection
        if (output_file)
        {
            out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out_fd == -1)
            {
                perror("Failed to open output file");
                exit(EXIT_FAILURE);
            }
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }

        // Error redirection
        if (error_file)
        {
            err_fd = open(error_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (err_fd == -1)
            {
                perror("Failed to open error file");
                exit(EXIT_FAILURE);
            }
            dup2(err_fd, STDERR_FILENO);
            close(err_fd);
        }

        // Execute the command
        execvp(argv[0], argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // Parent process
    if (!background)
    {
        waitpid(pid, &status, 0); // Wait only if not a background process
    }
    else
    {
        printf("Started background process PID: %d\n", pid);
    }
}

void execute_command_sequence(char *cmd1[], char *cmd2[], char *cmd3[])
{
    int pipe1[2], pipe2[2]; // Two pipes

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // First child process
    if (fork() == 0)
    {
        // Redirect stdout to pipe1 write end
        dup2(pipe1[1], STDOUT_FILENO);
        close(pipe1[0]); // Close unused read end
        close(pipe1[1]); // Close write end after dup

        execvp(cmd1[0], cmd1);
        perror("execvp cmd1");
        exit(EXIT_FAILURE);
    }

    // Second child process
    if (fork() == 0)
    {
        // Redirect stdin to pipe1 read end
        dup2(pipe1[0], STDIN_FILENO);
        // Redirect stdout to pipe2 write end
        dup2(pipe2[1], STDOUT_FILENO);
        close(pipe1[1]); // Close unused write end
        close(pipe1[0]); // Close read end after dup
        close(pipe2[0]); // Close unused read end
        close(pipe2[1]); // Close write end after dup

        execvp(cmd2[0], cmd2);
        perror("execvp cmd2");
        exit(EXIT_FAILURE);
    }

    // Third child process
    if (fork() == 0)
    {
        // Redirect stdin to pipe2 read end
        dup2(pipe2[0], STDIN_FILENO);
        close(pipe2[1]); // Close unused write end
        close(pipe2[0]); // Close read end after dup

        execvp(cmd3[0], cmd3);
        perror("execvp cmd3");
        exit(EXIT_FAILURE);
    }

    // Parent closes all pipes and waits for children
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    // Wait for all child processes to finish
    for (int i = 0; i < 3; i++)
    {
        wait(NULL);
    }
}

// Initialize the accumulator
int acc = 0;

void mycalc(char *args[], int num_args)
{
    if (num_args != 4)
    {
        fprintf(stderr, "[ERROR] The structure of the command is mycalc <operand 1> <add/mul/div> <operand 2>\n");
        return;
    }

    int operand1 = atoi(args[1]);
    char *operator= args[2];
    int operand2 = atoi(args[3]);
    int result, remainder;

    if (strcmp(operator, "add") == 0)
    {
        result = operand1 + operand2;
        acc += result; // Updating the accumulator with the result of addition
        fprintf(stderr, "[OK] %d + %d = %d; Acc %d\n", operand1, operand2, result, acc);
    }
    else if (strcmp(operator, "mul") == 0)
    {
        result = operand1 * operand2;
        fprintf(stderr, "[OK] %d * %d = %d\n", operand1, operand2, result);
    }
    else if (strcmp(operator, "div") == 0)
    {
        if (operand2 == 0)
        {
            fprintf(stderr, "[ERROR] Division by zero is not allowed.\n");
            return;
        }
        result = operand1 / operand2;
        remainder = operand1 % operand2;
        fprintf(stderr, "[OK] %d / %d = %d; Remainder %d\n", operand1, operand2, result, remainder);
    }
    else
    {
        fprintf(stderr, "[ERROR] The structure of the command is mycalc <operand 1> <add/mul/div> <operand 2>\n");
    }
}

// #define HISTORY_SIZE 20
// struct command history[HISTORY_SIZE];
// int history_count = 0;

// void execute_myhistory(char **args)
// {
//     if (args[1])
//     {
//         int cmd_number = atoi(args[1]);
//         // Validate cmd_number and execute the command from history
//         printf("Running command %d: %s\n", cmd_number, history[cmd_number].cmd);
//     }
//     else
//     {
//         // List last 20 commands
//         for (int i = 0; i < history_count; i++)
//         {
//             printf("%d %s\n", i, history[i].cmd);
//         }
//     }
// }

// void add_to_history(char ***argvv, char **filev, int in_background)
// {
//     // Add the command to the history array
// }

// files in case of redirection
char filev[3][64];

// to store the execvp second parameter
char *argv_execvp[8];

void siginthandler(int param)
{
    printf("****  Exiting MSH **** \n");
    // signal(SIGINT, siginthandler);
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
struct command *history;
int head = 0;
int tail = 0;
int n_elem = 0;

void free_command(struct command *cmd)
{
    if ((*cmd).argvv != NULL)
    {
        char **argv;
        for (; (*cmd).argvv && *(*cmd).argvv; (*cmd).argvv++)
        {
            for (argv = *(*cmd).argvv; argv && *argv; argv++)
            {
                if (*argv)
                {
                    free(*argv);
                    *argv = NULL;
                }
            }
        }
    }
    free((*cmd).args);
}

void store_command(char ***argvv, char filev[3][64], int in_background, struct command *cmd)
{
    int num_commands = 0;
    while (argvv[num_commands] != NULL)
    {
        num_commands++;
    }

    for (int f = 0; f < 3; f++)
    {
        if (strcmp(filev[f], "0") != 0)
        {
            strcpy((*cmd).filev[f], filev[f]);
        }
        else
        {
            strcpy((*cmd).filev[f], "0");
        }
    }

    (*cmd).in_background = in_background;
    (*cmd).num_commands = num_commands - 1;
    (*cmd).argvv = (char ***)calloc((num_commands), sizeof(char **));
    (*cmd).args = (int *)calloc(num_commands, sizeof(int));

    for (int i = 0; i < num_commands; i++)
    {
        int args = 0;
        while (argvv[i][args] != NULL)
        {
            args++;
        }
        (*cmd).args[i] = args;
        (*cmd).argvv[i] = (char **)calloc((args + 1), sizeof(char *));
        int j;
        for (j = 0; j < args; j++)
        {
            (*cmd).argvv[i][j] = (char *)calloc(strlen(argvv[i][j]), sizeof(char));
            strcpy((*cmd).argvv[i][j], argvv[i][j]);
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
void getCompleteCommand(char ***argvv, int num_command)
{
    // reset first
    for (int j = 0; j < 8; j++)
        argv_execvp[j] = NULL;

    int i = 0;
    for (i = 0; argvv[num_command][i] != NULL; i++)
        argv_execvp[i] = argvv[num_command][i];
}

/**
 * Main sheell  Loop
 */
int main(int argc, char *argv[])
{
    /**** Do not delete this code.****/
    int end = 0;
    int executed_cmd_lines = -1;
    char *cmd_line = NULL;
    char *cmd_lines[10];

    if (!isatty(STDIN_FILENO))
    {
        cmd_line = (char *)malloc(100);
        while (scanf(" %[^\n]", cmd_line) != EOF)
        {
            if (strlen(cmd_line) <= 0)
                return 0;
            cmd_lines[end] = (char *)malloc(strlen(cmd_line) + 1);
            strcpy(cmd_lines[end], cmd_line);
            end++;
            fflush(stdin);
            fflush(stdout);
        }
    }

    /*********************************/

    char ***argvv = NULL;
    int num_commands;

    history = (struct command *)malloc(history_size * sizeof(struct command));
    int run_history = 0;

    while (1)
    {
        int status = 0;
        int command_counter = 0;
        int in_background = 0;
        signal(SIGINT, siginthandler);

        if (run_history)
        {
            run_history = 0;
        }
        else
        {
            // Prompt
            write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

            // Get command
            //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
            executed_cmd_lines++;
            if (end != 0 && executed_cmd_lines < end)
            {
                command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
            }
            else if (end != 0 && executed_cmd_lines == end)
                return 0;
            else
                command_counter = read_command(&argvv, filev, &in_background); // NORMAL MODE
        }
        //************************************************************************************************

        /************************ STUDENTS CODE ********************************/
        if (command_counter > 0)
        {
            if (command_counter > MAX_COMMANDS)
            {
                printf("Error: Maximum number of commands is %d \n", MAX_COMMANDS);
            }
            else
            {
                // Print command
                print_command(argvv, filev, in_background);
            }
        }
    }

    return 0;
}
