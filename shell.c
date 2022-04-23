#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <termios.h>

#define NR_OF_JOBS 20
#define BUFSIZE 1024

#define BACKGROUND_EXECUTION 0
#define FOREGROUND_EXECUTION 1

#define COMMAND_EXTERNAL 0
#define COMMAND_JOBS 1
#define COMMAND_FG 2
#define COMMAND_BG 3
#define COMMAND_KILL 4
#define COMMAND_EXIT 5

#define STATUS_RUNNING 0
#define STATUS_DONE 1
#define STATUS_SUSPENDED 2
#define STATUS_TERMINATED 3

const char* STATUS_STRING[] = {
    "running",
    "done",
    "suspended",
    "terminated"
};

struct process {
    char *command;
    int argc;
    char **argv;
    pid_t pid;
    int type;
    int status;
};

struct job {
    int id;
    struct process *root;
    char *command;
    pid_t pgid;
    int mode;
};

struct shell_info {
    struct job *jobs[NR_OF_JOBS + 1];
};

struct shell_info *shell;

pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;

int insertJob(struct job *job) {
    int id = 1;
    while(shell->jobs[id] != NULL) {
        id++;
    }

    job->id = id;
    shell->jobs[id] = job;
    return id;
}

int releaseJob(int id) {
    if (id > NR_OF_JOBS || shell->jobs[id] == NULL) {
        return -1;
    }

    struct job *job = shell->jobs[id];
    struct process *proc;
    proc = job->root;
    free(proc->command);
    free(proc->argv);
    free(proc);

    free(job->command);
    free(job);
    return 0;
}

int removeJob(int id) {
    if (id > NR_OF_JOBS || shell->jobs[id] == NULL) {
        return -1;
    }

    releaseJob(id);
    shell->jobs[id] = NULL;
    return 0;
}

int setProcessStatus(int pid, int status) {
    int i;
    struct process *proc;

    for (i = 1; i <= NR_OF_JOBS; i++) {
        if (shell->jobs[i] == NULL) {
            continue;
        }
        proc = shell->jobs[i]->root;
        if (proc->pid == pid) {
            proc->status = status;
            return 0;
        }
    }
    return -1;
}

int waitForJob(int id) {
    if (id > NR_OF_JOBS || shell->jobs[id] == NULL) {
        return -1;
    }

    int wait_pid = -1;
    int status = 0;

    wait_pid = waitpid(-shell->jobs[id]->pgid, &status, WUNTRACED);

    if (WIFEXITED(status)) {
        setProcessStatus(wait_pid, STATUS_DONE);
    } else if (WIFSIGNALED(status)) {
        setProcessStatus(wait_pid, STATUS_TERMINATED);
    } else if (WSTOPSIG(status)) {
        status = -1;
        setProcessStatus(wait_pid, STATUS_SUSPENDED);
        printf("[%d]\t%d\t%s\t%s\n", id, shell->jobs[id]->root->pid, STATUS_STRING[shell->jobs[id]->root->status], shell->jobs[id]->root->command);
    }
    return status;
}

int getCommandType(char *command) {
    if (strcmp(command, "jobs") == 0) {
        return COMMAND_JOBS;
    } else if (strcmp(command, "fg") == 0) {
        return COMMAND_FG;
    } else if (strcmp(command, "bg") == 0) {
        return COMMAND_BG;
    } else if (strcmp(command, "kill") == 0) {
        return COMMAND_KILL;
    } else if (strcmp(command, "exit") == 0) {
        return COMMAND_EXIT;
    } else {
        return COMMAND_EXTERNAL;
    }
}

int jobs(int argc, char **argv) {
    for(int i = 0; i < NR_OF_JOBS; i++) {
        if (shell->jobs[i] != NULL) {
            printf("[%d] %d\n", i, shell->jobs[i]->root->pid);
        }
    }
    return 0;
}

int executeBuiltinCommand(struct process *proc) {
    switch (proc->type) {
        case COMMAND_EXIT:
            exit(0);
        case COMMAND_JOBS:
            jobs(proc->argc, proc->argv);
            break;
        default:
            return 0;
    }
    return 1;
}

int launchProcess(struct job *job, struct process *proc, int mode) {
    proc->status = STATUS_RUNNING;
    if (proc->type != COMMAND_EXTERNAL && executeBuiltinCommand(proc)) {
        return 0;
    }

    pid_t childpid;
    int status = 0;

    childpid = fork();

    if (childpid < 0) {
        return -1;
    } else if (childpid == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        proc->pid = getpid();
        if (job->pgid > 0) {
            setpgid(0, job->pgid);
        } else {
            job->pgid = proc->pid;
            setpgid(0, job->pgid);
        }

        if (execvp(proc->argv[0], proc->argv) < 0) {
            printf("command not found\n");
            exit(0);
        }

        exit(0);
    } else {
        proc->pid = childpid;
        if (job->pgid > 0) {
            setpgid(childpid, job->pgid);
        } else {
            job->pgid = proc->pid;
            setpgid(childpid, job->pgid);
        }

        if (mode == FOREGROUND_EXECUTION) {
            tcsetpgrp(0, job->pgid);
            status = waitForJob(job->id);
            signal(SIGTTOU, SIG_IGN);
            tcsetpgrp(0, getpid());
            signal(SIGTTOU, SIG_DFL);
        }
    }
    return status;
}

int launchJob(struct job *job) {
    int status = 0, jobId = -1;

    if (job->root->type == COMMAND_EXTERNAL) {
        jobId = insertJob(job);
    }

    status = launchProcess(job, job->root, job->mode);

    if (job->root->type == COMMAND_EXTERNAL) {
        if (status >= 0 && job->mode == FOREGROUND_EXECUTION) {
            removeJob(jobId);
        } else if (job->mode == BACKGROUND_EXECUTION) {
            printf("[%d] %d\n", jobId, job->root->pid);
        }
    }
    return status;
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
        job = createJob(line);
        launchJob(job);
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

    shell = (struct shell_info*) malloc(sizeof(struct shell_info));
    for (int i = 0; i < NR_OF_JOBS; i++) {
        shell->jobs[i] = NULL;
    }
}

int main(int argc, char **argv) {
    init();
    loop();
    exit(0);
}