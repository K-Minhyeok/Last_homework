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

void error_handling(char *message);
void *handle_client(void *arg);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
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

	while (1)
	{
		clnt_adr_sz = sizeof(clnt_adr);    
		clnt_sd = accept(serv_sd, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);	

		// TODO: pthread_create & detach 
		pthread_mutex_lock(&mutx);
		clnt_socks[clnt_cnt++] = clnt_sd;
		pthread_mutex_unlock(&mutx);
	
		pthread_create(&thread, NULL, handle_client, (void*)&clnt_sd);
		pthread_detach(thread);

		printf("Connected client IP(sock=%d): %s \n", clnt_sd, inet_ntoa(clnt_adr.sin_addr));
	}
	
	close(serv_sd);
	return 0;
}

void *handle_client(void *arg)
{
	// TODO: file receiving 
	int clnt_sock = *((int*)arg);
	int str_len = 0, i;
	char msg[BUF_SIZE];
	char file_name[FILE_LEN];
    char dir_name[FILE_LEN];
    int file_count=0;
    int num_file;
	FILE *fp;
    
    printf("hit\n");
    
    read(clnt_sock, dir_name, sizeof(dir_name));
    printf("hit %s\n",dir_name);

    mkdir(dir_name, 0755);
    printf("done");


	read(clnt_sock, file_name, sizeof(file_name));

	fp = fopen(file_name, "wb");

	
	while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0){
	pthread_mutex_lock(&mutx);
    fwrite(msg, 1, str_len, fp);
	pthread_mutex_unlock(&mutx);
	}
	fclose(fp);
	close(clnt_sock);
	
	return NULL;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}