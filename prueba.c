#define _POSIX_C_SOURCE         200112L
#define COMMAND_LINE_SIZE       1024
#define ARGS_SIZE               64
#define COMPUTER                "MINI_SHELL"
#define PROMPT                  ROJO_T "%s@%s" RESET_COLOR ":" VERDE_T "%s" RESET_COLOR "%c ", getenv("USER"), COMPUTER, getenv("PWD"),'$'
#define N_JOBS                  64
#define STOPPED					'D'
#define FINISHED				'F'
#define RUNNING					'E'
#define USE_READLINE

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

char* get_prompt();

int main(int argc, char *argv[]) {
    printf("%s%s", get_prompt() ,"\n");
}

char* get_prompt(){
    char s[COMMAND_LINE_SIZE];
    strcpy(s, "\x1b[91m");
    strcat(s, getenv("USER"));
    strcat(s, "@MINI_SHELL\x1b[0m:\x1b[92m");
    strcat(s, getenv("PWD"));
    return strcat(s, "\x1b[0m$");
}