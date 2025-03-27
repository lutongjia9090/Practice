// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutonjia@163.com)
//

#include "channel.h"
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
  Channel *server_ch = new Channel(server_sock.GetFd(), &epoll);
  server_ch->EnableReading();

  while (true) {
    std::vector<Channel *> chs = epoll.Loop();
    for (auto &ch : chs) {
      // client close, use `EPOLLIN` in some systems(recv() return 0)
      if (ch->GetREvent() & EPOLLRDHUP) {
        printf("Client(event fd=%d) disconnected.", ch->GetFd());
        close(ch->GetFd());
      } else if (ch->GetREvent() & EPOLLIN) {
        if (ch->GetFd() == server_sock.GetFd()) { // listen event
          InetAdress client_addr;
          Socket *client_sock = new Socket(server_sock.Accept(client_addr));

          Channel *client_ch = new Channel(client_sock->GetFd(), &epoll);
          client_ch->UseET();
          client_ch->EnableReading();
        } else { // read event
          char buffer[1024];
          while (true) {
            bzero(&buffer, sizeof(buffer));
            ssize_t nread = read(ch->GetFd(), buffer, sizeof(buffer));
            if (nread > 0) {
              printf("recv(eventfd=%d): %s\n", ch->GetFd(), buffer);
              send(ch->GetFd(), buffer, strlen(buffer), 0);
            } else if (nread == -1 &&
                       errno == EINTR) { // Signal interrupt while reading
              continue;
            } else if (nread == -1 &&
                       ((errno == EAGAIN) ||
                        (errno == EWOULDBLOCK))) { // Finished reading
              break;
            } else if (nread == 0) {
              printf("Client(eventfd=%d) disconnected.\n", ch->GetFd());
              close(ch->GetFd());
              break;
            }
          }
        }
      } else if (ch->GetREvent() & EPOLLOUT) { // Write event
      } else {                                 // error
        printf("Client(eventfd=%d) error.\n", ch->GetFd());
        close(ch->GetFd());
      }
    }
  }

  return 0;
}
