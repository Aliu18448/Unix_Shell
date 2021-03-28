
// ANDY LIU
// UNIX SHELL ATTEMPT
// MARCH 2021

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>


int c_args_offfset(char** c_args);

int c_args_offset(char** c_args){
    int idx = 0;
    while(c_args[idx] != '\0'){
        if(!strncmp(c_args[idx], "|", 1)){// found pipe
            return idx;/* new c_args starting at offset */
        }
        ++idx;
    }
    return -1;
}

char** parse(char* s, char* c_args[]) {
  char** words = malloc(2 * sizeof(char*));
  char break_chars[] = " \t"; /* delmiters of space and tab */
  int i = 0;

  char* p = strtok(s, break_chars); /*token created based on delimiters */
  c_args[0] = p;
  words[0] = malloc(BUFSIZ * sizeof(char));
  words[1] = malloc(BUFSIZ * sizeof(char));
  words[0] = "";
  words[1] = "";
  while (p != NULL) {
    p = strtok(NULL, break_chars);
    if (p == NULL)
        break;
    if (!strncmp(p, ">", 1)){
        p = strtok(NULL, break_chars);
        words[0] = "o";
        words[1] = p;
        return words;
    }
    else if (!strncmp(p, "<", 1)){
        p = strtok(NULL, break_chars);
        words[0] = "i";
        words[1] = p;
        return words;
    }
    else if (!strncmp(p, "|", 1)){
        words[0] = "p";
    }
    c_args[++i] = p;
  }
  return words;
}

int main(int argc, const char * argv[]) {
 char input [BUFSIZ];
 char RC [BUFSIZ]; /* recent memory cache */
 int pipefd[2]; /* file descriptor */
 
 /* buffer reset */
 memset(input, 0, BUFSIZ * sizeof(char));
 memset(RC, 0, BUFSIZ * sizeof(char));

 int counter = 0;
 int should_run = 1; /* flag to determine when to exit program */
 while (should_run) {
    printf("osh> ");
    fflush(stdout);
    fgets(input, BUFSIZ, stdin);
    input[strlen(input) - 1] = '\0'; /* "\n" is set to null */
    if(strncmp(input, "exit()", 6) == 0)/* exit the command line*/
        return 0;
    if(strncmp(input, "!!", 2)) /*history check */
        strcpy(RC, input);
    bool daemon = true;
    char* wait_offset = strstr(input, "&");
    if(wait_offset != NULL) {
        *wait_offset = ' '; 
        daemon =false;
    }
    pid_t process = fork();
    if (process < 0) { /*failed to make child process */
        fprintf(stderr, "creation stopped.\n");
        return -1;
    }
    if (process != 0) { /*parent process */
        if(daemon) {
            wait(NULL);
            wait(NULL);
        }
    }
    else{
        char* c_args[BUFSIZ];
        memset(c_args, 0, BUFSIZ * sizeof(char));

        int history_value = 0;
        /* ensures that history reads from RC */
        if(!strncmp(input, "!!", 2)){
            history_value = 1;
        }
        /* ternary statement to use correct buffer */
        char** change = parse( (history_value ? RC : input), c_args);
        if(history_value && RC[0] == '\0') {
            printf("No command used before.\n");
            exit(0);
        }
        if(!strncmp(change[0], "o", 1)){/* redirect output */
            printf("output saved to ./%s\n", change[1]);
            int fd = open(change[1], O_TRUNC | O_CREAT | O_RDWR);
            dup2(fd, STDOUT_FILENO); /* redirect stdout to file descriptor */ 
        }
        else if(!strncmp(change[0], "i", 1)){/* redirect input */
            printf("reading file; ./%s\n", change[1]);
            int fd = open(change[1], O_RDONLY);// read-only
            memset(input, 0, BUFSIZ * sizeof(char));
            read(fd, input,  BUFSIZ * sizeof(char));
            memset(c_args, 0, BUFSIZ * sizeof(char));
            input[strlen(input) - 1]  = '\0';
            parse(input, c_args);
        }
        else if(!strncmp(change[0], "p", 1)){/* found a pipe */
            pid_t pidc;/* hierachical child */
            int pipe_rhs_offset = c_args_offset(c_args);
            c_args[pipe_rhs_offset] = "\0";
            int e = pipe(pipefd);
            if(e < 0){
                fprintf(stderr, "pipe creation failed...\n");
                return 1;
            }
            char* lhs[BUFSIZ], *rhs[BUFSIZ];
            /* set buffers to 0 */
            memset(lhs, 0, BUFSIZ*sizeof(char));
            memset(rhs, 0, BUFSIZ*sizeof(char));

            for(int x = 0; x < pipe_rhs_offset; ++x){
                lhs[x] = c_args[x];
                }
            for(int x = 0; x < BUFSIZ; ++x){
                int idx = x + pipe_rhs_offset + 1;
                if(c_args[idx] == 0) break;
                rhs[x] = c_args[idx];
                }
                
                pidc = fork();/* create child to handle pipe's rhs */
                if(pidc < 0){
                    fprintf(stderr, "fork failed.\n");
                    return 1;
                }
                if(pidc != 0){/* parent process */ 
                    dup2(pipefd[1], STDOUT_FILENO);/* duplicate stdout to write end of file descriptor */
                    close(pipefd[1]);/* close write end of pipe */
                    execvp(lhs[0], lhs);
                    exit(0); 
                }
                else{/* child process */
                    dup2(pipefd[0], STDIN_FILENO);/* copy read end of pipe to stdin */
                    close(pipefd[0]);/* close read end of pipe */
                    execvp(rhs[0], rhs);/* execute command in child */
                    exit(0); 
                }
                wait(NULL);
            }
            execvp(c_args[0], c_args);
            exit(0);  
        
    
    }

 /**
 * After reading user input, the steps are:
 * (1) fork a child process using fork()
 * (2) the child process will invoke execvp()
 * (3) parent will invoke wait() unless command included &
 */
 }
 return 0;
} 

//=======================================================================
// UNIX SHELL OUTPUTS
//=======================================================================
// osh> ls -l
// total 28
// -rwxrwxr-x 1 osc osc 13992 Mar 28 16:45 out
// -rw-rw-r-- 1 osc osc   388 Mar 28 15:58 README.md
// -rw-rw-r-- 1 osc osc  5711 Mar 28 16:07 Unix_Shell.c
// osh> !!
// total 28
// -rwxrwxr-x 1 osc osc 13992 Mar 28 16:45 out
// -rw-rw-r-- 1 osc osc   388 Mar 28 15:58 README.md
// -rw-rw-r-- 1 osc osc  5711 Mar 28 16:07 Unix_Shell.c
// osh> ls -l | README.md
// osh> 
// osh> !!  
// No command used before.
// osh> ls -l | less README.md
// osh> < README.md
// osh> > README.md
// osh> exit()
//=======================================================================
//END OF OUTPUTS
//=======================================================================