#include "mini_shell.h"

static struct info_process jobs_list[N_JOBS];
int status;
char *arg;
int n_pids;

int main(int argc, char *argv[]) {
	jobs_list[0].pid=0;     // As we don't have any son on foreground.
    arg = argv[0];
    n_pids = 0;
	signal(SIGINT,ctrlc);
	signal(SIGCHLD,reaper);
    signal(SIGTSTP,ctrlz);

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
    if(ptr){
        free(ptr);
        ptr = (char *)NULL;
    }
    ptr = readline("");
    strcpy(line,ptr);
    
    if (ptr && *ptr){
        add_history(ptr);
    }

   return ptr;
}

int internal_cd(char **args) {
    if(!args[1]){
        args[2]=NULL;       // We have to do this because if the basic cd is the first command used, args[2] has no assigned value and may not be NULL, as it is not touched during parse_args
    }
    
    if(args[2]){
        fprintf(stderr, "Error: Demasiados argumentos: %s \n", args[2]);
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
        args[2]=NULL;       // We have to do this because if source is the first command used, args[2] has no assigned value and may not be NULL, as it is not touched during parse_args
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
    for(int i = 1; i <= n_pids; i++){
        printf("[%d] %d\t%c\t%s", i, jobs_list[i].pid, jobs_list[i].status, jobs_list[i].command_line);
    }
    return EXIT_SUCCESS;
}

int internal_fg(char **args){
    if(!args[1]){
        args[2]=NULL;       // We have to do this because if fg is the first command used, args[2] has no assigned value and may not be NULL, as it is not touched during parse_args
    }

    if(args[2]){
        fprintf(stderr, "Error: Demasiados argumentos \n");
        return -1;
    }
    if(args[1]){
        int pos = atoi(args[1]);
        printf("%d\n", pos);
        if (pos>n_pids || pos==0){
            fprintf(stderr, "Error: No existe este trabajo \n");
            return -1;
        } else {
            if (jobs_list[pos].status == STOPPED){
                kill(jobs_list[pos].pid, SIGCONT);
                jobs_list[pos].status = RUNNING;
                char *ampersand = strchr(jobs_list[0].command_line, '&');
                ampersand = '\0';
                jobs_list[0].pid = jobs_list[pos].pid;
                jobs_list[0].status = jobs_list[pos].status;
                strcpy(jobs_list[0].command_line, jobs_list[pos].command_line);
                jobs_list_remove(pos);
                printf("\n%s\n",jobs_list[pos].command_line);
                while (jobs_list[0].pid!=0){
                    pause();
                }
            }
        }
    } else {
        fprintf(stderr, "\nIndique el numero del proceso a eliminar -> fg [Índice proceso]\n");
        return EXIT_FAILURE;
    }
}

int internal_bg(char **args){
    if(!args[1]){
        args[2]=NULL;       // We have to do this because if bg is the first command used, args[2] has no assigned value and may not be NULL, as it is not touched during parse_args
    }

    if(args[2]){
        fprintf(stderr, "Error: Demasiados argumentos \n");
        return -1;
    }
    if(args[1]){
        int pos = atoi(args[1]);
        if (pos>n_pids || pos==0){
            fprintf(stderr, "Error: No existe este trabajo \n");
            return -1;
        } else {
            if (jobs_list[pos].status == RUNNING){
                fprintf(stderr, "Error: El trabajo ya esta en 2º plano \n");
                return -1;
            } else {
                jobs_list[pos].status == RUNNING;
                char *aux = strchr(jobs_list[pos].command_line, '\n');
                *aux = '\0';
                strcat(jobs_list[pos].command_line, " &\n");
                kill(jobs_list[pos].pid, SIGCONT);
                printf("\nProceso %d reactivado\n", jobs_list[pos].pid);
                printf("\nProceso reactivado: [%d]\t%d\t%c\t%s\n", pos, jobs_list[pos].pid, jobs_list[pos].status, jobs_list[pos].command_line);
                return 0;
            }
        }
    } else {
        fprintf(stderr, "\nIndique el numero del proceso a reactivar -> fg [Indice proceso]\n");
        return -1;
    }
}

// Parses the line into the different arguments and checks if one of them starts with # and ignores everything that comes afterwards.
int parse_args(char **args, char *line) {
    #ifdef USE_READLINE
    const char *s = " \t\r";
    #else
    const char *s = " \t\r\n";
    #endif
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

        //Joining arguments with backslash
        char *backslash;
        while(args[tokens] != NULL && (backslash = strchr(args[tokens], '\\'))){
            char *aux = backslash+1;
            *aux=' ';
            while(*aux){
                *backslash = *aux;
                backslash++; aux++;
            }
            strtok(NULL, s);
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
    } else if(!strcmp(*args,  "fg")) {
        internal_fg(args);
        return 1;
    } else if(!strcmp(*args,  "bg")) {
        internal_bg(args);
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
    int is_bkg = is_background(args);

    if (!check_internal(args)) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("No se pudo crear el proceso secundario.\n");
            exit(-1);
        }
        //Son process
        else if (pid == 0) {
            is_output_redirection(args);
            signal(SIGCHLD, SIG_DFL);
		    signal(SIGINT, SIG_IGN);
            //Si la señal esta en background ignora la señal, si no, hace la accion por defecto
            if(is_bkg){
                signal(SIGTSTP, SIG_IGN);
            } else {
                signal(SIGTSTP, SIG_DFL);           
            }
            if (execvp(args[0], args) < 0) {
                fprintf(stderr, "Error: Comando \'%s\' no encontrado\n", *args);
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }
        //Father process
        else {
            if(is_bkg){
                jobs_list_add(pid, RUNNING, og_line);
            } else {
                jobs_list[0].pid = pid;
                jobs_list[0].status = RUNNING;
                strcpy(jobs_list[0].command_line, og_line); 
                while(jobs_list[0].pid!=0){
                    pause();
                }
            }
        }
    }
}

void reaper(int signum){
    signal(SIGCHLD, reaper);
    pid_t pid;
    while ((pid=waitpid(-1, &status, WNOHANG)) > 0) {
        if(pid == jobs_list[0].pid){
            jobs_list[0].pid = 0;
            jobs_list[0].status = 'F';
            jobs_list[0].command_line[0] = '\0';
            if (WIFEXITED(status)) {
                //printf("[execute_line()→ Proceso hijo %d finalizado con exit(), estado: %d]\n", pid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("[execute_line()→ Proceso hijo %d finalizado con señal: %d]\n", pid, WTERMSIG(status));
            }
            pid = 0;
        } else {
            int x = jobs_list_find(pid);
            if (WIFEXITED(status)) {
                //fprintf(stderr, "[execute_line()→ Proceso hijo %d finalizado con exit(), estado: %d]\n", pid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                fprintf(stderr, "[execute_line()→ Proceso en segundo plano %d finalizado con señal: %d]\n", pid, WTERMSIG(status));
            }
            jobs_list_remove(x);
            pid = 0;
        }
    }
}

void ctrlz(int signum){
    signal(SIGTSTP, ctrlz);
    //printf("\nPID DESDE CTRLC: %d\n", jobs_list[0].pid);
    //fflush(stdout);
    if(jobs_list[0].pid>0){
        printf("\n");
        if(strcmp(jobs_list[0].command_line, arg)-10!=0){      // (jobs_list[0].command_line == arg)
            //fprintf(stderr, "Proceso a terminar: %s \nDiferencia con %s: %d\n", jobs_list[0].command_line, arg, strcmp(jobs_list[0].command_line, arg));
            kill(jobs_list[0].pid, SIGTSTP);
            jobs_list[0].status = STOPPED;
            jobs_list_add(jobs_list[0].pid, jobs_list[0].status, jobs_list[0].command_line);

            jobs_list[0].pid = 0;
            jobs_list[0].status = FINISHED;
            jobs_list[0].command_line[0] = '\0';
        } else {
            fprintf(stderr, "\nSeñal SIGTSTP no enviada porque el proceso en foreground es: %s\n", jobs_list[0].command_line);
        }
    } else {
        #ifdef USE_READLINE
           printf("\n" PROMPT);
        #else
           printf("\n");
        #endif
        fprintf(stderr, "\nSeñal SIGTSTP no enviada debido a que no hay proceso en foreground\n");
    }
    fflush(stdout);
}

void ctrlc(int signum){ 
    signal(SIGINT, ctrlc);
    //printf("\nPID DESDE CTRLC: %d\n", jobs_list[0].pid);
    //fflush(stdout);
    if(jobs_list[0].pid>0){
        printf("\n");
        if(strcmp(jobs_list[0].command_line, arg)-10!=0){      // (jobs_list[0].command_line == arg)
            //fprintf(stderr, "Proceso a terminar: %s \nDiferencia con %s: %d\n", jobs_list[0].command_line, arg, strcmp(jobs_list[0].command_line, arg));
            kill(jobs_list[0].pid, SIGTERM);
        } else {
            fprintf(stderr, "\nSeñal SIGTERM no enviada porque el proceso en foreground es: %s\n", jobs_list[0].command_line);
        }
    } else {
        #ifdef USE_READLINE
           printf("\n" PROMPT);
        #else
           printf("\n");
        #endif
        fprintf(stderr, "\nSeñal SIGTERM no enviada debido a que no hay proceso en foreground\n");
    }
}

int is_background(char **args){
    int i = 1;
    int found = 0;
    while(args[i] && !found){
        if(strcmp(args[i],"&") == 0){ // args[i] == '&'
            args[i] = NULL;
            found = 1;
        }
        i++;
    }
    return found;
}

int is_output_redirection(char **args){
    int i = 1;
    int found = 0;
    while(args[i] && !found){
        if(strcmp(args[i],">") == 0){ // args[i] == '>'
            args[i] = NULL;
            found = 1;
        }
        i++;
    }
    if(found){
        int fd = open(args[i], O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR);
        if(fd < 0) return EXIT_FAILURE;
        dup2(fd, 1);
        close(fd);
    }
    return found;
}

int jobs_list_add(pid_t pid, char status, char *command_line){
    if (n_pids < N_JOBS){
        n_pids++;
        jobs_list[n_pids].pid = pid;
        jobs_list[n_pids].status = status;
        strcpy(jobs_list[n_pids].command_line, command_line);
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "Error: jobs_list_add número maximo de trabajos alcanzado");
        return EXIT_FAILURE;
    }
}

int jobs_list_find(pid_t pid){
    for (int i = 0; i<N_JOBS; i++){
        if (jobs_list[i].pid == pid) {
            return i;
        }
    }
}

int  jobs_list_remove(int pos){
    if (n_pids > 0){
        jobs_list[pos].pid = jobs_list[n_pids].pid;
        jobs_list[pos].status = jobs_list[n_pids].status;
        strcpy(jobs_list[pos].command_line, jobs_list[n_pids].command_line);
        
        jobs_list[n_pids].pid = 0;
        jobs_list[n_pids].status = FINISHED;
        jobs_list[n_pids].command_line[0] = '\0';

        n_pids--;
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}
