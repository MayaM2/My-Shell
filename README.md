# My-Shell

This code was a home work assignment (Opearting Systems course at TAU, fall 2018). The course's staff provided a skeleton program that reads lines from the user and parses them into commands (not included here).

I implemented process_arglist() which handles the command, and also prepare() and finalize() as described below.

## Shell specifications:
1. Behavior of process_arglist():
  - Commands should be executed as a child process using fork and execvp.
  - In the original (shell/parent) process, process_arglist() should always return 1. This makes sure the shell continues processing user commands.
  - Support background processes:
    -If the last argument of the arglist array is “&” (ampersand only), run the child process in the background: the parent should not wait for the child process to finish, but instead continue executing commands.
    - Assume background processes don’t read input (stdin), that “&” can only appear as the last argument, and that it’s always separated by a whitespace from the previous argument.
  - Support nameless pipes:
    - The process_arglist() should not return until the last foreground child process it created exits.
  - No more than a single pipe (|) is provided, and it is correctly placed.
  - Pipes and background processes will not be combined in one command.
  - Not supportting built-in shell commands such as cd and exit, only executes of program binaries as described above.
  - Not supportting arguments with quotation marks or apostrophes.
2. Error handling:
  - If an error occurs in the parent process, print a proper error message and have process_arglist() return 0. 
  - If an error occurs in a child process (before it execs), output a proper error message and terminate only the child process with exit(1). 
  - The user command might be invalid. This should be treated as an error in the child process.
  - Print error messages to stderr.
  - Not necessarily exiting cleanly upon error.
3. General requirements:
  - Prevent zombies and remove them as fast as possible.
  - SIGINT handling:
    - After prepare() finishes, the parent (shell) does not terminate upon a SIGINT.
    - Foreground child processes (regular commands or parts of a pipe) terminate upon SIGINT.
    - Background child processes does not terminate upon SIGINT.
  - The skeleton calls a prepare() function when it starts and finalize() function before it exits. Both return 0 upon success. Any other return values indicates an error.
