#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <sys/time.h>
#include <pthread.h>
#include "grass.h"
#include "functions.h"


/**
 * System calls
**/

void ping_command(int sockfd, char *line){
    char **args;
    args = split_line(line, " \t\r\n\a");
    char ping_command[134];
    strcpy(ping_command, "ping ");
    strcat(ping_command, args[1]);
    strcat(ping_command, " -c 1");
    FILE *in = NULL;
    in = popen(ping_command, "r");
    char buffer[MAX];
    bzero(buffer, MAX);
    int block_size = -1;
    char first_char = getc(in);
    ungetc(first_char, in);
    if (first_char == EOF) {
        char ping_error[MAX];
        strcpy(ping_error, "ping: ");
        strcat(ping_error, args[1]);
        strcat(ping_error, ": Name or service not known \n");
        send(sockfd, ping_error, MAX, 0);
    } else {
        while ((block_size = fread(buffer, sizeof(char), MAX, in)) > 0) {
            if (send(sockfd, buffer, block_size, 0) < 0) {
                printf("Failed to send command output: %s \n", ping_command);
            }
            bzero(buffer, MAX);
        }
    }
    free(args);
    args = NULL;
}


void date_command(int authenticated, int sockfd){
    if(authenticated){
        FILE *in = NULL;
        in = popen("date", "r");

        char buffer[MAX];
        bzero(buffer, MAX);
        int block_size;
        while ((block_size = fread(buffer, sizeof(char), MAX, in)) > 0) {
            if (send(sockfd, buffer, block_size, 0) < 0) {
                printf("Failed to send command output \n");
            }
            bzero(buffer, MAX);
        }
    } else {
        write_to_socket(sockfd, "Error: access denied \n");
    }
}


/**
 * File management commands
**/

//Prints the current directory of a given user
void pwd_command(int authenticated, int sockfd, struct Archi *archi){
    if (authenticated) {
        char *temp = (char *) malloc(sizeof(char) * (strlen(archi->current->path) + 1));
        strcpy(temp, archi->current->path);
        strcat(temp, "\n");
        write_to_socket(sockfd, temp);
        free(temp);
        temp = NULL;
    } else {
        write_to_socket(archi->sockfd, "Error: access denied \n");
    }
}

//Prints the files of a given user in her current directory
void ls_command(int authenticated,  struct Archi *archi){
    if(authenticated){

        int keep_count = 0;
        for (int i = 0; i < MAX_DIR; i++){
            if (archi->current->next[i]) {
                keep_count++;
            }
        }
        for (int i = 0; i < MAX_FILES; i++) {
            if (archi->current->files[i]) {
                keep_count++;
            }
        }

        char* buff = (char*) malloc((keep_count+1)*128*sizeof(char));
        bzero(buff, keep_count+1);

        char **all_infos = (char **) malloc(keep_count * sizeof(char *));
        for (int i = 0 ; i < keep_count; i++){
            all_infos[i] = (char *) malloc(128 * sizeof(char));
            bzero(all_infos[i], 128);
        }

        //Get all subdirectories...
        keep_count = 0;
        for (int i = 0; i < MAX_DIR; i++){
            if (archi->current->next[i]) {
                sprintf(all_infos[keep_count],"%s\n", archi->current->next[i]->directory);
                keep_count++;
            }
        }
        //... And all files in the current directory
        for (int i = 0; i < MAX_FILES; i++) {
            if (archi->current->files[i]) {
                sprintf(all_infos[keep_count], "%s\n", archi->current->files[i]->filename);
                keep_count++;
            }
        }
        sort(all_infos, keep_count);

        //Save sorted array in a buffer
        sprintf(buff, "total %d \n", keep_count);
        for (int i = 0 ; i < keep_count ; i++){
            strncat(buff, all_infos[i], strlen(all_infos[i]) + 1);
        }
        //Free the memory
        for (int i = 0; i < keep_count; i++){
            free(all_infos[i]);
            all_infos[i] = NULL;
        }
        free(all_infos);
        all_infos = NULL;
        if (buff[0] == '\0'){
            //The current directory is empty
            write_to_socket(archi->sockfd, "");
        }else {
            write_to_socket(archi->sockfd, buff);
        free(buff);
        buff = NULL;
        }
    } else {
        write_to_socket(archi->sockfd, "Error: access denied \n");
    }
}

