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
#include "server_commands.h"


static struct User **userlist;
static int numUsers;


/*Send file from server to client
*/
void t_send_file_server(int sockfd, char *file_name, struct Archi *archi) {
    //Search the file in our architecture
    int index_file = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (strncmp(archi->current->files[i]->filename, file_name, strlen(archi->current->files[i]->filename)) == 0) {
            index_file = i;
            break;
        }

    }
    int file_size = archi->current->files[index_file]->size;

    //Assign port number
    srand(time(NULL));
    int port_number = rand() % 10000;

    char file_specs[64];
    char file_size_buffer[32];
    char port_number_buffer[32];
    sprintf(port_number_buffer, "%d", port_number);
    sprintf(file_size_buffer, "%d", file_size);
    strcpy(file_specs, "get port: ");
    strcat(file_specs, port_number_buffer);
    strcat(file_specs, " size: ");
    strcat(file_specs, file_size_buffer);
    printf("file specs : %s \n", file_specs);

    //Send file specs to client
    write_to_socket(sockfd, file_specs);

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
    if (listen(serverSocket, 50) == 0) {
        printf("Created new socket. Listening\n");
    }

    //Declare new thread
    pthread_t tid;

    addr_size = sizeof serverStorage;
    newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);

    //Argument structure of server_thread_send
    struct send_args *args = (struct send_args *) malloc(sizeof(struct send_args));
    args->sockfd = newSocket;
    args->file_size = file_size;
    args->file_name = malloc(128);
    args->file_data = malloc(file_size);
    strcpy(args->file_name, archi->current->files[index_file]->filename);
    strcpy(args->file_data, archi->current->files[index_file]->data);

    if (pthread_create(&tid, NULL, send_thread_server, (void *) args) != 0) {
        printf("Failed to create thread\n");
    }

    close(serverSocket);


}

/**
 * Authentification management commands
**/

/*Whoami implementation
*/
void whoami_command(int authenticated, int sockfd, struct Archi *archi){
    if (authenticated) {
        for (int i = 0; i < numUsers; i++) {
            if (userlist[i]->sockID == sockfd && userlist[i]->isLoggedIn) {
                char *buffer = (char *)malloc(MAX*sizeof(char)) ;
                strncpy(buffer, userlist[i]->uname, strlen(userlist[i]->uname) + 1);
                strcat(buffer, "\n");
                write_to_socket(sockfd, buffer);
                bzero(buffer, MAX);
                free(buffer);
            }
        }
    } else {
        write_to_socket(archi->sockfd, "Error: access denied \n");
    }
}

/*W implementation
*/
void w_command(int authenticated, int sock) {
    if(authenticated){

        char *buffer = (char *) malloc(numUsers * 128 * sizeof(char));
        char *space = " ";
        char **all_users = (char **) malloc(numUsers * sizeof(char *));

        for (int i = 0; i < numUsers; i++) {
            all_users[i] = (char *) malloc(128 * sizeof(char));
            bzero(all_users[i], 128);
        }
        for (int i = 0; i < numUsers; i++) {
            if (userlist[i]->isLoggedIn) {
                sprintf(all_users[i], "%s", userlist[i]->uname);
            }
        }
        sort(all_users, numUsers);
        for (int i = 0; i < numUsers; i++) {
            if (strlen(all_users[i]) != 0) {
                strncat(buffer, all_users[i], strlen(all_users[i]) + 1);
                strncat(buffer, space, 2);
            }
        }
        strcat(buffer, "\n");

        write_to_socket(sock, buffer);
        for (int i = 0; i < numUsers; i++) {
            free(all_users[i]);
            all_users[i] = NULL;
        }
        free(all_users);
        all_users = NULL;

        //finally release the first string
        bzero(buffer, numUsers*128);
        free(buffer);
        buffer = NULL;
    }
    else {
        write_to_socket(sock, "Error: access denied \n");
    }
}

/*Login implementation
*/
void login_command(char *line, int sock) {

    char *user = (char *) malloc(MAX_SIZE * sizeof(char));
    char *pass = (char *) malloc(MAX_SIZE * sizeof(char));

    char *ptr = strtok(line, " ");
    int index = 0;

    while (ptr != NULL) {
        if (index == 1) {
            memcpy(user, ptr, strlen(ptr) + 1);
        } else if (index == 3) {
            memcpy(pass, ptr, strlen(ptr) + 1);
        }
        ptr = strtok(NULL, " ");
        index++;
    }

    free(ptr);
    ptr = NULL;

    //Find and logout the current user
    for (int i = 0; i < numUsers; i++) {
        if (userlist[i]->sockID == sock) {
            if (userlist[i]->isLoggedIn){
                logout_command(1, sock);
            }
        }
    }

    int i = 0;
    int userFound = 0;
    while (i < numUsers && userFound == 0) {
        //If user exists...
        if (strncmp(userlist[i]->uname, user, strlen(user) + 1) == 0) {
            userFound = 1;
            //... And is not already logged in
            if (userlist[i]->isLoggedIn == 0) {
                //If pass is correct
                if (strncmp(userlist[i]->pass, pass, strlen(pass) + 1) == 0) {
                    userlist[i]->sockID = sock;
                    userlist[i]->isLoggedIn = 1;
                    printf("successful authentication of %s \n", user);
                    write_to_socket(sock, "Authentication successful. \n");
                } else {
                    write_to_socket(sock, "Authentication failed. \n");
                }
            } else {
                write_to_socket(sock, "Error: user is already logged in! \n");
            }
        }
        i++;
    }
    if (userFound == 0) {
        write_to_socket(sock, "Error: unknown user! \n");
    }

    free(user);
    user = NULL;
    free(pass);
    pass = NULL;
}

