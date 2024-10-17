# dragonshell - a simple unix shell
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Author : Ahnaful Hoque
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

## System Calls Used
The following system calls were used to implement the required features of Dragon Shell:

1. **Command Execution**:
   - `fork()`: To create child processes for executing commands.
   - `execvp()`: To execute commands with arguments in the child process.
   - `waitpid()`: To wait for child processes to finish, ensuring proper synchronization.
   
   **Command:** `ls -l`  
   **Expected Output:**  
   A detailed list of files and directories in the current working directory, showing permissions, ownership, size, and modification date.

2. **Input Handling**:
   - `getline()`: To read input from stdin dynamically.
   - `strtok()`: To tokenize the input string based on delimiters.

3. **Process Management**:
   - `chdir()`: To change the current working directory with the `cd` command.
   - `getcwd()`: To retrieve the current working directory with the `pwd` command.
   
   **Command:** `cd /usr/bin`  
   **Expected Output:**  
   The current working directory changes to `/usr/bin`. No output is shown. If the directory does not exist, the expected result is an error message:  
    
   **Command:** `pwd`  
   **Expected Output:**  
   The current working directory is printed to the terminal, e.g. /cshome/akmahnaf/Desktop/cmput_379/dragonshell-AhnafulH

   
4. **Pipe Handling**:
- `pipe()`: To create a pipe for inter-process communication.
- `dup2()`: To redirect input and output streams for piped commands.

**Command:** `/usr/bin/find ./ | /usr/bin/sort` 
**Expected Output:**  
This returns a sorted list of all files and folders listed in the current working directory


5. **Signal Handling**:
- `signal()`: To set up custom signal handlers for handling interruptions and suspensions.

**Command:** `ping google.com` and then press `Ctrl + C`  
**Expected Output:**  
The command should terminate, and the shell should return to the prompt without an error message.

**Command:** `sleep 20` and then press `Ctrl + Z`  
**Expected Output:**  
The command should suspend, and the shell should return to the prompt, indicating that the job has been stopped.

**Command:** `sleep 30` (or any long-running command) and then press `Ctrl + C`  
**Expected Output:**  
The command should terminate, and the shell should return to the prompt.

6. **Resource Management**:
- `getrusage()`: To track resource usage (user time and system time) of child processes.

**Command:** `exit`  
**Expected Output:**  
When exiting the shell, the total user time and system time for all executed commands should be displayed

7. **Input & Output Redirection**:
- Supports redirecting standard output to a file or standard input from a file.
- Handles chaining input and output redirection in a single command.

**Command:** `/usr/bin/echo "Hello World" > output.txt`  
**Expected Output:**  
The file `output.txt` is created or overwritten with the text `Hello World`.

**Command:** `/usr/bin/echo "test" > ../a.out`  
**Expected Output:**  
The file `a.out` (one level up from the current directory) is created or overwritten with the text `test`. If successful, no output is shown.

**Command:** `cat < ../a.out`  
**Expected Output:**  
The contents of `a.out`, which should display `test` in the terminal.

**Command:** `cat < input.txt > output.txt`  
**Expected Output:**  
The contents of `input.txt` are read and written to `output.txt`. If `output.txt` already exists, it will be overwritten. No output is shown in the terminal.

**Command:** `sleep 10 &`
**Expected Output:**  
PID 1234 is sent to background

## Testing
The implementation of Dragon Shell was tested using the following methods:

- **Functional Testing**: Each feature (e.g., command execution, piping, background processes) was tested manually by executing various commands and verifying the expected outputs.

- **Edge Case Testing**: Commands with incorrect syntax, such as invalid paths or nonexistent commands, were tested to ensure the shell handles errors gracefully.

- **Signal Handling Tests**: The behavior of the shell was verified by sending signals (Ctrl+C and Ctrl+Z) to running commands, ensuring the shell responded correctly.


## Usage
To compile and run dragonshell, follow these steps:
**Compile the program**:
   ```bash
   make
   ./dragonshell
   make rand
   (To allow the use of ./rand inside dragonshell)


