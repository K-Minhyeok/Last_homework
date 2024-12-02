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
#define BUF_SIZE 1024

struct file_data
{
	int file_size;
	char file_name[FILE_LEN];	
	char file_path[FILE_LEN];
};


void error_handling(char *message);
void list_files(const char *dir_path) {
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
            printf("File: %s\n", full_path);
			//여기서 thread 만들어서, 구조체 넘겨주기
        }
        else if (S_ISDIR(check_dir.st_mode)) {
            printf("Directory: %s\n", full_path);
            list_files(full_path);
        }
    }

    closedir(dp);
}


int sd;

int main(int argc, char *argv[])
{
	FILE *fp;
	
	char file_name[FILE_LEN];
	char buf[BUF_SIZE];
	int read_cnt;
	int read_size;
	struct sockaddr_in serv_adr;
	if (argc != 4) {
		printf("Usage: %s <IP> <port> <dir name> \n", argv[0]);
		exit(1);
	}
	
	fp = fopen(argv[3], "rb");
	sd = socket(PF_INET, SOCK_STREAM, 0);   

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));

	connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
	
	list_files(argv[3]);

    // Send directory name
    write(sd, argv[3], FILE_LEN);
    printf("Sending directory: %s\n", argv[3]);



	// Send file name 
	strcpy(file_name, argv[3]);
	write(sd, file_name, FILE_LEN);

	// Send file data 
	read_size = 0;
	while(1)
	{
		read_cnt = fread((void*)buf, 1, BUF_SIZE, fp);
		read_size += read_cnt;
		if (read_size % 1024 == 0)
			printf("Send %d bytes \n", read_size);

		if (read_cnt < BUF_SIZE)
		{
			write(sd, buf, read_cnt);

			break;
		}
		write(sd, buf, BUF_SIZE);
	}

	
	shutdown(sd, SHUT_WR);

	fclose(fp);
	close(sd);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}