//Changes the current directory of a given user
void cd_command(int authenticated, char *line, struct Archi *archi){
    char **args;
    args = split_line(line, " \t\r\n\a");
    if (authenticated) {
        struct Dir *copycurrent = archi->current;
        //Split path into directories
        char** path;
        path = split_line(args[1],"/");
        int arg = 0;
        int error = 0;
        //While there is a subdirectory to go to
        while(path[arg]){
            if (strncmp(path[arg], "..", 2) == 0) {
                if (archi->current->prev) {
                    archi->current = archi->current->prev;
                }
                else {
                    //Send error message
                    char* buff = (char*) malloc(1025*sizeof(char));
                    strcpy(buff, "Error: access denied \n");
                    write_to_socket(archi->sockfd, buff);
                    free(buff);
                    buff = NULL;
                    error = 1;
                    break;
                }
            }
            else {
                int i = 0;
                for (i = 0; i < MAX_DIR; i++){
                    //Check if match with all subdirectories
                    if (archi->current->next[i]) {
                        if (strncmp(archi->current->next[i]->directory, path[arg], strlen(archi->current->next[i]->directory)) == 0) {
                            archi->current = archi->current->next[i];
                            break;
                        }
                    }
                }
                if (i == MAX_DIR){
                    //Send error message
                    char* buff = (char*) malloc(1025*sizeof(char));
                    strcpy(buff, "Error: access denied \n");
                    write_to_socket(archi->sockfd, buff);
                    free(buff);
                    buff = NULL;
                    error = 1;
                    break;
                }
            }
            arg++;
        }
        free(args);
        args = NULL;
        free(path);
        path = NULL;
        if (error != 1){
            //Empty message if no error
            write_to_socket(archi->sockfd, "");
        }
        else {
            //Restore old pointer, come back to last valid current directory
            archi->current = copycurrent;
        }
    }
    else {
        write_to_socket(archi->sockfd, "Error: access denied \n");
    }

}

//Create a new directory
void mkdir_command(int authenticated, char *line, struct Archi *archi){

    char **args;
    args = split_line(line, " \t\r\n\a");
    if (authenticated){
        int exist_already = 0;
        for (int i = 0; i < MAX_DIR; i++){
            //If match, break and do nothing
            if (archi->current->next[i]) {
                if (strncmp(archi->current->next[i]->directory, args[1], strlen(args[1])) == 0){
                    exist_already = 1;
                    break;
                }
            }
        }
        //Else
        if (exist_already == 0){
            //Find the first NULL pointer and allocate it
            for (int i = 0; i < MAX_DIR; i++){
                if (!(archi->current->next[i])) {
                    struct Dir * newdir = (struct Dir *) malloc(sizeof(struct Dir));

                    char* temp = (char *) malloc(128*sizeof(char));
                    strcpy(temp, archi->current->path);
                    strcat(temp, "/");
                    strcat(temp, args[1]);
                    newdir->path = temp;
                    free(temp);
                    temp = NULL;
                    char* temp2 = (char *) malloc(128*sizeof(char));
                    strcpy(temp2, args[1]);
                    free(temp2);
                    temp2 = NULL;
                    newdir->directory = temp2;
                    newdir->prev = archi->current;
                    newdir->sockfd = archi->sockfd;
                    archi->current->next[i] = newdir;
                    break;
                }
            }
        }
    }
    else {
        write_to_socket(archi->sockfd, "Error: access denied \n");
    }
    free(args);
    args = NULL;
}

//Removes an existing directory
void rm_command(int authenticated, char *line, struct Archi *archi){
    char **args;
    args = split_line(line, " \t\r\n\a");
    if(authenticated){
        //Loop through subdirectories ...
        for (int i = 0; i < MAX_DIR; i++){
            if (archi->current->next[i]) {
                if (strncmp(archi->current->next[i]->directory, args[1], strlen(archi->current->next[i]->directory)) == 0){
                    free_directory(archi->current->next[i]);
                    archi->current->next[i] = NULL;
                    //break;
                }
            }
        }
        //... And files
        for (int i = 0; i < MAX_FILES; i++) {
            if (archi->current->files[i]) {
                if (strncmp(archi->current->files[i]->filename, args[1], strlen(archi->current->files[i]->filename)) == 0) {
                    free_file(archi->current->files[i]);
                    archi->current->files[i] = NULL;
                    break;
                }
            }
        }
    } else {
        write_to_socket(archi->sockfd, "Error: access denied \n");
    }
    free(args);
    args = NULL;
}

