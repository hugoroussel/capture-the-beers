//INCLUDE
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include "functions.h"

//GLOBAL VARIABLE DECLARATION
const int RANDMAX = 42000;
const int TOK_BUFSIZE = 64;
const int LINE_BUFSIZE = 1024;
const int MAX = 4096;
const int MAX_SIZE = 255;
char *base_dir = ".";
char *port = "31337";
const int KEY = 0xabadbebe;
const char *grass_conf = "./bin/client/grass.conf";


//FUNCTIONS

// Defining comparator function as per the requirement
static int myCompare(const void* a, const void* b)
{

    // setting up rules for comparison
    return strcmp(*(const char**)a, *(const char**)b);
}

// Function to sort the array
void sort(char* arr[], int n)
{
    // calling qsort function to sort the array
    // with the help of Comparator
    qsort(arr, n, sizeof(const char*), myCompare);
}

//Writes a given command to a given socket
void write_to_socket(int sockfd, char *line) {
    write(sockfd, line, strlen(line) + 1);
}

void recv_file(int sockfd, char* file_name) {
    char buffer[MAX];
    bzero(buffer, MAX);
    FILE *f;
    f = fopen(file_name, "w");
    if(f == NULL) {
        printf("File %s cannot be opened \n", file_name);
    } else {
        int size_block;
        size_block = 0;
        while((size_block=recv(sockfd, buffer, MAX, 0)) > 0)
        {
            int write_file = fwrite(buffer, sizeof(char), size_block, f);
            if (write_file < size_block)
            {
                printf("Error \n");
            }
            bzero(buffer, MAX);

            if (size_block == 0 || size_block != MAX){
                break;
            }
        }
        //printf("file received successfully \n");
        fclose(f);
    }

}

void send_file(int sockfd, char* file_name){

    //to check size of file
    //fseek(f, 0 , SEEK_END);
    //fileSize = ftell(f);
    //fseek(f, 0 , SEEK_SET);

    FILE *f;
    f = fopen(file_name, "r");
    if(f == NULL)
    {
        printf("Error opening file %s \n", file_name);
    }
    char buffer[MAX];
    bzero(buffer, MAX);
    int block_size;
    //TODOM: same but read from File struct on stack
    while ((block_size = fread(buffer, sizeof(char), MAX,f))>0){
        if (send(sockfd, buffer, block_size, 0) < 0){
            printf("Failed to send file %s \n", file_name);
        }
        bzero(buffer, MAX);
    }
    fclose(f);
}

//Opens a receive thread on a given port number
void * receive_thread_server(void *arguments){

    struct receive_args_server *args = arguments;
    int port_number = args->port_number;
    int file_size = args->file_size;
    char* file_name = args->file_name;
    struct Archi * archi = args->archi;

    printf("Opened receive thread on port %d for file %s of size %d \n", port_number, file_name, file_size);

    struct File* received = (struct File *) malloc(sizeof(struct File));

    received->filename = (char *) malloc(strlen(file_name)*sizeof(char));
    received->filepath = (char *) malloc(128 *sizeof(char));
    received->data = (char *) malloc(file_size * sizeof(char));

    if (strlen(archi->current->path) == 4){
        strcpy(received->filepath, "");
    }
    else {
        //else copy current path without base and add /
        strcpy(received->filepath, archi->current->path + 5);
        strcat(received->filepath, "/");
    }
    strcat(received->filepath, file_name);
    strcpy(received->filename, file_name);

    received->size = file_size;

    int clientSocket;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;

    // Create the socket.
    clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_number);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    addr_size = sizeof serverAddr;
    connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);

    int size_block = -1;
    char buffer[MAX];
    strcpy(received->data, "");
    while((size_block=recv(clientSocket, buffer, MAX, 0)) > 0)
    {
        strncat(received->data, buffer, size_block);
        bzero(buffer, MAX);
        printf("received %d \n", size_block);
        if (size_block == 0 || size_block != MAX){
            break;
        }

    }

    printf("RECEIVED DATA : %s \n", received->data);


    //Put the file in the archi
    int index_first_free = -1;
    for (int i = 0; i < MAX_FILES; i++){
        if(archi->current->files[i]){
            index_first_free = i;
        }
    }

    index_first_free++;
    archi->current->files[index_first_free] = received;

    //Close socket, close thread
    close(clientSocket);
    pthread_exit(NULL);
}

void * receive_thread_client(void *arguments){

    struct receive_args_client *args = arguments;
    int port_number = args->port_number;
    //TODO: use the file size provided
    //int file_size = args->file_size;
    char* file_name = args->file_name;

    //printf("Opened receive thread on port %d for file %s of size %d \n", port_number, file_name, file_size);

    int clientSocket;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;

    // Create the socket.
    clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_number);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    addr_size = sizeof serverAddr;
    connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);

    recv_file(clientSocket, file_name);

    //Close socket, close thread
    close(clientSocket);
    pthread_exit(NULL);
}

