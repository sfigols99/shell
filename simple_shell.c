/**
 * Simple shell interface program.
 *
 * Operating System Concepts - Tenth Edition
 * Copyright John Wiley & Sons - 2018
 *
 * Modified by Ilker Demirkol
 */

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_LINE 80 /* 80 chars per line, per command */
#define BUILT_IN_COMMANDS 5

	    /**
         	 * Steps are: 
		 * 1) Read user input
		 * 2) If it is a built-in command, call the relevant function
         	 * 3) Else: fork a child process
         	 *  3.11) the child process will invoke execvp() to execute the command provided by the user
		 *  3.12) the parent will wait till the command (child) ends, then displays "Command run successfully.", if the exit value returned is positive, else: some negative message
         */

int should_run = 1;

/* AUXILIAR FUNCTIONS */
static int get_last_arg(char **args) {
	// Auxiliar funcion that returns the last argument
	int i;
	for (i = 0; args[i] != NULL; i++);
	return --i;
}

static char *get_username() {
	// Returns the username logined in
	char *usrname = getlogin();
	if (!usrname) {
		perror("getlogin() error");
		return "";
	} else {
		return usrname;
	}
}
/*-------------------------------------*/

/* -------- BUILT IN FUNCTIONS AND DEFINITIONS ------------ */

void get_actual_path(char **args);
void change_dir(char **args);
void change_mode(char **args);
void change_owner(char **args);
void surt(char **args);

// definition of built in commands
char *built_ins[] = {
	"ic",  // Print current dir
	"cd",  // Change directory
	"cm",  // Change the file's permissions into given mode
	"co",  // Change owner giving user and file path
	"surt" // Exits Shell
}; 

typedef void (*function_t)(char**);

function_t built_in_functions[BUILT_IN_COMMANDS] = {
	&get_actual_path, 
	&change_dir, 
	&change_mode, 
	&change_owner, 
	&surt
};

void get_actual_path(char **args) {
	// Prints the current path
	char cwd[MAX_LINE];

	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		printf("%s",cwd);
	} else {
		perror("getcwd() error");
	}
}

void change_dir(char **args) {
	// Change directory
	char path[50];
	if (args[1] == NULL) {
		strcat(path, "/home/");
		strcat(path, get_username());
		chdir(path);
	}
	else if (chdir(args[1]) != 0) {
		perror("chdir() error");
	}
}

void change_mode(char **args) {
	mode_t mode = 384; // If no mode bits are give, rw for user by default. 384d = 600o
	if (args[2] != NULL) {
		mode = strtol(args[2], 0, 8);  // atoi doesn't transform to octal	
		//printf("%o", mode);
	}
	if (chmod(args[1], mode) < 0) {  
		perror("chmod() error");
	}

}

void change_owner(char **args) {
	uid_t user_id = -1;
	struct passwd *user_info;

	if(args[1] != NULL && args[2] != NULL) {
		user_info=getpwnam(args[2]);
		if (user_info==NULL) {
			printf("User not found. ");
		}
		
		else {
			user_id = user_info->pw_uid;
		}
	}
	if (chown(args[1], user_id, -1) < 0) { 
		// The gid is not modified as in the statement there is nothing said about it.
		perror("chown() error");
	}
}

void surt(char **args) {
	// Exits shell
	should_run = 0;
}

/* ------------------------------------- */

static int execute_command(char ** const args) {
	// First of all we look at the built in commands definition
	// and then execute the function in the function structure.

	// Then we run execvp if command is not built in.
	// perror is used in case that the command is not found (chill ends when this happens)

	/* ----------- BUILT IN SECTION ------------- */
	for (int i = 0; i < BUILT_IN_COMMANDS; i++) {
		
		if (strcmp(args[0], built_ins[i]) == 0) { // args[0] == any_builtin
			built_in_functions[i](args);
			printf("\n");
			return 0;
		}
	}

	/* ---------- NOT BUILT IN SECTION ----------- */
	int pid = fork();

	if (pid<0) {
		printf("Fork Failed.\n");
	} 
		
	else if (pid == 0) {
		if (strcmp(args[get_last_arg(args)], "&") == 0) {
			args[get_last_arg(args)] = NULL; 
		}
		
		if (execvp(args[0], args) < 0) {
			perror("Error");
			exit(1);
		}
			
	} 
	
	else {
		// In case there is not background execution
		if (strcmp(args[get_last_arg(args)], "&") != 0) {
			wait(NULL);
		}
	}
	return 0;
}

int main(void)
{
	char *token;
	char aux_args[MAX_LINE];
	char *args[MAX_LINE];	/* command line (of 80) has max of 40 arguments */
	int i;
	
	printf("Sergi's Bash\n");		

    while (should_run){
		/* --------- PROMPT ---------- */
		printf("%s@", get_username());
		get_actual_path(args);
		printf("> %c ", 37);

		/* ------- GET INPUT -----------*/
		fgets(aux_args, MAX_LINE, stdin);
		if (aux_args[0] != '\n') {
			
			if (aux_args[strlen(aux_args)-1] == '\n') { 
				aux_args[strlen(aux_args)-1] = '\0';
			}

			if (aux_args[strlen(aux_args)-1] == '&') {
				aux_args[strlen(aux_args)-1] = ' ';
				aux_args[strlen(aux_args)] = '&';
				aux_args[strlen(aux_args)] = '\0';
			}
			
			token = strtok(aux_args," ");
			i = 0;
		
			while (token != NULL) {
				args[i] = token;
				token = strtok(NULL, " ");
				i++;
			}

			args[i] = NULL;
		
		/* -------- EXECUTE COMMAND ----------- */
			execute_command(args);
			
      		fflush(stdout);
		}
    }    
	return 0;
}