/**
 * File transfer commands
**/

//Gets a file from the server
void get_command(int authenticated, int sockfd, char *line, struct Archi *archi){
    char **args;
    args = split_line(line, " \t\r\n\a");
    if (authenticated) {
        int index_file = -1;
        for (int i = 0; i < MAX_FILES; i++) {
            if(archi->current->files[i]){
                if (strcmp(archi->current->files[i]->filename, args[1]) == 0) {
                    index_file = i;
                    break;
                }
            }
        }
        if (index_file == -1) {
            printf("Error: file %s does not exists \n", args[1]);
            write_to_socket(sockfd, "Error: file does not exists \n");
        } else {
            t_send_file_server(sockfd, args[1], archi);
        }

    } else {
        write_to_socket(archi->sockfd, "Error: access denied \n");
    }
    free(args);
    args = NULL;

}

//Puts a file on the server given a correct size
void put_command(int authenticated, int sockfd, char *line, struct Archi *archi){
    char **args;
    args = split_line(line, " \t\r\n\a");

    if (authenticated) {
        srand(time(NULL));
        int random_port_number = rand() % 10000;
        char port_buffer[12];
        sprintf(port_buffer, "%d", random_port_number);

        char file_specs[64];
        strcpy(file_specs, "put port: ");
        strcat(file_specs, port_buffer);
        printf("file specs : %s \n", file_specs);

        //Send the port number to client
        write_to_socket(sockfd, port_buffer);

        char *file_name = args[1];
        int file_size = atoi(args[2]);

        struct receive_args_server *rec_args = (struct receive_args_server *) malloc(sizeof(struct receive_args_server));

        rec_args->port_number = random_port_number;
        rec_args->file_size = file_size;
        rec_args->file_name = malloc(1024);
        strcpy(rec_args->file_name, file_name);
        rec_args->archi = archi;

        pthread_t tid;
        if (pthread_create(&tid, NULL, receive_thread_server, (void *) rec_args) != 0) {
            printf("Failed to create thread\n");
        }
    } else {
        write_to_socket(archi->sockfd, "Error: access denied \n");
    }
}

/**
 * Grep commands
**/

//Grep from scratch, v1 (only returns filepath for a given word, no pattern, no wildcard)
void grep_command(struct Dir* current, char* buff, char* pattern){
    for (int i = 0; i < MAX_DIR; i++){
        if (current->next[i]) {
    printf("Recursive loop, search in %s\n", current->next[i]->path);
            grep_command(current->next[i], buff, pattern);
        }
    }
    for (int i = 0; i < MAX_FILES; i++){
        if (current->files[i]) {
    if (find_pattern(current->files[i], pattern) == 0){
        printf("found one %s\n", current->files[i]->filename);
                strcat(buff, current->files[i]->filename);
                strcat(buff,"\n");
        printf("check %s\n", buff);
    }
        }
    }

}

//Grep with syscall
void unconventional_grep_command(int authenticated, int sockfd, struct Dir* current, char* pattern){
    if (authenticated) {
        //Create temporary base directory and  write all files from current directory
        system("mkdir base");
        write_files(current);

        //Exec grep command in ./base/
        char* command = (char*)malloc(MAX*sizeof(char));
        strcpy(command,  "grep -l ");
        strcat(command, pattern);
        strcat(command, " ./base/*");
        FILE *in;
        in = popen(command, "r");
        char buffer[MAX];
        bzero(buffer, MAX);
        int block_size = -1;
        char first_char = getc(in);
        ungetc(first_char, in);
        if (first_char == EOF) {
            char error[MAX];
            strcpy(error, "grep: ");
            strcat(error, pattern);
            strcat(error, ": Pattern not found \n");
            send(sockfd, error, MAX, 0);
        } else {
            while ((block_size = fread(buffer, sizeof(char), MAX, in)) > 0) {
                if (send(sockfd, buffer, block_size, 0) < 0) {
                    printf("Failed to send command output: %s \n", command);
                }
                bzero(buffer, MAX);
            }
        }
        //Remove temporary base directory
        system("rm -R base");
        free(command);
        command = NULL;
    } else {
        write_to_socket(sockfd, "Error: please authenticate to execute this command \n");
    }


}
