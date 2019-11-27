#include "mini_shell.h"

static struct info_process jobs_list[N_JOBS];
int status;
char *arg;

int main(int argc, char *argv[]) {
	jobs_list[0].pid=0;     // As we don't have any son on foreground.
    arg = argv[0];
	signal(SIGINT,ctrlc);
	signal(SIGCHLD,reaper);
    char line[COMMAND_LINE_SIZE];
    while (read_line(line)){
        execute_line(line);
    }
}

// Prints the prompt and then reads the command introduced by the user.
char *read_line(char *line) {
    printf(PROMPT);
    fflush(stdout);   
    char *ptr = fgets(line, COMMAND_LINE_SIZE, stdin);

    if (!ptr) {  //ptr==NULL
        printf("\r");
        if (feof(stdin)) { //feof(stdin!=0)
            exit(0);
        }
        else {
            ptr = line; // si no al pulsar inicialmente CTRL+C sale fuera del shell
            ptr[0] = 0; // Si se omite esta línea aparece error ejecución ": no se encontró la orden"*/
        }
   }

   return ptr;
}

int internal_cd(char **args) {
    if(!args[1]){
        args[2]=NULL;       // We have to do this because if the basic cd is the first command used, args[2] has no assigned value and may not be NULL, as it is not touched during parse_args
    }

    if(args[2]){
        fprintf(stderr, "Error: Demasiados argumentos \n");
        return -1;
    } else if(!args[1]){
        if(chdir(getenv("HOME")) == -1){
            fprintf(stderr, "Error %d: %s \n", errno, strerror(errno));
            return -1;
        }
    } else if(chdir(args[1]) == -1){
        fprintf(stderr, "Error %d: %s \n", errno, strerror(errno));
        return -1;
    }
    setenv("PWD", getcwd(NULL, 0), 1);

    return 0;
}

int internal_export(char **args) {
    if (!args[1]) {
        fprintf(stderr, "Error de sintaxis. Uso: export Nombre=Valor\n");
        return -1;
    }

    char *name = strtok(args[1], "=");
    char *value = strtok(NULL, " \t\n\r");

    if (!value) {
        fprintf(stderr, "Error de sintaxis. Uso: export Nombre=Valor\n");
        return -1;
    }

    setenv(name, value, 1);

    return 0;
}

int internal_source(char **args) {
    if(!args[1]){
        args[2]=NULL;       // We have to do this because if the basic source is the first command used, args[2] has no assigned value and may not be NULL, as it is not touched during parse_args
    }
    
    if(args[2]){
        fprintf(stderr, "Error: Demasiados argumentos \n");
        return -1;
    }
    if(args[1]){
        FILE *fp;
        fp = fopen(args[1], "r");
        if (fp){
            char command[COMMAND_LINE_SIZE];
            while (fgets(command, COMMAND_LINE_SIZE, fp)){
                fflush(fp);
                execute_line(command);
            }
            fclose(fp);
            return 0;
        } else {
            fprintf(stderr, "Error: El fichero no existe \n");
            return -1;
        }
    } else {
        fprintf(stderr, "Error: Debe indicar un fichero a leer \n");
        return -1;
    }
}

int internal_jobs(char **args) {
    printf("[internal_jobs()→Esta función mostrará el PID de los procesos que no estén en foreground] \n");
    return 0;
}



