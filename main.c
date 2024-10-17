/****************************************************************************

  @file         main.c

  @author       Ahnaful Hoque

  @date         21/08/2024

*******************************************************************************/

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <signal.h>
#include <fcntl.h>

#define LINE_LENGTH 100
#define MAX_ARGS 5
#define MAX_LENGTH 20
#define MAX_BG_PROC 1
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

pid_t bg_processes[MAX_BG_PROC] = {-1}; // Array to keep track of background processes
int bg_count = 0;
long total_user_time = 0;
long total_sys_time = 0;

/**
 * @brief Reads a line of input from standard input (stdin).
 *
 * If getline() encounters EOF, the function exits successfully (EXIT_SUCCESS). 
 * On read error, it prints an error message with perror and exits with 
 * failure status (EXIT_FAILURE).
 *
 * @param void
 * @return A pointer to the dynamically allocated buffer containing the 
 *         input line, or NULL if an error occurred.
 */

char *read_line(void)
{
    char *line = NULL;
    size_t bufsize = 0;

    if (getline(&line, &bufsize, stdin) == -1)
    {
        if (feof(stdin))
        {
            exit(EXIT_SUCCESS);
        }
        else
        {
            perror("Error in getline()");
            exit(EXIT_FAILURE);
        }
    }
    return line;
}


/**
 * @brief Splits a line into tokens based on predefined delimiters.
 *
 * Dynamically allocates memory for the tokens and handles reallocation 
 * as needed. Uses strtok() to split the line and returns a NULL-terminated 
 * array of strings (tokens).
 *
 * @param line A pointer to the string to be tokenized.
 *
 * @return tokens: A pointer to an array of strings extracted from the 
 *         input line.
 *         NULL: if memory allocation fails.
 */

char **split_line(char *line)
{
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if (!tokens)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL)
    {
        tokens[position] = token;
        position++;

        if (position >= bufsize)
        {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

/**
 * @brief Changes the current working directory.
 *
 * Attempts to change the working directory to the specified path.
 * If the path is NULL, an error message is shown. If changing
 * the directory fails, it prints an error using perror().
 *
 * @param path The directory path to change to. If NULL, an 
 *             error is displayed.
 * @return none
 */

void cd(char *path)
{
    if (path == NULL)
    {
        printf("dragonshell: Expected argument to \"cd\"\n");
    }
    else if (chdir(path) != 0)
    {
        perror("dragonshell");
    }
}


/**
 * @brief Prints the current working directory.
 *
 * Retrieves and displays the current working directory. If
 * retrieving the directory fails, it prints an error using
 * perror().
 * 
 * @param none
 * @return none
 */

void pwd(void)
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("%s\n", cwd);
    }
    else
    {
        perror("dragonshell");
    }
}

/**
 * @brief Gracefully terminates the shell and its background processes.
 *
 * Retrieves and prints the total user and system execution time 
 * for all child processes spawned since the shell started. Sends 
 * a SIGTERM signal to any running background processes before 
 * terminating the shell.
 * 
 * @param none
 * @return none
 */

void exit_shell(void)
{
    struct rusage usage;
    // Get resource usage of child processes
    getrusage(RUSAGE_CHILDREN, &usage);

    total_user_time += usage.ru_utime.tv_sec;
    total_sys_time += usage.ru_stime.tv_sec;

    printf("User time: %ld seconds\n",total_user_time);
    printf("Sys time: %ld seconds\n", total_sys_time);
    
    // Terminate any background processes
    
    for (int i = 0; i < MAX_BG_PROC; i++)
    {
        if (bg_processes[i] != -1)
        {
            kill(bg_processes[i], SIGTERM);
        }
    }
    exit(0);
}

pid_t shell_pid, child_pid = -1;

/**
 * @brief Handles SIGINT signal (Ctrl+C).
 *
 * If a child process is running, sends SIGINT to the entire process group
 * of the child. If no child process is active, prints a new prompt
 * to the shell.
 *
 * @param sig The signal number received (typically SIGINT)
 * @return none
 * 
 */
void sigint_handler(int sig)
{
    if (child_pid != -1)
    {
        kill(-child_pid, SIGINT);
    }
    else
    {
        printf("\ndragonshell > ");
        fflush(stdout);
    }
}

/**
 * @brief Handles SIGTSTP signal (Ctrl+Z).
 *
 * If a child process is running, sends SIGTSTP to the entire process
 * group of the child. If no child process is active, prints a new
 * prompt to the shell.
 *
 * @param sig The signal number received (typically SIGTSTP).
 * @return 0
 */
void sigtstp_handler(int sig)
{
    if (child_pid != -1)
    {
        kill(-child_pid, SIGTSTP);
    }
    else
    {
        printf("\ndragonshell > ");
        fflush(stdout);
    }
}

/**
 * @brief Checks for a pipe in the input string and splits commands.
 *
 * Searches for a pipe ('|') in the input string. If found, it splits
 * the string into two commands: one before the pipe and one after.
 *
 * @param input The input command string to check for a pipe.
 * @param cmd1 Pointer to store the command before the pipe.
 * @param cmd2 Pointer to store the command after the pipe.
 * @return 1 if a pipe is detected, 0 otherwise.
 */
int is_pipe(char *input, char **cmd1, char **cmd2)
{
    char *pipe_pos = strstr(input, "|");
    if (pipe_pos != NULL)
    {
        *pipe_pos = '\0';          // Split the input at the pipe
        *cmd1 = input;             // Command before the pipe
        *cmd2 = pipe_pos + 1;      // Command after the pipe
        return 1;                  // Pipe is detected
    }
    return 0;                      // No pipe detected
}


