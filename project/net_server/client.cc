#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s [ip] [port]\n", argv[0]);
    printf("example: %s 127.0.0.1 5085\n", argv[0]);
    return -1;
  }

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    printf("Failed to create socket.\n");
    return -1;
  }

  struct sockaddr_in server_addr;
  char buffer[1024];

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(argv[1]);
  server_addr.sin_port = htons(atoi(argv[2]));

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {
    printf("Failed to connect to server.\n");
    return -1;
  }

  printf("Connect successfully.\n");

  for (int i = 0; i < 2000000; ++i) {
    memset(buffer, 0, sizeof(buffer));
    printf("Please input: ");
    scanf("%s", buffer);

    if (send(sockfd, buffer, strlen(buffer), 0) <= 0) {
      printf("Failed to send data.");
      close(sockfd);
      return -1;
    }

    memset(buffer, 0, sizeof(buffer));
    if (recv(sockfd, buffer, sizeof(buffer), 0) < 0) {
      printf("Failed to receive data.");
      close(sockfd);
      return -1;
    }

    printf("recv data: %s\n", buffer);
  }

  return 0;
}
