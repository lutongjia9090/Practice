// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutonjia@163.com)
//

#include "inet_adress.h"
#include "socket.h"
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

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s [ip] [port]\n", argv[0]);
    printf("example: %s 127.0.0.1 5085\n", argv[0]);
    return -1;
  }

  Socket server_sock(CreateSocket());
  server_sock.SetReuseAddr(true);
  server_sock.SetReusePort(true);
  server_sock.SetTcpNoDelay(true);
  server_sock.SetKeepAlive(true);

  InetAdress server_addr(argv[1], atoi(argv[2]));
  server_sock.Bind(*server_addr.GetSockAddr());
  server_sock.Listen();

  int epoll_fd = epoll_create(1);

  // prepare epoll event for listen fd
  epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = server_sock.GetFd();
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sock.GetFd(), &ev);

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
      // client close, use `EPOLLIN` in some systems(recv() return 0)
      if (evs[i].events & EPOLLRDHUP) {
        printf("Client(event fd=%d) disconnected.", evs[i].data.fd);
        close(evs[i].data.fd);
      } else if (evs[i].events & EPOLLIN) {
        if (evs[i].data.fd == server_sock.GetFd()) {
          InetAdress client_addr;
          Socket *client_sock = new Socket(server_sock.Accept(client_addr));

          ev.data.fd = client_sock->GetFd();
          ev.events = EPOLLIN | EPOLLET;
          epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock->GetFd(), &ev);
        } else {
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
        }
      } else if (evs[i].events & EPOLLOUT) { // Write event
      } else {                               // error
        printf("Client(eventfd=%d) error.\n", evs[i].data.fd);
        close(evs[i].data.fd);
      }
    }
  }

  return 0;
}
