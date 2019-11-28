#define _POSIX_C_SOURCE         200112L
#define COMMAND_LINE_SIZE       1024
#define ARGS_SIZE               64
#define COMPUTER                "MINI_SHELL"
#define PROMPT                  ROJO_T "%s@%s" RESET_COLOR ":" VERDE_T "%s" RESET_COLOR "%c ", getenv("USER"), COMPUTER, getenv("PWD"),'$'
#define N_JOBS                  64

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

char* read_line();
int execute_line();
int parse_args();
int check_internal();
int internal_cd();
int internal_export();
int internal_jobs();
int internal_source();
void reaper();
void ctrlc();
int jobs_list_add();
int jobs_list_find();
int jobs_list_remove();
int is_background();


struct info_process {
	pid_t pid;
	char status; // 'E'jecutandose,'D'etenido,'F'inalizado
	char command_line[COMMAND_LINE_SIZE];
};