// Parses the line into the different arguments and checks if one of them starts with # and ignores everything that comes afterwards.
int parse_args(char **args, char *line) {
    const char *s = " \t\n\r";
    int tokens = 0;

    args[tokens] = strtok(line, s);


    while (args[tokens] != NULL) {
        tokens++;
    
        args[tokens] = strtok(NULL, s);
        
        if(args[tokens] != NULL && args[tokens][0] == '#'){
            args[tokens] = NULL;
        }
        
        // Joining arguments with double quotes
        if(args[tokens] != NULL && (strchr(args[tokens], '\"'))){   // Once we find simple or double quotes we ignore spaces until another simple or double quotes.
            int i = 0;
            while(args[tokens][i]){      // However, the previous strtok already changed the next space with \0, so we have to change it again.
                i++;
            }
            args[tokens][i] = ' ';
            strtok(NULL, "\"");       // And finally we separate once more with the quotations

            args[tokens]++;
	    }

        // Joining arguments with simple quotes
        if(args[tokens] != NULL && strchr(args[tokens], '\'')){     // Once we find simple or double quotes we ignore spaces until another simple or double quotes.
            int i = 0;
            while(args[tokens][i]){      // However, the previous strtok already changed the next space with \0, so we have to change it again.
                i++;
            }
            args[tokens][i] = ' ';
            strtok(NULL, "\'");       // And finally we separate once more with the quotations

            args[tokens]++;
	    }
    }
    
    return tokens;
}

// Checks the first token of the line and checks if it's one of the internal commands.
int check_internal(char **args) {
    if(*args == NULL) return 0;
    if(!strcmp(*args, "cd")) {
        internal_cd(args);
        return 1;
    } else if(!strcmp(*args,  "export")) {
        internal_export(args);
        return 1;
    } else if(!strcmp(*args,  "jobs")) {
        internal_jobs(args);
        return 1;
    } else if(!strcmp(*args,  "source")) {
        internal_source(args);
        return 1;
    } else if(!strcmp(*args, "exit")){
        exit(0);
    }
    return 0;
}

int execute_line(char *line) {
    char og_line[COMMAND_LINE_SIZE];
    strcpy(og_line, line);
    char *args[ARGS_SIZE];
    parse_args(args, line);

    if (!check_internal(args)) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("No se pudo crear el proceso secundario.\n");
            exit(-1);
        }
        //Son process
        else if (pid == 0) {
            signal(SIGCHLD, SIG_DFL);
		    signal(SIGINT, SIG_IGN);
            printf("[execute_line()→ PID padre: %d]\n", getppid());
            fflush(stdout);
            if (execvp(args[0], args) < 0) {
                fprintf(stderr, "Error: Comando \'%s\' no encontrado\n", *args);
                exit(EXIT_FAILURE);
            }
        }
        //Father process
        else {
            jobs_list[0].pid = pid;
            jobs_list[0].status = 'E';
            strcpy(jobs_list[0].command_line, og_line); 
            printf("%s\n", jobs_list[0].command_line);
            while(jobs_list[0].pid!=0){
                pause();
            }
        }
    }

}

void reaper(int signum){
    signal(SIGCHLD, reaper);
    pid_t pid;
    while ((pid=waitpid(-1, &status, WNOHANG)) > 0) {
        if (pid==jobs_list[0].pid){
            jobs_list[0].pid = 0;
            jobs_list[0].status = 'F';
            jobs_list[0].command_line[0] = '\0';
            if (WIFEXITED(status)) {
                printf("[execute_line()→ Proceso hijo %d finalizado con exit(), estado: %d]\n", pid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("[execute_line()→ Proceso hijo %d finalizado con señal: %d]\n", pid, WTERMSIG(status));
            }
        }
    }
}

void ctrlc(int signum){
    signal(SIGINT, ctrlc);
    printf("\nPID DESDE CTRLC: %d\n", jobs_list[0].pid);
    fflush(stdout);
    if(jobs_list[0].pid>0){
        if(strcmp(jobs_list[0].command_line, arg)-10!=0){      // (jobs_list[0].command_line == arg)
            fprintf(stderr, "Proceso a terminar: %s \nDiferencia con %s: %d\n", jobs_list[0].command_line, arg, strcmp(jobs_list[0].command_line, arg));
            kill(jobs_list[0].pid, SIGTERM);
        } else {
            fprintf(stderr, "\nSeñal no enviada porque el proceso a terminar es: %s\n", jobs_list[0].command_line);
        }
    } else {
        fprintf(stderr, "\nSeñal SIGTERM no enviada debido a que no hay proceso en foreground\n");
    }
}
