#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <stdbool.h>
#include "direct_assignment.h"
#include "operator_evaluation.h"
#include "expression_validation.h"


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
#define COMMAND_VALID_EXPRESSION 6
#define COMMAND_INVALID_EXPRESSION 7
#define COMMAND_EXPRESSION_OPERATOR 8
#define COMMAND_DISPLAY_RESULT 9

#define STATUS_RUNNING 0
#define STATUS_DONE 1
#define STATUS_SUSPENDED 2
#define STATUS_TERMINATED 3
#define STATUS_CONTINUED 4

const char* STATUS_STRING[] = {
    "running",
    "done",
    "suspended",
    "terminated",
    "continued"
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
struct evaluation_factors eval;

pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;

pid_t waitpid(pid_t pid, int *stat_loc, int options);

struct evaluation_factors initialize_expression() 
{
    eval.assignment = "";
    eval.variable_name = "";
    eval.evaluating_value = false;
    eval.valid = false;
    return eval;
}

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

void printJobStatus(int id) {
    printf("[%d]\t%d\t%s\t%s\n", id, shell->jobs[id]->root->pid, STATUS_STRING[shell->jobs[id]->root->status], shell->jobs[id]->root->command);
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
        printJobStatus(id);
    }
    return status;
}

int getCommandType(char *command) {
    int print_statement = strcmp(command, "print");
    
    if (print_statement > 0) eval = verify_assignment_syntax(eval, command);
    // printf("%i\n", print_statement);
    // printf("%i\n", eval.valid);

    if (strcmp(command, "jobs") == 0) return COMMAND_JOBS;
    else if (strcmp(command, "fg") == 0) return COMMAND_FG;
    else if (strcmp(command, "bg") == 0) return COMMAND_BG;
    else if (strcmp(command, "kill") == 0) return COMMAND_KILL;
    else if (strcmp(command, "exit") == 0) return COMMAND_EXIT;
    else {
        if (strcmp(command, "sleep") == 0) return COMMAND_EXTERNAL;
        if (eval.valid && print_statement > 0) {
            if (!eval.evaluating_value) return COMMAND_VALID_EXPRESSION;
            else return COMMAND_EXPRESSION_OPERATOR;
        }
        if (!eval.valid) {
            if (print_statement > 0) return COMMAND_INVALID_EXPRESSION;
            else if (print_statement == 0) return COMMAND_DISPLAY_RESULT;
        }
        if (eval.valid && print_statement == 0) return COMMAND_DISPLAY_RESULT;
        return COMMAND_EXTERNAL;
    }
}

int jobs(int argc, char **argv) {
    for(int i = 0; i < NR_OF_JOBS; i++) {
        if (shell->jobs[i] != NULL) {
            printJobStatus(i);
        }
    }
    return 0;
}

int fg(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: fg <pid>\n");
        return -1;
    }

    int status;
    pid_t pid;
    int id = -1;

    if (argv[1][0] == '%') {
        id = atoi(argv[1] + 1);
        pid = shell->jobs[id]->pgid;
        if (pid < 0) {
            printf("fg %s: no such job\n", argv[1]);
            return -1;
        }
    } else {
        pid = atoi(argv[1]);
    }

    if (kill(-pid, SIGCONT) < 0) {
        printf("fg %d: job not found\n", pid);
        return -1;
    }

    tcsetpgrp(0, pid);

    if (id > 0) {
        if (shell->jobs[id]->root->status != STATUS_DONE) {
            shell->jobs[id]->root->status = STATUS_CONTINUED;
        }
        printJobStatus(id);
        if (waitForJob(id) >= 0) {
            removeJob(id);
        }
    } else {
        int s = 0;
        waitpid(pid, &s, WUNTRACED);
        if (WIFEXITED(s)) {
            setProcessStatus(pid, STATUS_DONE);
        } else if (WIFSIGNALED(s)) {
            setProcessStatus(pid, STATUS_TERMINATED);
        } else if (WSTOPSIG(s)) {
            setProcessStatus(pid, STATUS_SUSPENDED);
        }
    }

    signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(0, getpid());
    signal(SIGTTOU, SIG_DFL);

    return 0;
}

