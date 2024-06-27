# Linux Shell

The shell accepts user commands and executes each command in a separate process, providing basic shell functionalities including process handling, internal commands, and a history feature.

## Features

1. **Command Execution**: Executes user commands in separate child processes using `fork()` and `execvp()`.
2. **Background Execution**: Allows commands to run in the background by appending `&` at the end of the command.
3. **Internal Commands**: 
   - `exit`: Exit the shell program.
   - `pwd`: Display the current working directory.
   - `cd`: Change the current working directory.
   - `help`: Display help information on internal commands.
4. **Shell Prompt**: Displays the current working directory in the shell prompt.
5. **History Feature**: Maintains a history of the 10 most recent commands.
   - `history`: Display the 10 most recent commands.
   - `!n`: Re-run the command numbered `n` from the history.
   - `!!`: Re-run the previous command.
   - `!-`: Clear all previous commands from the history.
6. **Signal Handling**: Custom signal handler for `SIGINT` to display help information when `Ctrl+C` is pressed.

## Getting Started

### Prerequisites

- Linux operating system
- GCC compiler

### Installation

1. Clone the repository:
    ```sh
    git clone https://github.com/ahmad-masud/Linux-Shell
    cd ShellCraft
    ```

2. Build the project:
    ```sh
    make
    ```

3. Run the shell:
    ```sh
    ./shell
    ```

### Usage

- To run a command, simply type it at the prompt and press Enter.
- To run a command in the background, append `&` at the end of the command.
- Use internal commands like `pwd`, `cd`, `exit`, and `help` as needed.
- Use `history` to view recent commands and `!n` to re-run a specific command from the history.

## Example

```sh
/home/user$ ls -l
/home/user$ cd /usr
/usr$ pwd
/usr
/usr$ history
30 history
29 cd /usr
28 ls -l
...
/usr$ !28
ls -l
```

## Files

- `shell.c`: Main source code for the shell.
- `Makefile`: Build instructions for the project.
- 
## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
