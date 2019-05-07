//INCLUDE
#include "grass.h"
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include<pthread.h>
#include "functions.h"

void t_send_file_from_client(char* file_name, int file_size, int port_number){

    //Create a new socket and assign to it the previous port
    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;

    //Create the socket
    serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_number);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    //Set all bits of the padding field to 0
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    //Bind the address struct to the socket
    bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if(listen(serverSocket,50)==0){
        //printf("Created new socket. Listening\n");
    }

    pthread_t tid;

    addr_size = sizeof serverStorage;
    newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);

    //Arguments of client_thread_send
    struct send_args *args = (struct send_args *)malloc(sizeof(struct send_args));
    args->file_name = malloc(128);
    strcpy(args->file_name, file_name);
    args->sockfd = newSocket;
    args->file_size = file_size;

    if( pthread_create(&tid, NULL, send_thread_client, (void *)args) != 0){
        printf("Failed to create thread\n");
    }
}

void help ()
{
    puts("GRASS [COMMAND] [ARGUMENTS]");
    puts("  ----------------------------:");
    puts("  no auth needed:");
    puts("  ----------------------------:");
    puts("  help: displays this help.");
    puts("  login <username>: start authentication.");
    puts("  pass <password>: finish authentication.");
    puts("  debug: prints passwords.");
    puts("  ping <host>: test connection to host.");
    puts("  exit: signals end of command session.");
    puts("  ----------------------------:");
    puts("  auth needed:");
    puts("  ----------------------------:");
    puts("  ls: list available files in current working directory.");
    puts("  cd <directory>: change working directory to specified one.");
    puts("  mkdir <directory>: create new directory with specified name in the current working directory.");
    puts("  rm <name>: delete file or directory with specified name in the current working directory.");
    puts("  get <filename>: retrieves specified file from the current working directory.");
    puts("  put <filename> <size>: sends specified file from the current local working directory to the server.");
    puts("  grep <pattern>: searches every file the current working directory and its subdirectory for the requested pattern.");
    puts("                  pattern must follow the Regular Expression rules.");
    puts("  date: returns date.");
    puts("  whoami: returns name of logged in user.");
    puts("  w: returns list of logged in users.");
    puts("  logout: logout user.");

}

void send_and_print(int sockfd, char* copy){
    write_to_socket(sockfd, copy);
    char buffer[1025];
    bzero(buffer, 1025);
    read(sockfd,buffer,1025);
    printf("%s", buffer);

}

