#include "launcher.h"

#define MAX_TOKENS 256
#define DELIMS " \t\r\n\a"

int openOut(char *outFile) {

   int fd;
   if((fd = open(outFile, O_CREAT | O_WRONLY | O_TRUNC, 0666)) == -1) {
      fprintf(stderr, "cshell: Unable to open file for output\n");
      return -1;
   }
   else
      return fd;
}

int openIn(char *inFile) {

   int fd;
   if((fd = open(inFile, O_RDONLY)) == -1) {
      fprintf(stderr, "cshell: Unable to open file for input\n");
      return -1;
   }
   else
      return fd;
}

int makePipe(Pipeline *p, char *args[]) {
   char *inFile, *outFile;
   unsigned cmd_i_INsave, cmd_i_OUTsave, fd, line_i, cmd_i, arg_i;
   line_i = cmd_i = arg_i = 0;
   /* Initialize default STDIN/OUT file descriptors for first command*/
   inFile = outFile = NULL;
   p->cmds[0].inFD = STDIN_FILENO;
   p->cmds[0].outFD = STDOUT_FILENO;
   /* initialize pipeline number of commands to 1 */
   p->numCmds = 1;
   for(line_i = 0; args[line_i] != NULL; line_i++) {
     
      /* hit pipe */
      if(0 == strcmp(args[line_i], "|")) {
         if(arg_i == 0 || args[line_i+1] == NULL) {
            fprintf(stderr, "cshell: Invalid pipe\n");
            return 1;
         }
         else if(p->numCmds >= MAX_COMMANDS) {
            fprintf(stderr, "cshell: Too many commands\n");
            return 1;
         }
         else {
            /* VALID pipe - new command */
            p->cmds[cmd_i].args[arg_i] = NULL; 
            /* NULL terminate previous cmds arg list */
            p->numCmds++;
            cmd_i++;
            arg_i = 0;
            p->cmds[cmd_i].inFD = STDIN_FILENO;
            p->cmds[cmd_i].outFD = STDOUT_FILENO;
         }
      }
      /* INPUT redirection */
      else if(0 == strcmp(args[line_i], "<")) {
         inFile = args[line_i+1];
         line_i += 1;
         cmd_i_INsave = cmd_i; 
      }
      /* OUTPUT redirection */
      else if(0 == strcmp(args[line_i], ">")) {
         outFile = args[line_i+1];
         line_i += 1;
         cmd_i_OUTsave = cmd_i;
      }
      /* NOT pipe or redir -> some arg or command */
      else if(arg_i > MAX_ARGS-2) {
         fprintf(stderr, "cshell: %s: Too many arguments\n", 
            p->cmds[cmd_i].args[0]);
         return 1;
      }
      else {
         /* store cmd OR arg into commands arg list */
         p->cmds[cmd_i].args[arg_i] = args[line_i];
         arg_i++;
      }
   }

   if(inFile) {
      if((fd = openIn(inFile)) != -1)
         p->cmds[cmd_i_INsave].inFD = fd;
      else
         return 1;
   }

   if(outFile) {
      if((fd = openOut(outFile)) != -1)
         p->cmds[cmd_i_OUTsave].outFD = fd;
      else
         return 1;
   }

   /* NULL terminate last commands arg list - no more pipes */
   p->cmds[cmd_i].args[arg_i] = NULL;
   
   /* success */
   return 0;
}

int getLine(char line[MAX_LINE]) {

   /* NOTE: MAX_LINE = 1025 (+1 for null term)*/
   int c, i;
   for(i = 0; i < MAX_LINE-1 && (c=getchar()) != EOF && c != '\n'; i++)
      line[i] = c;

   line[i] = '\0';
   /* if i = 1024 -> Line was too long -> return -1, else line length*/
   return (i == MAX_LINE-1) ? -1 : i; 
}

int parseLine(char *line, char **args) {

   int i = 0;
   char *token;
   token = strtok(line, DELIMS);      
   while(token != NULL) {
      args[i] = token;
      i++;
      token = strtok(NULL, DELIMS);
   }
   args[i] = NULL;
   return i;
}

void flushLine() {
   int c;
   while ((c = getchar()) != '\n' && c != EOF);
}

void runShell() {
   char line[MAX_LINE];
   char *args[MAX_TOKENS];
   int validPipe, status = 1;
   Pipeline p;
   do {
      printf(":-) ");
      if(getLine(line) == -1) {
         fprintf(stderr, "cshell: Command line too long\n");
         flushLine();         
      }
      else if(feof(stdin)) {
         printf("exit\n");
         status = 0;
      }
      else if(0 == strcmp(line, "exit"))
         status = 0;
      else {
         parseLine(line, args);
         validPipe = makePipe(&p, args);
         if(!validPipe)
            launch(p);
      }
   } while(status);
}
      
int main(int argc, char *argv[]) {
   setbuf(stdout, NULL);
   runShell();
   return EXIT_SUCCESS;
}