/*Logout implementation
*/
void logout_command(int authenticated, int sockfd){
    if (authenticated) {
        for (int i = 0; i < numUsers; i++) {
            if (userlist[i]->sockID == sockfd) {
                userlist[i]->isLoggedIn = 0;
                printf("successful logout %s \n", userlist[i]->uname);
            }
        }
    } else {
        write_to_socket(sockfd, "Error: access denied \n");
    }
}

/*Exit implementation
*/
void exit_command(int sockfd, struct Archi *client_archi){
    //Logout the user
    logout_command(1, sockfd);
    int index = -1;
    for(int i=0; i <30; i++){
        if(client_socket[i] == sockfd){
            index = i;
            break;
        }
    }
    client_socket[index] = 0;
    close(sockfd);
    free_and_null(client_archi);
    client_archi = NULL;
}

/**
 * Server functions
**/

/*Switch between client command's
*/
void read_client(int sockfd, char *line, struct Archi *archi) {
    //Check  if user is authenticated
    int authenticated = 0;
    for(int i = 0; i < numUsers; i++){
        if(userlist[i]->sockID == sockfd && userlist[i]->isLoggedIn){
            authenticated = 1;
            break;
        }
    }

    char *copy = malloc(strlen(line) + 1);
    strcpy(copy, line);
    char **args;
    args = split_line(line, " \t\r\n\a");

    if (strncmp(args[0], "ping", 4) == 0) {
        ping_command(sockfd, copy);
    }
    else if ((strncmp(args[0], "login", 5) == 0)) {
        login_command(copy, sockfd);
    }
    else if (strncmp(args[0], "pwd", 3) == 0) {
        pwd_command(authenticated, sockfd, archi);
    }
    else if (strncmp(args[0], "ls", 2) == 0) {
        ls_command(authenticated, archi);
    }
    else if (strncmp(args[0], "cd", 2) == 0) {
        cd_command(authenticated, copy, archi);
    }
    else if (strncmp(args[0], "mkdir", 5) == 0) {
        mkdir_command(authenticated, copy, archi);
    }
    else if (strncmp(args[0], "rm", 2) == 0) {
        rm_command(authenticated, copy, archi);
    }
    else if (strncmp(args[0], "get", 3) == 0) {
        get_command(authenticated, sockfd, copy, archi);
    }
    else if (strncmp(args[0], "put", 3) == 0) {
        put_command(authenticated, sockfd, copy, archi);
    }
    else if (strncmp(args[0], "date", 4) == 0) {
        date_command(authenticated, sockfd);
    }
    else if (strncmp(args[0], "whoami", 6) == 0) {
        whoami_command(authenticated, sockfd, archi);
    }
    else if (strncmp(args[0], "logout", 6) == 0) {
        logout_command(authenticated, sockfd);
    }
    else if (strncmp(args[0], "exit", 4) == 0) {
        exit_command(sockfd, archi);
    }
    else if (strncmp(args[0], "w", 1) == 0) {
        w_command(authenticated, sockfd);
    }
    else if (strncmp(args[0], "grep", 4) == 0) {
        unconventional_grep_command(authenticated, archi->sockfd,  archi->current, args[1]);
    } else {
        printf("Error: server received unknown command\n");
    }
    bzero(copy, strlen(line) + 1);
    free(copy);
    copy = NULL;
    return;

}

