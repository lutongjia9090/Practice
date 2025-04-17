// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutonjia@163.com)
//

#include "channel.h"
#include <stdio.h>
#include <stdlib.h>

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
  Channel *server_ch = new Channel(server_sock.GetFd(), &epoll, true);
  server_ch->EnableReading();

  while (true) {
    std::vector<Channel *> chs = epoll.Loop();
    for (auto &ch : chs) {
      ch->HandleEvent(&server_sock);
    }
  }

  return 0;
}
