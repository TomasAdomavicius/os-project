#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <termios.h>

#define BUFSIZE 1024

#define BACKGROUND_EXECUTION 0
#define FOREGROUND_EXECUTION 1

#define COMMAND_EXTERNAL 0
#define COMMAND_JOBS 1
#define COMMAND_FG 2
#define COMMAND_BG 3
#define COMMAND_KILL 4

struct process {
    char *command;
    int argc;
    char **argv;
    pid_t pid;
    int type;
};

struct job {
    int id;
    struct process *root;
    char *command;
    pid_t pgid;
    int mode;
};

pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;

int getCommandType(char *command) {
    if (strcmp(command, "jobs") == 0) {
        return COMMAND_JOBS;
    } else if (strcmp(command, "fg") == 0) {
        return COMMAND_FG;
    } else if (strcmp(command, "bg") == 0) {
        return COMMAND_BG;
    } else if (strcmp(command, "kill") == 0) {
        return COMMAND_KILL;
    } else {
        return COMMAND_EXTERNAL;
    }
}

struct process* createProcess(char *line) {
    int argc = 0;
    char *command = strdup(line);
    char *token;
    char **tokens = (char**) malloc(BUFSIZE * sizeof(char*));

    token = strtok(line, " ");
    while (token != NULL) {
        tokens[argc] = token;
        argc++;
        token = strtok(NULL, " ");
    }

    struct process *proc = (struct process*) malloc(sizeof(struct process));
    proc->command = command;
    proc->argv = tokens;
    proc->argc = argc;
    proc->pid = -1;
    proc->type = getCommandType(tokens[0]);
    return proc;
}

struct job* createJob(char *line) {
    int mode = FOREGROUND_EXECUTION;

    if (line[strlen(line) - 1] == '&') {
        mode = BACKGROUND_EXECUTION;
        line[strlen(line) - 1] = '\0';
    }

    struct job *job = (struct job*) malloc(sizeof(struct job));
    job->root = createProcess(line);
    job->command = line;
    job->pgid = -1;
    job->mode = mode;
    return job;
}

char* readLine() {
    int bufsize = BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if(!buffer) {
        fprintf(stderr, "allocation error\n");
        exit(1);
    }

    while(1) {
        c = getchar();

        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        if (position >= bufsize) {
            bufsize += BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "allocation error\n");
                exit(1);
            }
        }
    }
}

void loop() {
    char *line;
    struct job *job;

    while(1) {
        printf("> ");
        line = readLine();
        if(strlen(line) == 0) {
            continue;
        }
        // todo
        if(strcmp(line, "exit") == 0) {
            exit(0);
        } 
        job = createJob(line);
    }
}

/* Make sure the shell is running interactively as the foreground job before proceeding. */
void init() {
  /* See if we are running interactively.  */
  shell_terminal = STDIN_FILENO;
  shell_is_interactive = isatty (shell_terminal);

  if (shell_is_interactive)
    {
      /* Loop until we are in the foreground.  */
      while (tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp ()))
        kill (- shell_pgid, SIGTTIN);

      /* Ignore interactive and job-control signals.  */
      signal (SIGINT, SIG_IGN);
      signal (SIGQUIT, SIG_IGN);
      signal (SIGTSTP, SIG_IGN);
      signal (SIGTTIN, SIG_IGN);
      signal (SIGTTOU, SIG_IGN);
      signal (SIGCHLD, SIG_IGN);

      /* Put ourselves in our own process group.  */
      shell_pgid = getpid ();
      if (setpgid (shell_pgid, shell_pgid) < 0)
        {
          perror ("Couldn't put the shell in its own process group");
          exit (1);
        }

      /* Grab control of the terminal.  */
      tcsetpgrp (shell_terminal, shell_pgid);

      /* Save default terminal attributes for shell.  */
      tcgetattr (shell_terminal, &shell_tmodes);
    }
}

int main(int argc, char **argv) {
    init();
    loop();
    exit(0);
}