// Parse the grass.conf file and fill in the global variables
void parse_grass() {
    userlist = (struct User **) malloc(sizeof(struct User));
    FILE *fp;
    char buff[MAX_SIZE];
    char delim[] = " ";
    char *token;
    //Open file in read mode
    if ((fp = fopen(grass_conf, "r")) == NULL) {
        printf("Error! opening file: %s \n", strerror(errno));
        exit(1);
    }

    //Read file line by line
    while (fgets(buff, MAX_SIZE, fp) != NULL) {
        token = strtok(buff, delim);
        //Init base directory
        if (strncmp(token, "base", 4) == 0) {
            token = strtok(NULL, delim);
            char *base = malloc(MAX_SIZE * sizeof(token));
            memcpy(base, token, MAX_SIZE * sizeof(token) + 1);
            base_dir = malloc(strlen(base) + 1);
            strncpy(base_dir, base, strlen(base) + 1);

        } else if (strncmp(token, "port", 4) == 0) {
            token = strtok(NULL, delim);
            char *p = malloc(MAX_SIZE * sizeof(token));
            memcpy(p, token, MAX_SIZE * sizeof(token) + 1);
            port = malloc(strlen(p) + 1);
            strncpy(port, p, strlen(p) + 1);

        } else if (strncmp(token, "user", 4) == 0) {
            //Read first token: user name
            token = strtok(NULL, delim);
            char * uname = (char *) malloc(sizeof(char) *(MAX_SIZE * sizeof(token)));
            memcpy(uname, token, MAX_SIZE * sizeof(token));
            //Read second token: pass
            token = strtok(NULL, delim);
            char * pass = (char *) malloc(sizeof(char)*(MAX_SIZE * sizeof(token)));
            memcpy(pass, token, MAX_SIZE * sizeof(token));
            //Create new user
            struct User *user = malloc(sizeof(struct User));
            //Copy uname and pass to user params
            user->uname = uname;
            user->pass = pass;
            user->isLoggedIn = 0;

            //Add new user to userlist
            userlist[numUsers] = user;
            //Increment number of users
            numUsers = numUsers + 1;
            //Increase memory of user list
            userlist = (struct User **) realloc(userlist, numUsers * sizeof(struct User));
            //Skip comments and new lines
        } else if (strncmp(token, "#", 1) != 0 && strncmp(token, "\n", 1) != 0) {
            printf("Unknown config: %s\n", token);
        }
    }
    fclose(fp);
}

int main() {
    //Get configuration details from grass.conf
    parse_grass();

    int master_socket, addrlen, new_socket, max_clients = 30, activity, i, valread, sd;
    int max_sd;
    struct sockaddr_in address;
    char buffer[1025]; 

    //Init select loop constant: set of socket descriptors
    fd_set readfds;

    //Create an array of archi struct for each client and set it to NULL
    struct Archi *client_archi[max_clients];

    //Initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++) {
        client_socket[i] = 0;
        client_archi[i] = NULL;
    }

    //Create a master socket
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //Set socket details
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    int port_n = atoi(port);
    address.sin_port = htons(port_n);

    //Bind the socket to localhost port 8080
    if (bind(master_socket, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", port_n);

    //Specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //Accept incoming connections
    addrlen = sizeof(address);
    puts("Waiting for connections ...\n");

    while (1) {
        //Clear the socket set
        FD_ZERO(&readfds);

        //Add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        //Add child sockets to set
        for (i = 0; i < max_clients; i++) {
            sd = client_socket[i];

            //If valid socket descriptor then add to read list
            if (sd > 0)
                FD_SET(sd, &readfds);

            //Highest file descriptor number, need it for the select function
            if (sd > max_sd)
                max_sd = sd;
        }

        //Wait indefinitely for an activity on one of the sockets 
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error\n");
        }

        //When  incoming connection
        if (FD_ISSET(master_socket, &readfds)) {
            if ((new_socket = accept(master_socket, (struct sockaddr *) &address, (socklen_t * ) & addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //Inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n", new_socket,
                   inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            //Add new socket to array of sockets
            for (i = 0; i < max_clients; i++) {
                //If position is empty
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;

                    //Initialisation archi struct for the new client
                    struct Dir *basedir = (struct Dir *) malloc(sizeof(struct Dir));
                    char *dir = (char *) malloc(128 * sizeof(char));
                    char *path = (char *) malloc(128 * sizeof(char));
                    strcpy(dir, "base");
                    strcpy(path, "base");
                    basedir->sockfd = new_socket;
                    basedir->path = path;
                    basedir->directory = dir;

                    client_archi[i] = (struct Archi *) malloc(sizeof(struct Archi));
                    client_archi[i]->sockfd = new_socket;
                    client_archi[i]->current = basedir;
                    break;
                }
            }

        }

        //Else it's some IO operation on some other socket
        for (i = 0; i < max_clients; i++) {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds)) {


                //Check if it was for closing , and read the incoming message
                if ((valread = read(sd, buffer, 1024)) <= 0) {
                    //Somebody disconnected, log him out and close socket
                    logout_command(1, sd);
                    getpeername(sd, (struct sockaddr *) &address, (socklen_t * ) & addrlen);
                    printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    close(sd);
                    client_socket[i] = 0;

                    //Free struct 
                    free_and_null(client_archi[i]);
                    client_archi[i] = NULL;

                } else {
                    buffer[valread] = '\0';
                    read_client(sd, buffer, client_archi[i]);
                    bzero(buffer, 1024);
                }

            }
        }
    }
}
