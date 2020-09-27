#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

/* max 10 but extra for null and cmd */
#define MAX_ARGS 12
#define MAX_COMMANDS 20
#define MAX_LINE 1025

typedef struct Command {
   int inFD;
   int outFD;
   char *args[MAX_ARGS]; 
} Command;

typedef struct Pipeline {
   Command cmds[MAX_COMMANDS];
   int numCmds;
} Pipeline;

int createChild(Command cmd, int oldIN);
void waitChildren(pid_t cpids[], int n);
int launch(Pipeline pipeline);

#endif