/**
 * @brief Executes two commands connected by a pipe.
 *
 * This function creates a pipe, forks two child processes to execute
 * the specified commands. The output of the first command is connected
 * to the input of the second command through the pipe.
 *
 * @param cmd1 The command to be executed first.
 * @param cmd2 The command to be executed second.
 * 
 * @return none
 */
void execute_pipe(char *cmd1, char *cmd2)
{
    int pipefd[2];
    pid_t p1, p2;

    if (pipe(pipefd) == -1)
    {
        perror("dragonshell: pipe failed");
        return;
    }

    p1 = fork();
    if (p1 == 0)
    {
        // First child process (cmd1)
        close(pipefd[0]);            // Close read end of pipe
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to write end of pipe
        close(pipefd[1]);

        char **args1 = split_line(cmd1);
        if (execvp(args1[0], args1) == -1)
        {
            perror("dragonshell");
        }
        free(args1);
        exit(EXIT_FAILURE);
    }
    else if (p1 < 0)
    {
        perror("dragonshell: fork failed");
        return;
    }

    p2 = fork();
    if (p2 == 0)
    {
        // Second child process (cmd2)
        close(pipefd[1]);            // Close write end of pipe
        dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to read end of pipe
        close(pipefd[0]);

        char **args2 = split_line(cmd2);
        if (execvp(args2[0], args2) == -1)
        {
            perror("dragonshell");
        }
        free(args2);
        exit(EXIT_FAILURE);
    }
    else if (p2 < 0)
    {
        perror("dragonshell: fork failed");
        return;
    }

    // Parent process
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(p1, NULL, 0);            // Wait for both child processes
    waitpid(p2, NULL, 0);
}

/**
 * @brief Executes a command with optional background processing.
 *
 * This function forks a child process to execute a specified command.
 * If the command is executed in the foreground, the parent process
 * waits for it to complete and retrieves its resource usage. If executed
 * in the background, it prints the child process ID and returns immediately.
 *
 * @param args An array of arguments for the command, where the first element is the command itself.
 * @param background A flag indicating whether to run the command in the background (1) or foreground (0).
 * 
 * @return none
 */
void execute_command(char **args, int background) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process

        // Check for output redirection
        int output_fd = -1;
        for (int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], ">") == 0) {
                output_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (output_fd == -1) {
                    perror("dragonshell");
                    exit(EXIT_FAILURE);
                }
                args[i] = NULL; // Remove '>' from args
                break;
            }
        }

        // Redirect stdout if needed
        if (output_fd != -1) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        // Check for input redirection
        int input_fd = -1;
        for (int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], "<") == 0) {
                input_fd = open(args[i + 1], O_RDONLY);
                if (input_fd == -1) {
                    perror("dragonshell");
                    exit(EXIT_FAILURE);
                }
                args[i] = NULL; // Remove '<' from args
                break;
            }
        }

        // Redirect stdin if needed
        if (input_fd != -1) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }

        // Execute the command
        if (execvp(args[0], args) == -1) {
            perror("dragonshell");
        }
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent process
        if (!background) {
            // Wait for the child process to finish
            waitpid(pid, NULL, 0);
            
            // Get the resource usage of the child process
            struct rusage usage;
            getrusage(RUSAGE_CHILDREN, &usage);
            total_user_time += usage.ru_utime.tv_sec;
            total_sys_time += usage.ru_stime.tv_sec;
        } else {
            printf("PID %d is sent to background\n", pid);
        }
    } else {
        perror("dragonshell");
    }
}

/**
 * @brief Checks if a command should be run in the background.
 *
 * This function scans the command-line arguments for the presence of
 * the background operator "&". If found, it modifies the arguments 
 * array to terminate the command at that point and returns 1, indicating
 * that the command should be executed in the background. If not found, 
 * it returns 0.
 *
 * @param args An array of command-line arguments, ending with a NULL pointer.
 * @return 1 if the command should run in the background, 0 otherwise.
 */
int is_background_process(char **args)
{
    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], "&") == 0)
        {
            args[i] = NULL;
            return 1;
        }
    }
    return 0;
}

/**
 *  @brief main entry point
 * 
 * @param none
 * @return 0
 */
int main(void)
{
    char *input;
    char **tokens;
    shell_pid = getpid();

    signal(SIGINT, sigint_handler);    // Set signal handlers
    signal(SIGTSTP, sigtstp_handler);  // for SIGINT and SIGTSTP

    printf("Welcome to Dragon Shell!\n");

    while (1)
    {
        printf("dragonshell > ");
        input = read_line();

        char *cmd1 = NULL;
        char *cmd2 = NULL;

        if (is_pipe(input, &cmd1, &cmd2))
        {
            execute_pipe(cmd1, cmd2);
        }
        else
        {
            tokens = split_line(input);

            if (tokens[0] == NULL)
            {
                free(input);
                free(tokens);
                continue;
            }

            if (strcmp(tokens[0], "cd") == 0)
            {
                cd(tokens[1]);
            }
            else if (strcmp(tokens[0], "pwd") == 0)
            {
                pwd();
            }
            else if (strcmp(tokens[0], "exit") == 0)
            {
                exit_shell();
            }
            else
            {
                int background = is_background_process(tokens);
                execute_command(tokens, background);
            }

            free(tokens);
        }

        free(input);
    }

    return 0;
}
