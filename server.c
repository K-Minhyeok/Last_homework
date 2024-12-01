#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>

#define FILE_LEN 32
#define BUF_SIZE 1024

void error_handling(char *message);
void *handle_client(void *arg);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int serv_sd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sd, 5) == -1)
        error_handling("listen() error");

    while (1) {
        struct sockaddr_in clnt_addr;
        socklen_t clnt_addr_sz = sizeof(clnt_addr);
        int clnt_sd = accept(serv_sd, (struct sockaddr *)&clnt_addr, &clnt_addr_sz);
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, (void *)&clnt_sd);
        pthread_detach(thread);
        printf("Connected client IP: %s\n", inet_ntoa(clnt_addr.sin_addr));
    }

    close(serv_sd);
    return 0;
}

void *handle_client(void *arg) {
    int clnt_sock = *((int *)arg);
    char dir_name[FILE_LEN];
    read(clnt_sock, dir_name, sizeof(dir_name));

    // Create directory
    mkdir(dir_name, 0755);

    char file_name[FILE_LEN];
    long total_bytes = 0;

    while (1) {
        int read_size = read(clnt_sock, file_name, sizeof(file_name));
        if (read_size <= 0) break; // Connection closed

        FILE *fp = fopen(file_name, "wb");
        if (!fp) {
            perror("File open error");
            continue;
        }

        char buf[BUF_SIZE];
        long file_bytes = 0;
        while ((read_size = read(clnt_sock, buf, sizeof(buf))) > 0) {
            fwrite(buf, 1, read_size, fp);
            file_bytes += read_size;
        }

        fclose(fp);
        total_bytes += file_bytes;
        printf("[%s] Received from … %ld bytes\n", file_name, file_bytes);  // 수정된 부분
    }

    printf("The client has completed the upload of %ld bytes!\n", total_bytes);

    close(clnt_sock);
    return NULL;
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
