#define _POSIX_C_SOURCE         200112L
#define COMMAND_LINE_SIZE       1024
#define ARGS_SIZE               64
#define COMPUTER                "MINI_SHELL"
#define PROMPT                  ROJO_T "%s@%s" RESET_COLOR ":" VERDE_T "%s" RESET_COLOR "%c ", getenv("USER"), COMPUTER, getenv("PWD"),'$'
#define N_JOBS                  64
#define STOPPED					'D'
#define FINISHED				'F'
#define RUNNING					'E'
//#define USE_READLINE

// Definitions of color codes.
#define RESET_COLOR             "\x1b[0m"
#define ROJO_T                  "\x1b[91m"
#define VERDE_T                 "\x1b[92m"
#define AMARILLO_T              "\x1b[93m"
#define AZUL_T                  "\x1b[94m"
#define MAGENTA_T               "\x1b[95m"
#define CYAN_T                  "\x1b[96m"
#define BLANCO_T                "\x1b[97m"

// Used libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

char* read_line(char *line);
int execute_line(char *line);
int parse_args(char **args, char *line);
int check_internal(char **args);
int internal_cd(char **args);
int internal_export(char **args);
int internal_jobs();
int internal_source(char **args);
void reaper(int signum);
void ctrlc(int signum);
int jobs_list_add(pid_t pid, char status, char *command_line);
int jobs_list_find(pid_t pid);
int jobs_list_remove(int pos);
int is_background(char **args);
void ctrlz(int signum);
int is_output_redirection (char **args);
int internal_fg(char **args);
int internal_bg(char **args);
char* get_prompt();

struct info_process {
	pid_t pid;
	char status; // 'E'jecutandose,'D'etenido,'F'inalizado
	char command_line[COMMAND_LINE_SIZE];
};

