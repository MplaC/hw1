#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>

#define FALSE 0
#define TRUE 1
#define INPUT_STRING_SIZE 80

#include "io.h"
#include "parse.h"
#include "process.h"
#include "shell.h"

int cmd_quit(tok_t arg[]) {
  printf("Bye\n");
  exit(0);
  return (1);
}

// Part 1 - supporting cd
int cmd_cd(tok_t arg[]) {
	if (!arg) {
		//Do nothing
	} else {
		chdir(arg[0]);
	}
	return (1);
}

int cmd_help(tok_t arg[]);

/* Command Lookup table */
typedef int cmd_fun_t (tok_t args[]); /* cmd functions take token array and return int */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_cd, "cd", "change directories"}, 		// Part 1 - supporting cd
  {cmd_quit, "quit", "quit the command shell"},
};

int cmd_help(tok_t arg[]) {
  int i;
  for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
    printf("%s - %s\n",cmd_table[i].cmd, cmd_table[i].doc);
  }
  return (1);
}

int lookup(char cmd[]) {
  int i;
  for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0)) return (i);
  }
  return (-1);
}

void init_shell()
{
  /* Check if we are running interactively */
  shell_terminal = STDIN_FILENO;

  /** Note that we cannot take control of the terminal if the shell
      is not interactive */
  shell_is_interactive = isatty(shell_terminal);

  if(shell_is_interactive){

    /* force into foreground */
    while(tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp()))
      kill( - shell_pgid, SIGTTIN);

    shell_pgid = getpid();
    /* Put shell in its own process group */
    if(setpgid(shell_pgid, shell_pgid) < 0){
      perror("Couldn't put the shell in its own process group");
      exit(1);
    }

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);
    tcgetattr(shell_terminal, &shell_tmodes);
  }
  /** YOUR CODE HERE */
}

/**
 * Add a process to our process list
 */
void add_process(process* p)
{
  /** YOUR CODE HERE */
}

/**
 * Creates a process given the inputString from stdin
 */
process* create_process(char* inputString)
{
  /** YOUR CODE HERE */
  return NULL;
}


// Part 1 - Add current working directory to path
#define PROMPTLENGTH 1024
void prompt(int lineNum) {
  char pbuf[PROMPTLENGTH];
  fprintf(stdout,"%d %s: ", lineNum, getcwd(pbuf, PROMPTLENGTH));
}

// Part 2 - Support other commands
void exec_fork(char *cmd, char *argv[]) {
	pid_t cpid = fork();
	if (cpid == 0) {
		execv(cmd, argv);
		_exit(1);
	} else if (cpid > 0) {
		int status;
		while (-1 == waitpid(cpid, &status, 0)) {
			if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
				fprintf(stdout, "Child process id: %d, failed\n", cpid);
				exit(1);
			}
		}
	} else {
		perror("fork failed");
		_exit(3);
	}
}

// Part 2 & 3 - Count tokens
int toks_count(char *t[]) {
	int tcount = 0;
	for (int i = 0; i < MAXTOKS; i++) {
		if (t[i] == NULL) { break;}
		tcount = i;
	}
	return (++tcount);
}

int shell (int argc, char *argv[]) {
  char *s = malloc(INPUT_STRING_SIZE+1);			/* user input string */
  tok_t *t;			/* tokens parsed from input */
  int lineNum = 0;
  int fundex = -1;
  pid_t pid = getpid();		/* get current processes PID */
  pid_t ppid = getppid();	/* get parents PID */

  //Part 3 - Search $Path for command
  char *path = getenv("PATH");
  tok_t *tpath = getToks(path);


  //  pid_t cpid, tcpid, cpgid;

  init_shell();

  printf("%s running as PID %d under %d\n",argv[0],pid,ppid);

  lineNum=0;
  prompt(++lineNum); // Part 1 - Add current working directory to path (Startup)

  while ((s = freadln(stdin))){
    t = getToks(s); /* break the line into tokens */
    fundex = lookup(t[0]); /* Is first token a shell literal */
    if(fundex >= 0) cmd_table[fundex].fun(&t[1]);
    else {

    	// Part 2 - ignore if no command entered
    	if (toks_count(t) > 0) {

    		// Part 3 - Check 1st in current folder
    		if ( (access(t[0], F_OK)) == -1) {
            	//Part 3 - Search $Path for command
            	int pcount = toks_count(tpath);
            	char cmd[1024] = "";
            	int pathfound = -1;

            	// Part 3 - Search PATH
            	for (int i = 0; i < pcount; i++) {
            		char tempcmd[1024] = "";

            		//Build path for access check
            		strcat(tempcmd, tpath[i]);
            		strcat(tempcmd, "/");
            		strcat(tempcmd, t[0]);

            		//Check for access at the path
            		if ( (access(tempcmd, F_OK)) != -1) {
            			strcat(cmd, tempcmd);
            			pathfound = 1;
            			break;
            		}
            	}

            	// Part 3 - if command found, update command PATH, else entered command will run
            	if (pathfound > 0) {
    				t[0] = cmd;
            	}
    		}

			//Part 2 - Start fork
			exec_fork(t[0], t);
    	}
    }
    prompt(++lineNum); // Part 1 - Add current working directory to path
  }
  return (0);
}
