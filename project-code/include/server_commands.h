
//FUNCTION DECLARATION
void ping_command(int socket, char *line);
void date_command(int authenticated, int sockfd);
void login_command(char *line, int sock);
void logout_command(int authenticated, int sockfd);
void w_command(int authenticated, int sock);
void pwd_command(int authenticated, int sockfd, struct Archi *archi);
void ls_command(int authenticated, struct Archi *archi);
void cd_command(int authenticated, char *line, struct Archi *archi);
void mkdir_command(int authenticated, char *line, struct Archi *archi);
void rm_command(int authenticated, char *line, struct Archi *archi);
void get_command(int authenticated, int sockfd, char *line, struct Archi *archi);
void put_command(int authenticated, int sockfd, char *line, struct Archi *archi);
void unconventional_grep_command(int authenticated, int sockfd, struct Dir* current, char* pattern);

void grep_command(struct Dir* current, char* buff, char* pattern);