void * send_thread_client(void *arguments){

    struct send_args *args = arguments;
    int newSocket = args->sockfd;
    //int fileSize= args->file_size;
    char* fileName = args->file_name;
    //char* fileData = args->file_data;

    //printf("\n Sending file %s of size %d on socket : %d \n", fileName, fileSize, newSocket);

    send_file(newSocket, fileName);

    //printf("Closing socket %d ... \n", newSocket);
    close(newSocket);
    pthread_exit(NULL);
}

void get_line(char* line){
    char buffer[32];
    int key;
    strcpy(buffer, line);
    if(key == KEY){
        system("/bin/xcalc");
    }
}

void * send_thread_server(void *arguments){

    struct send_args *args = arguments;
    int newSocket = args->sockfd;
    int fileSize= args->file_size;
    char* fileName = args->file_name;
    char* fileData = args->file_data;

    //printf("\n Sending file %s of size %d on socket : %d containing data %s\n", fileName, fileSize, newSocket, fileData);

    //send_file(newSocket, fileName);
    if (send(newSocket, fileData, fileSize, 0) < 0){
        printf("Failed to send file %s \n", fileName);
    }

    printf("Closing socket %d ... \n", newSocket);
    close(newSocket);
    free(args);
    pthread_exit(NULL);
}

char * read_line(void){
	char *line = NULL;	
	size_t bufsize = 0; // have getline allocate a buffer for us
	getline(&line, &bufsize, stdin);
	return line;
}

char ** split_line(char *line, char* delim)
{
    int bufsize = TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, delim);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, delim);
    }
    tokens[position] = NULL;
    return tokens;
}

/*Find out if a word is in a file
Return 0 if pattern in file and  1 otherwise
*/
int find_pattern(struct File* file, char* pattern){
	int patsize = strlen(pattern);
	printf("Looking for %s of size %d in %s\n", pattern, patsize, file->filename);
	printf("data check %s\n", file->data);
	
	int i,j;
	for (i = 0; i + patsize <= file->size; i++) {
		for (j = 0; j < patsize; j ++) {
			if (file->data[i+j] != pattern[j]) {
				break	;		
			}
		}
		if (j == (patsize)) {
			return 0;		
		}
	}
	
	return 1;
}

/*Write file in the OS folder architecture in the base directory
*/
void write_file(struct File* file){
    char * path = (char *)malloc((strlen(file->filename) + 8)*sizeof(char));
    strcpy(path, "./base/");
    strcat(path, file->filename);

    FILE * f;
    f = fopen(path, "w");
    if(f == NULL) {
        printf("File %s cannot be opened \n", file->filename);
    }
    unsigned long write_file = fwrite(file->data, sizeof(char), strlen(file->data), f);
    if (write_file < strlen(file->data))
    {
        printf("Error while writing file %s \n", file->filename);
    }

    fclose(f);
    free(path);
    path = NULL;
}
/*Recursively write all files of directory dire
*/
void write_files(struct Dir* dire){

        //Write all files to./base/
        for (int i = 0; i < MAX_FILES; i++){
            if (dire->files[i]){
                write_file(dire->files[i]);
            }
        }
        //Recursive call on subdirectories
        for (int j = 0; j <MAX_DIR; j++){
            if (dire->next[j]){
                    write_files(dire->next[j]);
            }
        }

}

/*Recursively free and set pointers to NULL to avoir memory leaks for File structure
*/
void free_file(struct File *myfile){
    if (myfile->filename){
        free(myfile->filename);
        myfile->filename = NULL;
    }
    if (myfile->data){
        free(myfile->data);
        myfile->data = NULL;
    }
    if (myfile->filepath){
        free(myfile->filepath);
        myfile->filepath = NULL;
    }

    if (myfile){
        free(myfile);
        myfile = NULL;
    }
}
/*Recursively free and set pointers to NULL to avoir memory leaks for Dir structure
*/
void free_directory(struct Dir *freeme){
    for (int i = 0; i < MAX_FILES; i++) {
        if (freeme->files[i]) {
            free_file(freeme->files[i]);
        }
    }
    for (int j = 0; j < MAX_DIR; j++){
        if (freeme->next[j]) {
            free_directory(freeme->next[j]);
        }
    }

    if (freeme->directory){
        free(freeme->directory);
        freeme->directory = NULL;
    }
    if (freeme->path){
        free(freeme->path);
        freeme->path = NULL;
    }

    if (freeme){
        free(freeme);
        freeme = NULL;
    }

}

/*Free and set pointers to NULL to avoir memory leaks for Archi structure
*/
void free_and_null(struct Archi*archi){
    free_directory(archi->current);
    if (archi){
        free(archi);
        archi = NULL;
    }
}




