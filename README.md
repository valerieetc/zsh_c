# Welcome to Zsh
***


## Description
Zsh is a custom Unix shell implemented as a read-parse-execute loop. User input is received with getline() and parsed by 
splitting on spaces while handling quote marks. If valid, the command is executed as a builtin command, 
an executable file with a direct path (e.g. ./project) or a binary. For executable files and binaries fork() 
is used and execve() is called in the child process. 

The shell supports the following builtins: `echo`, `cd`, `pwd`, `env`, `setenv`, `unsetenv`, `which`, `exit`, `quit`.

## Installation
You can install the program by running make.
As a prerequisite you need to have a GCC compiler and the Make utility.

## Usage
You can run the program the following way:
```
./my_zsh
```
This will launch the CLI where you will be able to input your commands.
Commands are received by the program when enter is pressed. To quit the program, type "exit" or "quit".
