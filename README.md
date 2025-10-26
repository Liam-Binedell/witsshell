# Witsshell

Witsshell is a small educational Unix-like command shell implemented in C (C11). It supports a few built-in commands, running external programs found on a configurable search path, simple output redirection, and running commands in parallel using `&`.

## Features

- Interactive prompt: `witsshell>` (prints to stderr as a prompt)
- Built-in commands:
  - `exit` — exits the shell (must be used with no arguments)
  - `cd <dir>` — change current working directory (exactly one argument required)
  - `path [dir1 dir2 ...]` — set the search path for external programs; if `path` is empty, external commands cannot be run
- External command execution using `execv` with search through `path`
- Parallel commands separated by `&` (commands separated by `&` are started and then waited on)
- Simple output redirection using `>` to a single file (overwrites; no append support)
- Error handling prints a single standardized message: `An error has occurred` (followed by newline) to stderr

## Build requirements

- A POSIX-compatible environment (Linux tested)
- GCC supporting C11 (`gcc`) or another C11 compiler

## Build

From the repository root run:

```bash
make
```

## Usage
- Interactive mode (Recommended for manual use):
  - ```bash
    ./witsshell
    ```
- Batch mode:
  - ```bash
    ./witsshell my-script.txt
    ```
- Command examples (Interactive mode):
  ```bash
  witsshell> path /bin /usr/bin
  witsshell> ls -l > out.txt
  witsshell> sleep 1 & echo "done"
  witsshell> cd /tmp
  witsshell> exit
  ```

## Behavioural Notes:
- The shell initializes its path to contain bin by default.
- When you use > filename, both stdout and stderr of the external command are redirected to filename (the file is created/truncated). Only a single > per command is supported.
& is used to run multiple commands in parallel on the same input line; the shell waits for all spawned children before showing the next prompt (interactive) or before proceeding in batch mode.
- `cd` requires exactly one argument. exit must be provided with no arguments to succeed; otherwise the shell prints the error message.
- If a command cannot be found or another runtime error occurs, the shell writes: "An error has occurred" to stderr.

## Limitations and Known Edge Cases:
- Parsing is simple: splitting on &, > and whitespace. Complex quoting, pipelines (|), input redirection (<), command substitution, environment variable expansion, or append redirection (>>) are not supported.
- No advanced job control (no fg/bg, no SIGTSTP handling).
- The shell duplicates the output file descriptor to both stdout and stderr for redirected commands.
