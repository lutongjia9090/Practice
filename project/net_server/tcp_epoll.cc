#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// set non-blocking
void SetNonblocking(int fd) {
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}
int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s [ip] [port]\n", argv[0]);
    printf("example: %s 127.0.0.1 5085\n", argv[0]);
    return -1;
  }

  int listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listen_fd < 0) {
    printf("Failed to create socket.\n");
    return -1;
  }

  // set listen_fd
  int opt = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
             static_cast<socklen_t>(sizeof(opt)));
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &opt,
             static_cast<socklen_t>(sizeof(opt)));
  setsockopt(listen_fd, SOL_SOCKET, TCP_NODELAY, &opt,
             static_cast<socklen_t>(sizeof(opt)));
  setsockopt(listen_fd, SOL_SOCKET, SO_KEEPALIVE, &opt,
             static_cast<socklen_t>(sizeof(opt)));
  SetNonblocking(listen_fd);

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(argv[1]);
  server_addr.sin_port = htons(atoi(argv[2]));
  if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    printf("Failed to bind server addr.");
    return -1;
  }

  if (listen(listen_fd, 128) != 0) {
    printf("Failed to listen.");
    close(listen_fd);
    return -1;
  }

  int epoll_fd = epoll_create(1);

  // prepare epoll event for listen fd
  epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = listen_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);

  struct epoll_event evs[10];
  while (true) {
    int nfds = epoll_wait(epoll_fd, evs, 10, -1);
    if (nfds < 0) {
      printf("epoll_wait failed.\n");
      break;
    }

    if (nfds == 0) {
      printf("epoll_wait timeout.\n");
      continue;
    }

    for (int i = 0; i < nfds; ++i) {
      if (evs[i].data.fd == listen_fd) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd =
            accept(listen_fd, (struct sockaddr *)&client_addr, &len);
        SetNonblocking(client_fd);
        printf("accept a new client: fd=%d, ip=%s, port=%d.\n", client_fd,
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        ev.data.fd = client_fd;
        ev.events = EPOLLIN | EPOLLET;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
      } else {
        // client close, use `EPOLLIN` in some systems(recv() return 0)
        if (evs[i].events & EPOLLRDHUP) {
          printf("Client(event fd=%d) disconnected.", evs[i].data.fd);
          close(evs[i].data.fd);
        } else if (evs[i].events & EPOLLIN) {
          char buffer[1024];
          while (true) {
            bzero(&buffer, sizeof(buffer));
            ssize_t nread = read(evs[i].data.fd, buffer, sizeof(buffer));
            if (nread > 0) {
              printf("recv(eventfd=%d): %s\n", evs[i].data.fd, buffer);
              send(evs[i].data.fd, buffer, strlen(buffer), 0);
            } else if (nread == -1 &&
                       errno == EINTR) { // Signal interrupt while reading
              continue;
            } else if (nread == -1 &&
                       ((errno == EAGAIN) ||
                        (errno == EWOULDBLOCK))) { // Finished reading
              break;
            } else if (nread == 0) {
              printf("Client(eventfd=%d) disconnected.\n", evs[i].data.fd);
              close(evs[i].data.fd);
              break;
            }
          }
        } else if (evs[i].events & EPOLLOUT) { // Write event
        } else {                               // error
          printf("Client(eventfd=%d) error.\n", evs[i].data.fd);
          close(evs[i].data.fd);
        }
      }
    }
  }

  return 0;
}
