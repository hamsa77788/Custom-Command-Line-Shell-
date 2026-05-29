#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_LINE 1024
#define MAX_ARGS 100
#define HISTORY_SIZE 100

char *history[HISTORY_SIZE];
int history_count = 0;
pid_t foreground_pid = -1;

void add_history(char *cmd) {
    if (history_count < HISTORY_SIZE)
        history[history_count++] = strdup(cmd);
}

void print_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
}

void free_history() {
    for (int i = 0; i < history_count; i++)
        free(history[i]);
}

void handle_sigint(int sig) {
    (void)sig;
    if (foreground_pid != -1) {
        kill(foreground_pid, SIGKILL);
        printf("\nProcess %d terminated\n", foreground_pid);
    } else {
        printf("\nmyShell> ");
        fflush(stdout);
    }
}

void handle_sigtstp(int sig) {
    (void)sig;

    if (foreground_pid != -1) {
        kill(foreground_pid, SIGTSTP);
        printf("\nProcess %d stopped\n", foreground_pid);
        fflush(stdout);

        foreground_pid = -1;
    }
}


void parse(char *line, char **args) {
    int i = 0;
    args[i] = strtok(line, " \t\n");
    while (args[i] != NULL && i < MAX_ARGS - 1)
        args[++i] = strtok(NULL, " \t\n");
    args[i]=NULL;
}

void redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            if (args[i+1] == NULL) {
                fprintf(stderr, "Syntax error: Expected file after '>'\n");
                exit(1);
            }
            int fd = open(args[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open failed");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
        }
        else if (strcmp(args[i], "<") == 0) {
            if (args[i+1] == NULL) {
                fprintf(stderr, "Syntax error: Expected file after '<'\n");
                exit(1);
            }
            int fd = open(args[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("open failed");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            args[i] = NULL;
        }
    }
}

void execute(char **args, int background) {
    pid_t pid = fork();
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        redirection(args);
        execvp(args[0], args);
        perror("myShell: Command not found");
        exit(1);
    } else if (pid > 0) {
        foreground_pid = pid;
        if (!background) {
            waitpid(pid, NULL, WUNTRACED);
        } else {
            printf("[Background PID %d]\n", pid);
        }
        foreground_pid = -1;
    } else {
        perror("fork failed");
    }
}

void execute_pipe(char *cmd1, char *cmd2) {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe failed");
        return;
    }
    pid_t p1 = fork();
    if (p1 < 0) {
        perror("fork failed");
        return;
    }
    if (p1 == 0) {
        signal(SIGINT, SIG_DFL);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        char *args1[MAX_ARGS];
        parse(cmd1, args1);
        redirection(args1);
        execvp(args1[0], args1);
        perror("first command failed");
        exit(1);
    }
    pid_t p2 = fork();
    if (p2 < 0) {
        perror("fork failed");
        return;
    }
    if (p2 == 0) {
        signal(SIGINT, SIG_DFL);
        dup2(fd[0], STDIN_FILENO);
        close(fd[1]);
        close(fd[0]);
        char *args2[MAX_ARGS];
        parse(cmd2, args2);
        redirection(args2);
        execvp(args2[0], args2);
        perror("second command failed");
        exit(1);
    }
    close(fd[0]);
    close(fd[1]);
    waitpid(p1, NULL, 0);
    waitpid(p2, NULL, 0);
}

int handle_builtin(char *line) {
    if (strcmp(line, "pwd") == 0) {
        char cwd[MAX_LINE];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd failed");
        }
        return 1;
    }
    if (strcmp(line, "history") == 0) {
        print_history();
        return 1;
    }
    if (strcmp(line, "exit") == 0) {
        free_history();
        exit(0);
    }
    if (strncmp(line, "cd ", 3) == 0 || strcmp(line, "cd") == 0) {
        char *dir = strtok(line + 2, " \t\n");
        if (dir == NULL) {
            fprintf(stderr, "cd: missing directory\n");
        } else {
            if (chdir(dir) != 0) {
                perror("cd failed");
            }
        }
        return 1;
    }
    return 0;
}

int main() {
    char line[MAX_LINE];
    char *args[MAX_ARGS];
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);
    while (1) {
        while (waitpid(-1, NULL, WNOHANG) > 0);
        printf("myShell> ");
        fflush(stdout);
        if (fgets(line, MAX_LINE, stdin) == NULL) {
            printf("\n");
            break;
        }
        if (strlen(line) <= 1)
            continue;
        line[strcspn(line, "\n")] = '\0';
        add_history(line);
        if (handle_builtin(line))
            continue;
        char line_copy[MAX_LINE];
        strcpy(line_copy, line);
        char *pipe_pos = strchr(line_copy, '|');
        if (pipe_pos) {
            *pipe_pos = '\0';
            execute_pipe(line_copy, pipe_pos + 1);
            continue;
        }
        parse(line_copy, args);
        if (args[0] == NULL)
            continue;
        int background = 0;
        for (int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], "&") == 0) {
                background = 1;
                args[i] = NULL;
                break;
            }
        }
        execute(args, background);
    }
    free_history();
    return 0;
}
