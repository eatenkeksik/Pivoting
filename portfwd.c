#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if(argc < 4) {
        fprintf(stderr, "Usage: %s <local_port> <remote_host> <remote_port>\n", argv[0]);
        return -1;
    }

    int local_port = atoi(argv[1]);
    char *remote_host = argv[2];
    int remote_port = atoi(argv[3]);

    int local_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(local_socket == -1) {
        perror("socket() failed");
        return -1;
    }

    struct sockaddr_in local_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(local_port),
        .sin_addr.s_addr = INADDR_ANY
    };
    if(bind(local_socket, (struct sockaddr*)&local_addr, sizeof(local_addr)) == -1) {
        perror("bind() failed");
        close(local_socket);
        return -1;
    }

    if(listen(local_socket, 5) == -1) {
        perror("listen() failed");
        close(local_socket);
        return -1;
    }

    printf("Port forwarding started: localhost:%d -> %s:%d\n", local_port, remote_host, remote_port);

    while(1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(local_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if(client_socket == -1) {
            perror("accept() failed");
            continue;
        }
        int remote_socket = socket(AF_INET, SOCK_STREAM, 0);
        if(remote_socket == -1) {
            perror("socket() failed");
            close(client_socket);
            continue;
        }
        struct sockaddr_in remote_addr = {
            .sin_family = AF_INET,
            .sin_port = htons(remote_port),
            .sin_addr.s_addr = inet_addr(remote_host)
        };
        if(connect(remote_socket, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) == -1) {
            perror("connect() failed");
            close(client_socket);
            close(remote_socket);
            continue;
        }

        printf("New connection: %s:%d -> %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), remote_host, remote_port);

        fd_set set;
        FD_ZERO(&set);
        int max_fd = (client_socket > remote_socket) ? client_socket : remote_socket;
        while(1) {
            FD_SET(client_socket, &set);
            FD_SET(remote_socket, &set);
            if(select(max_fd + 1, &set, NULL, NULL, NULL) == -1) {
                perror("select() failed");
                break;
            }
            if(FD_ISSET(client_socket, &set)) {
                char buf[1024];
                ssize_t bytes_read = recv(client_socket, buf, sizeof(buf), 0);
                if(bytes_read <= 0) break;
                send(remote_socket, buf, bytes_read, 0);
            }
            if(FD_ISSET(remote_socket, &set)) {
                char buf[1024];
                ssize_t bytes_read = recv(remote_socket, buf, sizeof(buf), 0);
                if(bytes_read <= 0) break;
                send(client_socket, buf, bytes_read, 0);
            }
        }

        printf("Connection closed: %s:%d -> %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), remote_host, remote_port);

        close(client_socket);
        close(remote_socket);
    }

    close(local_socket);

    return 0;
}
