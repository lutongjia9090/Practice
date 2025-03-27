// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutonjia@163.com)
//

#include "epoll.h"
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

  // prepare epoll event for listen fd
  Epoll epoll;
  epoll.AddFd(server_sock.GetFd(), EPOLLIN);

  std::vector<epoll_event> evs;
  while (true) {
    evs = epoll.Loop(-1);
    for (auto &ev : evs) {
      // client close, use `EPOLLIN` in some systems(recv() return 0)
      if (ev.events & EPOLLRDHUP) {
        printf("Client(event fd=%d) disconnected.", ev.data.fd);
        close(ev.data.fd);
      } else if (ev.events & EPOLLIN) {
        if (ev.data.fd == server_sock.GetFd()) {
          InetAdress client_addr;
          Socket *client_sock = new Socket(server_sock.Accept(client_addr));

          epoll.AddFd(client_sock->GetFd(), EPOLLIN | EPOLLET);
        } else {
          char buffer[1024];
          while (true) {
            bzero(&buffer, sizeof(buffer));
            ssize_t nread = read(ev.data.fd, buffer, sizeof(buffer));
            if (nread > 0) {
              printf("recv(eventfd=%d): %s\n", ev.data.fd, buffer);
              send(ev.data.fd, buffer, strlen(buffer), 0);
            } else if (nread == -1 &&
                       errno == EINTR) { // Signal interrupt while reading
              continue;
            } else if (nread == -1 &&
                       ((errno == EAGAIN) ||
                        (errno == EWOULDBLOCK))) { // Finished reading
              break;
            } else if (nread == 0) {
              printf("Client(eventfd=%d) disconnected.\n", ev.data.fd);
              close(ev.data.fd);
              break;
            }
          }
        }
      } else if (ev.events & EPOLLOUT) { // Write event
      } else {                           // error
        printf("Client(eventfd=%d) error.\n", ev.data.fd);
        close(ev.data.fd);
      }
    }
  }

  return 0;
}
