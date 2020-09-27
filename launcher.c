#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "launcher.h"

void dupIn(Command cmd) {

   if(cmd.inFD != STDIN_FILENO) {
      if(-1 == dup2(cmd.inFD, STDIN_FILENO)) {
         perror(NULL);
         exit(EXIT_FAILURE);
      }
      close(cmd.inFD);
   }
}

void dupOut(Command cmd) {

   if(cmd.outFD != STDOUT_FILENO) {
      if(-1 == dup2(cmd.outFD, STDOUT_FILENO)) {
         perror(NULL);
         exit(EXIT_FAILURE);
      }
      close(cmd.outFD);
   }
}
 
int createChild(Command cmd, int oldIN) {

   pid_t cpid;
   /* FORK */
   if ((cpid = fork()) < 0) {
      perror("fork error");
      exit(EXIT_FAILURE);
   }
   /* CHILD */
   if (cpid == 0) {
      dupIn(cmd);
      dupOut(cmd);
      if(oldIN != 0)
         close(oldIN);
      execvp(cmd.args[0], cmd.args);      
      fprintf(stderr, "cshell: %s: Command not found\n", cmd.args[0]);
      exit(127);
   }
   else {
      return cpid;
   }
}

void waitChildren(pid_t cpids[], int n) {

   int i, status;
   pid_t pid;
   for(i = 0; i < n; i++) {
      if((pid = waitpid(cpids[i], &status, 0)) < 0) {
         fprintf(stderr, "waitpid error");
      }
   }
}

int launch(Pipeline pipeline) {

   int i;
   pid_t cpids[MAX_COMMANDS];
   int pipefd[2];
   int saveFD;
   int inRD, outRD;
   for(i = 0; i < pipeline.numCmds; i++) {

      inRD = outRD = -1;

      /* PIPE */
      if(i != (pipeline.numCmds - 1)) {
         /* if not the last child or only 1 cmd -> pipe*/
         if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
         }
      }
     
      if(pipeline.cmds[i].inFD != 0) {
         inRD = pipeline.cmds[i].inFD;
      }
      if(pipeline.cmds[i].outFD != 1) {
         outRD = pipeline.cmds[i].outFD;
      }

      /* Assign childs pipe FDs */
      if(i == 0) { 
         /* first child */
         pipeline.cmds[i].inFD = STDIN_FILENO;
         if(pipeline.numCmds > 1)
            pipeline.cmds[i].outFD = pipefd[1];
         else 
            pipeline.cmds[i].outFD = STDOUT_FILENO;
      }
      else if(i == (pipeline.numCmds - 1)) { /* last child */
         pipeline.cmds[i].inFD = saveFD;
         pipeline.cmds[i].outFD = STDOUT_FILENO;
      }
      else {
         /* set in to previous out */
         pipeline.cmds[i].inFD = saveFD;
         pipeline.cmds[i].outFD = pipefd[1];
      }

      if(inRD != -1)
         pipeline.cmds[i].inFD = inRD;

      if(outRD != -1)
         pipeline.cmds[i].outFD = outRD;

      /* Create Child and return cpid */
      cpids[i] = createChild(pipeline.cmds[i], pipefd[0]);

      /* PARENT */
      if(inRD != -1)
         close(inRD);

      if(outRD != -1)
         close(outRD);

      if(i != (pipeline.numCmds - 1)) {
         /* dont close on last child - didnt pipe*/ 
         close(pipefd[1]); 
         /* close parent write fd for next pipe*/
      }
      
      /* close old saved inFD and save new inFD */
      if(i > 0)
         close(saveFD);

      saveFD = pipefd[0];
      /* leave read open for the next child to use - child closes it */
   }
   waitChildren(cpids, pipeline.numCmds);
   return 0;
}
