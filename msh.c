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

#define MAX_HISTORY 20

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

void execute_command(char *argv[], char *input_file, char *output_file, char *error_file, int background)
{
    int pid, status;
    int in_fd, out_fd, err_fd;

    pid = fork();
    if (pid == 0)
    {
        // Child process
        // Input redirection
        if (input_file && strcmp(input_file, "0") != 0)
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
        if (output_file && strcmp(output_file, "0") != 0)
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
        if (error_file && strcmp(error_file, "0") != 0)
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
        fprintf(stderr, "Started background process PID: %d\n", pid); // Using fprintf with stderr
    }
}

void execute_command_sequence(char ***commands, int num_commands, char *input_file, char *output_file, char *error_file, int background)
{
    int **pipes = malloc((num_commands - 1) * sizeof(int *));
    int i = 0;
    pid_t pid;
    int in_fd = 0, out_fd = 1;

    // Set up all needed pipes in advance
    for (i = 0; i < num_commands - 1; i++)
    {
        pipes[i] = malloc(2 * sizeof(int));
        pipe(pipes[i]);
    }

    for (i = 0; i < num_commands; i++)
    {
        pid = fork();
        if (pid == 0)
        {
            // Redirect input from the previous pipe if not the first command
            if (i > 0)
            {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            else if (input_file && strcmp(input_file, "0") != 0)
            {
                // Handle input redirection for the first command
                in_fd = open(input_file, O_RDONLY);
                if (in_fd == -1)
                {
                    perror("Failed to open input file");
                    exit(EXIT_FAILURE);
                }
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            // Redirect output to the next pipe if not the last command
            if (i < num_commands - 1)
            {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            else if (output_file && strcmp(output_file, "0") != 0)
            {
                // Handle output redirection for the last command
                out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (out_fd == -1)
                {
                    perror("Failed to open output file");
                    exit(EXIT_FAILURE);
                }
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }

            // Close all pipes in the child
            for (int j = 0; j < num_commands - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(commands[i][0], commands[i]);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }

    // Parent closes all pipe ends
    for (i = 0; i < num_commands - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
        free(pipes[i]);
    }
    free(pipes);

    if (!background)
    {
        while (wait(NULL) > 0)
            ; // Wait for all children if not in background
    }
    else
    {
        printf("Background execution started for a pipeline of %d commands.\n", num_commands);
    }
}

// Global accumulator
int acc = 0;

void mycalc(char *args[], int num_args)
{
    // Ensure the number of arguments is correct
    if (num_args != 4)
    {
        printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n");
        return;
    }

    // Arguments parsing
    int operand1 = atoi(args[1]); // Convert first argument to integer
    char *operation = args[2];    // Operation to perform: add, mul, div
    int operand2 = atoi(args[3]); // Convert second argument to integer

    // Validate input conversion - atoi returns 0 if conversion fails, which can be misleading if '0' is an operand
    if ((strcmp(args[1], "0") != 0 && operand1 == 0) || (strcmp(args[3], "0") != 0 && operand2 == 0))
    {
        printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n");
        return;
    }

    // Calculation variables
    int result = 0;
    int remainder = 0;

    // Determine operation based on operator string
    if (strcmp(operation, "add") == 0)
    {
        result = operand1 + operand2;
        acc += result; // Update accumulator only on 'add'
        fprintf(stderr, "[OK] %d + %d = %d; Acc %d\n", operand1, operand2, result, acc);
    }
    else if (strcmp(operation, "mul") == 0)
    {
        result = operand1 * operand2;
        fprintf(stderr, "[OK] %d * %d = %d\n", operand1, operand2, result);
    }
    else if (strcmp(operation, "div") == 0)
    {
        if (operand2 == 0)
        {
            printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n");
            return;
        }
        result = operand1 / operand2;
        remainder = operand1 % operand2;
        fprintf(stderr, "[OK] %d / %d = %d; Remainder %d\n", operand1, operand2, result, remainder);
    }
    else
    {
        printf("[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n");
    }
}

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

void add_history_item(char ***argvv, char filev[3][64], int in_background)
{
    if (history == NULL)
    {
        history = (struct command *)malloc(MAX_HISTORY * sizeof(struct command));
    }

    struct command *cmd = &history[tail];
    store_command(argvv, filev, in_background, cmd);

    tail = (tail + 1) % MAX_HISTORY;
    if (n_elem < MAX_HISTORY)
    {
        n_elem++;
    }
}

void display_history()
{
    int count = 0;
    int index = head;
    while (count < n_elem)
    {
        fprintf(stderr, "%d ", count); // Outputting to stderr and starting from 0
        for (int j = 0; j < history[index].num_commands; j++)
        {
            for (int k = 0; k < history[index].args[j]; k++)
            {
                fprintf(stderr, "%s ", history[index].argvv[j][k]); // Outputting to stderr
            }
            if (j < history[index].num_commands - 1)
            {
                fprintf(stderr, "| "); // Outputting to stderr
            }
        }
        fprintf(stderr, "\n"); // Outputting to stderr
        index = (index + 1) % MAX_HISTORY;
        count++;
    }
}

void execute_history_item(int index)
{
    if (index < 0 || index >= n_elem)
    {
        printf("ERROR: Command not found\n");
        return;
    }

    int i = (head + index) % MAX_HISTORY;
    fprintf(stderr, "Running command %d\n", index);

    // Execute the command based on its structure
    if (history[i].num_commands == 1)
    {
        // Handle single commands with no piping but potential redirection and background execution
        execute_command(history[i].argvv[0], history[i].filev[0], history[i].filev[1], history[i].filev[2], history[i].in_background);
    }
    else if (history[i].num_commands > 1 && history[i].num_commands <= 3)
    {
        // Assuming you are in a function where 'i' indexes into a history or similar structure
        if (history[i].num_commands >= 2)
        {
            // Array to hold command pointers
            char ***commands = malloc(history[i].num_commands * sizeof(char **));
            for (int j = 0; j < history[i].num_commands; j++)
            {
                commands[j] = history[i].argvv[j];
            }

            // Call the updated execute_command_sequence function
            execute_command_sequence(commands, history[i].num_commands, history[i].filev[0], history[i].filev[1], history[i].filev[2], history[i].in_background);

            // Free the temporary array after use
            free(commands);
        }
    }
    else
    {
        printf("ERROR: Command not found\n");
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
 * Main shell Loop
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
        // Check for internal commands
        if (strcmp(argvv[0][0], "myhistory") == 0)
        {
            int new_count = 0;
            while (argvv[0][new_count] != NULL)
                new_count++;
            if (new_count == 1)
            {
                display_history();
            }
            else if (new_count == 2)
            {
                int index = atoi(argvv[0][1]);
                execute_history_item(index);
            }
            else
            {
                fprintf(stderr, "Usage: myhistory [<command_index>]\n");
            }
            continue;
        }

        // Add command to history
        char *cmd_line = NULL;
        asprintf(&cmd_line, "%s", argvv[0][0]);
        add_history_item(argvv, filev, in_background);

        if (strcmp(argvv[0][0], "mycalc") == 0)
        {
            // Ensure command_counter correctly counts all elements in argvv[0]
            int arg_count = 0;
            while (argvv[0][arg_count] != NULL)
                arg_count++;
            mycalc(argvv[0], arg_count);
            continue;
        }

        if (command_counter > 0)
        {
            if (command_counter == 1)
            {
                // Handle single commands with no piping but potential redirection and background execution
                execute_command(argvv[0], filev[0], filev[1], filev[2], in_background);
            }
            else if (command_counter > 1 && command_counter <= 3)
            {
                if (command_counter >= 2)
                {
                    // Array to hold command pointers for dynamic command sequence execution
                    char ***commands = malloc(command_counter * sizeof(char **));
                    for (int j = 0; j < command_counter; j++)
                    {
                        commands[j] = argvv[j];
                    }

                    // Call the updated execute_command_sequence function
                    execute_command_sequence(commands, command_counter, filev[0], filev[1], filev[2], in_background);

                    // Free the temporary array after use
                    free(commands);
                }
            }
            else
            {
                fprintf(stderr, "Error: Unsupported number of commands.\n");
            }
        }
        else if (command_counter == 0)
        {
            fprintf(stderr, "No command entered.\n");
        }
        /***********************************************************************/
    }

    return 0;
}
