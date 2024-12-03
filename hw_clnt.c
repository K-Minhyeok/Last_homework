#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>

#define FILE_LEN 32
#define MAX_NUM_FILE 32
#define BUF_SIZE 1024

typedef struct File_data
{
	char file_name[FILE_LEN];	
	//char file_path[FILE_LEN];

}File_data;

pthread_t thread; 
int count=0;
struct sockaddr_in serv_adr;


void error_handling(char *message);
void *handle_client(void *arg);


void list_files_for_count(char *dir_path) {
    struct dirent *entry;
    DIR *dp = opendir(dir_path);

    if (dp == NULL) {
        error_handling("Directory open error");
    }

    while ((entry = readdir(dp)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat check_dir;
        if (stat(full_path, &check_dir) == -1) {
            perror("stat error");
            continue;
        }

        if (S_ISREG(check_dir.st_mode)) {
			count++;
        }
        else if (S_ISDIR(check_dir.st_mode)) {
            list_files_for_count(full_path);
        }
    }

    closedir(dp);
}

void list_files_for_thread(char *dir_path) {
    struct dirent *entry;
	File_data file_data_to_send;

    DIR *dp = opendir(dir_path);

    if (dp == NULL) {
        error_handling("Directory open error");
    }

    while ((entry = readdir(dp)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat check_dir;
        if (stat(full_path, &check_dir) == -1) {
            perror("stat error");
            continue;
        }

        if (S_ISREG(check_dir.st_mode)) {
            printf("File: %s\n", full_path);
			strcpy(file_data_to_send.file_name,full_path);
			pthread_create(&thread, NULL, handle_client, (void*)&file_data_to_send);
			pthread_join(thread,NULL);

        }
        else if (S_ISDIR(check_dir.st_mode)) {
            list_files_for_thread(full_path);
        }
    }

    closedir(dp);
}



int main(int argc, char *argv[])
{
	int sd;

	if (argc != 4) {
		printf("Usage: %s <IP> <port> <dir name> \n", argv[0]);
		exit(1);
	}
	
	sd = socket(PF_INET, SOCK_STREAM, 0);   

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));

	connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
	

	list_files_for_count(argv[3]);
	printf("%d === \n",count);
	//file_data = (File_data *)malloc(sizeof(File_data)*count);
	list_files_for_thread(argv[3]);
	

    write(sd, argv[3], FILE_LEN);
	write(sd, &count, sizeof(count));

	close(sd);
	return 0;
}


void *handle_client(void *arg)
{
	// TODO: file receiving 
	File_data file_data = *((File_data*)arg);
	int clnt_sock= socket(PF_INET, SOCK_STREAM, 0);
	connect(clnt_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
	//이건 새로 연결할 필요가 있다 싶을 때 쓰기

	int str_len = 0, i;
	char msg[BUF_SIZE];
	char file_name[FILE_LEN];
    char dir_name[FILE_LEN];
    int file_count=0;
    int num_file;
	int read_cnt;
	int read_size=0;
	FILE *fp;

 

    printf("=======thread hit for %s\n",file_data.file_name);
    //파일 이름 보내기
	write(clnt_sock, file_data.file_name, sizeof(file_data.file_name));

	//파일 보내기
	fp = fopen(file_data.file_name, "rb");
	
	printf("hit fp\n");
	while(1)
		{
			read_cnt = fread((void*)msg, 1, BUF_SIZE, fp);
			read_size += read_cnt;

			if (read_cnt < BUF_SIZE)
			{
				write(clnt_sock, msg, read_cnt);
				printf("Sent %d bytes in %s\n", read_size,file_data.file_name);
				break;
			}
			write(clnt_sock, msg, BUF_SIZE);
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

