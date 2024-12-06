#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <dirent.h>
#include <signal.h>

#define FILE_LEN 32
#define BUF_SIZE 1024
#define MAX_CLNT 256
#define ADD_LEN 64

typedef struct File_data
{
	char file_name[FILE_LEN];	
	char dir_path[FILE_LEN];
	int file_size;

}File_data;

typedef struct ClientInfo {
    int sock;
    char ip[ADD_LEN];
} ClientInfo;


void error_handling(char *message);
void *handle_client(void *arg);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
int has_dir = 0; //디렉토리 이름 받았는지 판단
char dir_name[FILE_LEN];
int num_file=0;
int file_count=0;
int quit=0;
char last_ip[ADD_LEN];


pthread_mutex_t mutx;

int main(int argc, char *argv[])
{
	int serv_sd, clnt_sd;
	pthread_t thread; 
	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t clnt_adr_sz;

	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}
	
	serv_sd = socket(PF_INET, SOCK_STREAM, 0);   
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));
	
	if (bind(serv_sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if (listen(serv_sd, 5) == -1)
		error_handling("listen() error");

    pthread_mutex_init(&mutx, NULL); 


	while (1)
	{
		clnt_adr_sz = sizeof(clnt_adr);    
		clnt_sd = accept(serv_sd, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);	
		
		ClientInfo *client_info = malloc(sizeof(ClientInfo));
		client_info->sock = clnt_sd;
		inet_ntop(AF_INET, &clnt_adr.sin_addr, client_info->ip, INET_ADDRSTRLEN);
		strcpy(last_ip , client_info->ip);
		// TODO: pthread_create & detach 
		pthread_mutex_lock(&mutx);
		clnt_socks[clnt_cnt++] = clnt_sd;
		pthread_mutex_unlock(&mutx);
	
		pthread_create(&thread, NULL, handle_client,  (void*)client_info);
		pthread_detach(thread);

	}
	
	close(serv_sd);
	return 0;
}

void *handle_client(void *arg)
{
	// TODO: file receiving 
	
	ClientInfo *client_info = (ClientInfo*)arg;
	int clnt_sock = client_info->sock;
	int str_len = 0, i;
	char msg[BUF_SIZE];
	char file_name[FILE_LEN];
	int file_size=0;
	FILE *fp;

	pthread_mutex_lock(&mutx);

    if(has_dir==0){   
    read(clnt_sock, dir_name, sizeof(dir_name));
	has_dir =1;

	if (mkdir(dir_name, 0755) == -1) { 
            perror("mkdir() error");
            pthread_mutex_unlock(&mutx);
            close(clnt_sock);
            free(client_info);
            return NULL;
        }


    read(clnt_sock, &file_count, sizeof(file_count));

	}

	pthread_mutex_unlock(&mutx);

	//파일 관련 구조체 받기
	File_data file_data;
	read(clnt_sock, &file_data, sizeof(file_data));


	if (file_data.file_name[0] == '\0') { 

    return NULL; 
}

    if (access(file_data.dir_path, F_OK) == -1) {
        if (mkdir(file_data.dir_path, 0755) == -1) { 
            perror("mkdir() error");
            close(clnt_sock);
            free(client_info);
            return NULL;
        }
    }
	

	fp = fopen(file_data.file_name, "wb");
	while (((str_len = read(clnt_sock, msg, sizeof(msg))) != 0)&& quit ==0){

		if (strstr(msg, "de3ac21778e51de199438300e1a9f816c618d33a") != NULL) {
			quit=1;
			printf("quit!\n");
		}
	pthread_mutex_lock(&mutx);
    fwrite(msg, 1, str_len, fp);
	file_size+=str_len;
	pthread_mutex_unlock(&mutx);
	}
	fclose(fp);
 	pthread_mutex_lock(&mutx);
	if(file_size==file_data.file_size){
    num_file++; 
	printf("[%s] Received from %s... %d bytes \n",file_data.file_name,client_info->ip,file_size);
	}
    pthread_mutex_unlock(&mutx);

	close(clnt_sock);

	if(file_size==0){
		return NULL;
	}
	if(file_count == num_file ){
	printf("==== The client(%s) has completed the upload of %d/%d ====\n", client_info->ip, num_file, file_count);

	pthread_mutex_lock(&mutx);
    num_file=0; 
	has_dir=0;
    pthread_mutex_unlock(&mutx);
	}
	
	else if(quit ==1){
	printf("==== File upload cancellation message is received from %s ====\n",client_info->ip );

	}
	
	free(client_info);
	return NULL;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}