int bg(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: bg <pid>\n");
        return -1;
    }

    pid_t pid;
    int id = -1;

    if (argv[1][0] == '%') {
        id = atoi(argv[1] + 1);
        pid = shell->jobs[id]->pgid;
        if (pid < 0) {
            printf("bg %s: no such job\n", argv[1]);
            return -1;
        }
    } else {
        pid = atoi(argv[1]);
    }

    if (kill(-pid, SIGCONT) < 0) {
        printf("bg %d: job not found\n", pid);
        return -1;
    }

    if (id > 0) {
        if (shell->jobs[id]->root->status != STATUS_DONE) {
            shell->jobs[id]->root->status = STATUS_CONTINUED;
        }
        printJobStatus(id);
    }

    return 0;
}

int killJob(int argc, char **argv) {
    pid_t pid;
    int id = -1;

    if (argv[1][0] == '%') {
        id = atoi(argv[1] + 1);
        pid = shell->jobs[id]->pgid;
        if (pid < 0) {
            printf("kill %s: no such job\n", argv[1]);
            return -1;
        }
        pid = -pid;
    } else {
        pid = atoi(argv[1]);
    }

    if (kill(pid, SIGKILL) < 0) {
        printf("kill %d: job not found\n", pid);
        return 0;
    }

    if (id > 0) {
        if (shell->jobs[id]->root->status != STATUS_DONE) {
            shell->jobs[id]->root->status = STATUS_TERMINATED;
        }
        printJobStatus(id);
        if (waitForJob(id) >= 0) {
            removeJob(id);
        }
    }

    return 1;
}

int executeBuiltinCommand(struct process *proc) {
    switch (proc->type) {
        case COMMAND_EXIT:
            exit(0);
        case COMMAND_JOBS:
            jobs(proc->argc, proc->argv);
            break;
        case COMMAND_FG:
            fg(proc->argc, proc->argv);
            break;
        case COMMAND_BG:
            bg(proc->argc, proc->argv);
            break;
        case COMMAND_KILL:
            killJob(proc->argc, proc->argv);
            break;
        case COMMAND_VALID_EXPRESSION:
            break;
        case COMMAND_EXPRESSION_OPERATOR:
            eval = verify_expr_syntax(eval, proc->argv);
            eval = extract_evaluate(eval);
            if (!eval.valid) eval = initialize_expression();
            break;
        case COMMAND_INVALID_EXPRESSION:
            printf("%s: command not found\n", eval.assignment);
            break;
        case COMMAND_DISPLAY_RESULT:
            if (!eval.valid) break;
            display_assignment_result(eval, proc->command);
            break;
        default:
            return 0;
    }
    return 1;
}

int getJobIdByPid(int pid) {
    struct process *proc;

    for (int i = 1; i <= NR_OF_JOBS; i++) {
        if (shell->jobs[i] != NULL) {
            proc = shell->jobs[i]->root;
            if (proc->pid == pid) {
                return i;
            }
        }
    }

    return -1;
}

void checkZombie() {
    int status, pid;
    while ((pid = waitpid(-1, &status, WNOHANG|WUNTRACED|WCONTINUED)) > 0) {
        if (WIFEXITED(status)) {
            setProcessStatus(pid, STATUS_DONE);
        } else if (WIFSTOPPED(status)) {
            setProcessStatus(pid, STATUS_SUSPENDED);
        } else if (WIFCONTINUED(status)) {
            setProcessStatus(pid, STATUS_CONTINUED);
        }

        int job_id = getJobIdByPid(pid);
        if (job_id > 0 && shell->jobs[job_id]->root->status == STATUS_DONE) {
            printJobStatus(job_id);
            removeJob(job_id);
        }
    }
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

    checkZombie();
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
    char *command = strdup(line);

    if (line[strlen(line) - 1] == '&') {
        mode = BACKGROUND_EXECUTION;
        line[strlen(line) - 1] = '\0';
    }

    struct job *job = (struct job*) malloc(sizeof(struct job));
    job->root = createProcess(line);
    job->command = command;
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

    // Initialize values for assignment evaluation preventing error prompt
    eval = initialize_expression();

    while (1) {
        printf("> ");
        line = readLine();

        if(strlen(line) == 0) {
            checkZombie();
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
