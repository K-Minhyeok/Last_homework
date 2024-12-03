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

void error_handling(char *message);

typedef struct {
    int sock;
    char file_path[FILE_LEN];
} upload_arg;

void *upload_file(void *arg) {
    upload_arg *u_arg = (upload_arg *)arg;
    int sock = u_arg->sock;
    char *file_path = u_arg->file_path;

    FILE *fp = fopen(file_path, "rb");
    if (!fp) {
        perror("File open error");
        free(u_arg);
        return NULL;
    }

    // Send file name
    write(sock, file_path, sizeof(file_path));

    char buf[BUF_SIZE];
    int read_cnt;
    long total_bytes = 0;

    while ((read_cnt = fread(buf, 1, BUF_SIZE, fp)) > 0) {
        write(sock, buf, read_cnt);
        total_bytes += read_cnt;
    }

    fclose(fp);
    free(u_arg);

    printf("[%s] Upload completed! … %ld bytes\n", file_path, total_bytes);
    return NULL;
}

int main(int argc, char *argv[]) {
    int sd;
    struct sockaddr_in serv_adr;

    if (argc != 4) {
        printf("Usage: %s <IP> <port> <dir name>\n", argv[0]);
        exit(1);
    }

    sd = socket(PF_INET, SOCK_STREAM, 0);   

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));

    DIR *dir = opendir(argv[3]);
    if (dir == NULL) {
        error_handling("Directory open error");
    }

    // Send directory name
    write(sd, argv[3], FILE_LEN);
    printf("Sending directory: %s\n", argv[3]);

    struct dirent *entry;
    pthread_t threads[256]; // 최대 256개의 파일 업로드 스레드
    int thread_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // 일반 파일인 경우
            char file_path[FILE_LEN];
            snprintf(file_path, sizeof(file_path), "%s/%s", argv[3], entry->d_name);

            upload_arg *arg = (upload_arg *)malloc(sizeof(upload_arg));
            arg->sock = sd;
            strcpy(arg->file_path, file_path);

            pthread_create(&threads[thread_count], NULL, upload_file, (void *)arg);
            thread_count++;
        }
    }

    closedir(dir);

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    shutdown(sd, SHUT_WR);
    close(sd);
    return 0;
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
