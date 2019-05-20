//GLOBAL VARIABLES
const int LINE_BUFSIZE;
const int TOK_BUFSIZE;
const int RANDMAX;
const int MAX;
const int MAX_SIZE;
char *base_dir;
char *port;
const int KEY ;
const char *grass_conf;
int client_socket[30];
int STATUS;

//CONSTANTS FOR STRUCTURE DEFINITION
#define MAX_FILES 1005
#define MAX_DIR 100

//STRUCTURES
struct send_args {
    int sockfd;
    int file_size;
    char* file_name;
    char* file_data;
};

struct receive_args_client {
    int port_number;
    int file_size;
    char* file_name;
};

struct receive_args_server {
    int port_number;
    int file_size;
    char* file_name;
    struct Archi* archi;
};

//Store files on the heap
struct File {
	//socket identifier
	int sockfd;
	//filename
	char* filename;
	//filepath
	char* filepath;
	//filesize;
	int size;
	//pointer to data
	char* data;
};

//Describe directories
struct Dir {
    //socket identifier
    int sockfd;
    //path;
    char* path;
    //directory name;
    char* directory;
    //previous node
    struct Dir* prev;
    //list of next nodes
    struct Dir* next[MAX_DIR];
    //list of files in directory
    struct File* files[MAX_FILES];
};

//Navigate through different directories using a pointer to the current one
struct Archi{
    int sockfd;
    struct Dir* current;
};

//FUNCTION DECLARATION
void t_send_file_server(int sockfd, char *file_name, struct Archi *archi);
void sort(char* arr[], int n);
char *read_line(void);
char **split_line(char *line, char* delim);
void parse_args(int sockfd, char* line);
void send_command_server(int sockfd, char *line);
void recv_file(int sockfd, char* file_name);
void send_file(int sockfd, char* file_name);
void write_to_socket(int sockfd, char *line);
size_t getFilesize(const char* filename);
int find_pattern(struct File* file, char* pattern);
void * receive_thread_server(void *arguments);
void * send_thread_server(void *arguments);
void * receive_thread_client(void *arguments);
void * send_thread_client(void *arguments);
void write_file(struct File* file);
void free_and_null(struct Archi*archi);
void free_directory(struct Dir* freeme);
void free_file(struct File *myfile);
void write_files(struct Dir* dire);

