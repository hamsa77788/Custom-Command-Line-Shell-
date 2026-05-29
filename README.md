# Simple Unix Shell

## Project Description
This project is a simple implementation of a Unix-like shell written in C.
It supports executing system commands, handling pipes, redirection, background processes, signal handling, and built-in commands.

---

## Features

- Execute system commands using fork() and execvp()
- Built-in commands:
  - cd
  - exit
  - pwd
  - history
- Input/Output redirection:
  - > redirect output to file
  - < read input from file
- Pipes support:
  - |
- Background processes using &
- Signal handling:
  - Ctrl + C terminates foreground process only
  - Ctrl + Z stops foreground process only
- Error Handling
  - Command not found
  - File and directory errors
  - Fork and pipe failures


---

## Project Structure
```
SP_13/
├── myShell.c
├── Makefile
└── README.md
```
---

## How to Build & Run

### Compile:
```bash
make
```

---

## Running the Shell

```bash
./myShell
```

---

## Team Contributions

All team members worked together collaboratively on all parts of the project.