void parse_args(int sockfd, char* line){
    char** args;
    char * copy = malloc(strlen(line) + 1);
    strcpy(copy, line);
    args = split_line(line, " \t\r\n\a");

    //check for "\n" lines
    if (*line == 0x0a){
	return;}

    //number of strings in the array
    int number_of_args;
    for (number_of_args = 0; args[number_of_args] != NULL; number_of_args++);

    //printf("you entered %d arguments \n", size);

    if(strncmp(args[0], "ping", 4) == 0){
        if (number_of_args != 2){
            printf("Error: please enter a valid host \n");
        }
        else{
            send_and_print(sockfd, copy);
        }
    }
    else if (strncmp(args[0], "ls", MAX) == 0) {
        send_and_print(sockfd, copy);
    }
    else if (strncmp(args[0], "cd", MAX) == 0) {
        if(number_of_args != 2){
            printf("Error: please enter a valid directory name. \n");
        }
        else{
            send_and_print(sockfd, copy);
        }
    }
    else if (strncmp(args[0], "mkdir", MAX) == 0) {
        if(number_of_args != 2){
            printf("Error: please enter a valid directory name. \n");
        }
        else{
            if (strlen(args[1])>=128){
                printf("Error: the path is too long. \n");
            } else {
                write_to_socket(sockfd, copy);
            }

        }
    }
    else if (strncmp(args[0], "rm", MAX) == 0) {
        if(number_of_args != 2){
            printf("Error: please enter a valid directory name. \n");
        } else {
            write_to_socket(sockfd, copy);
        }
    }
    else if (strncmp(args[0], "get", MAX) == 0) {
        if(number_of_args != 2){
            printf("Error: please enter a valid filename. \n");
        }
        else {
            //Send request
            write_to_socket(sockfd, copy);

            //Read the port and the size from the server
            char buffer[1025];
            bzero(buffer, 1025);
            read(sockfd, buffer, 1024);
            if(strncmp(buffer, "Error", 5) == 0){
                printf("Error: file %s does not exists \n", args[1]);
            } else {
                char** args_transfer;
                args_transfer = split_line(buffer, " \t\r\n\a");

                int port_number = atoi(args_transfer[2]);
                int file_size = atoi(args_transfer[4]);

                //printf("GET file %s size %d port %d \n", args[1], file_size, port_number);
                //Connect to the new thread using our port
                struct receive_args_client *rec_args = (struct receive_args_client *)malloc(sizeof(struct receive_args_client));

                rec_args->file_name = malloc(1024);
                strcpy(rec_args->file_name, args[1]);
                rec_args->port_number = port_number;
                rec_args->file_size = file_size;

                pthread_t tid;
                if( pthread_create(&tid, NULL, receive_thread_client, (void *)rec_args) != 0 ){
                    printf("Failed to create thread\n");
                }
            }
        }
    }
    else if (strncmp(args[0], "put", MAX) == 0){
        if (number_of_args != 3) {
            printf("Error: please enter a valid filename and it's size. \n");
        }
        else{
            char* file_name = args[1];
            int file_size = atoi(args[2]);
            //Check file
            FILE *f;
            f = fopen(file_name, "r");
            if (f == NULL){
                printf("Error: unknown file. Please provide a valid filename.\n");
            }
            else if(strlen(file_name) >= 128){
                printf("Error: path is too long. \n");
            }
            else {
                //Check size
                fseek(f, 0 , SEEK_END);
                int fileSize = ftell(f);
                fseek(f, 0 , SEEK_SET);

                if (fileSize != file_size){
                    printf("Error: bad size. Please provide a valid file size. \n");
                } else {
                    //Sending put command to server
                    write_to_socket(sockfd, copy);

                    //Receive port number
                    char buffer[1025];
                    bzero(buffer, 1025);
                    read(sockfd, buffer, 1024);
                    if(strncmp(buffer, "Error", 5) == 0){
                        printf("Error: please authenticate to execute this command \n");
                    }
                    else {
                        int port_number = atoi(buffer);
                        //printf("received port number %d \n", port_number);
                        t_send_file_from_client(file_name, file_size, port_number);
                    }
                }

            }
        }
    }

    else if (strncmp(args[0], "date", MAX) == 0) {
        send_and_print(sockfd, copy);
    }

    else if (strncmp(args[0], "whoami", MAX) == 0) {
        send_and_print(sockfd, copy);
    }
    else if (strncmp(args[0], "pwd", MAX) == 0) {
        send_and_print(sockfd, copy);

    }
    else if (strncmp(args[0], "login", MAX) == 0){
        if (number_of_args != 2) {
            printf("Error: no username provided \n");
        }
        else {
            char *password = malloc(128);
            password = read_line();
            char **args_password;
            args_password = split_line(password, " ");
            if (args_password[1] == NULL || strlen(args_password[1]) == 0) {
                printf("Error: no password provided \n");
            }
            else {
                char * credentials = (char *) malloc(MAX * sizeof(char));
                strncpy(credentials, "", MAX);
                strncat(credentials, "login ", 7);
                strncat(credentials, args[1], strlen(args[1]));
                strcat(credentials, " ");
                strncat(credentials, "pass ", 6);
                strncat(credentials, args_password[1], strlen(args_password[1]));
                //printf("the credentials are : %s \n", credentials);
                write_to_socket(sockfd, credentials);
                free(password);
                char buffer[1025];
                bzero(buffer, 1025);
                //Check if authentication was succesful
                read(sockfd,buffer,1025);
                printf("%s", buffer);
            }
        }
    }
    else if (strncmp(args[0], "logout", MAX) == 0){
        write_to_socket(sockfd, "logout");
    }

    else if (strncmp(args[0], "grep", MAX) == 0){
        if (number_of_args != 2) {
            printf("Error: no pattern provided \n");
        }
        else {
            write_to_socket(sockfd, copy);
            char buffer[1025];
            bzero(buffer, 1025);
            read(sockfd,buffer,1025);
            printf("%s", buffer);
        }
    }
    else if (strncmp(args[0], "w", MAX) == 0) {
        if (number_of_args != 1) {
            printf("Error: too many arguments \n");
        }
        else{
            write_to_socket(sockfd, copy);
            char buffer[1025];
            bzero(buffer, 1025);
            read(sockfd,buffer,1025);
            printf("%s", buffer);
        }
    }
    else if (strncmp(args[0], "help", MAX) == 0) {
       help();
    }
    else if (strncmp(args[0], "exit", MAX) == 0) {
        write_to_socket(sockfd, copy);
        STATUS = 0;
    }
    else if (strncmp(args[0], "debug", MAX) == 0) {
        printf("passwords\n");
    }
    else {
        //send_command(sockfd, copy);
        printf("Error: unknown command. Type help for list of available commands \n");
        free(copy);
    }
}


int main(int argc, char **argv) {
    if(argc != 3){
        printf("Incorrect number of arguments");
    } else {
        int sockfd;
        struct sockaddr_in servaddr;
        int serverport = atoi(argv[2]);
        char *serverip = argv[1];

        //Socket creation and verification
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            printf("socket creation failed...\n");
            exit(0);
        }
        else
            //printf("Socket successfully created..\n");

        //bzero(&servaddr, sizeof(servaddr));
        bzero((char *) &servaddr, sizeof(servaddr));


        // assign IP, PORT
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(serverip);
        servaddr.sin_port = htons(serverport);

        // connects the client socket to server socket
        if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
            printf("connection with the server failed...\n");
            exit(0);
        }
        else
        puts("\n"
             "  ___  ____   __   ____  ____ \n"
             " / __)(  _ \\ / _\\ / ___)/ ___)\n"
             "( (_ \\ )   //    \\\\___ \\\\___ \\\n"
             " \\___/(__\\_)\\_/\\_/(____/(____/\n"
             "");
        printf("connected to the server..\n");
        puts("Type help for list of available commands. \n");
        char *line = NULL;
        STATUS = 1;
        do {
            printf(">>");
            line = read_line();
            parse_args(sockfd, line);
	    free(line);
        } while (STATUS);

    }



}
