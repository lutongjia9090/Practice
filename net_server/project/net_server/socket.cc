// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutonjia@163.com)
//

#include "socket.h"
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int CreateSocket() {
  int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
  if (fd < 0) {
    printf("Failed to create socket.\n");
    return -1;
  }

  return fd;
}

Socket::Socket(int fd) : fd_(fd) {}

Socket::~Socket() { ::close(fd_); }

int Socket::GetFd() const { return fd_; }

void Socket::SetReuseAddr(bool on) {
  int opt = on ? 1 : 0;
  setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt,
             static_cast<socklen_t>(sizeof(opt)));
}

void Socket::SetReusePort(bool on) {
  int opt = on ? 1 : 0;
  setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt,
             static_cast<socklen_t>(sizeof(opt)));
}

void Socket::SetTcpNoDelay(bool on) {
  int opt = on ? 1 : 0;
  setsockopt(fd_, SOL_SOCKET, TCP_NODELAY, &opt,
             static_cast<socklen_t>(sizeof(opt)));
}

void Socket::SetKeepAlive(bool on) {
  int opt = on ? 1 : 0;
  setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &opt,
             static_cast<socklen_t>(sizeof(opt)));
}

void Socket::Bind(const sockaddr &server_addr) {
  if (::bind(fd_, &server_addr, sizeof(sockaddr)) < 0) {
    printf("Failed to bind server addr.");
    close(fd_);
    exit(-1);
  }
}

void Socket::Listen(int max_conn) {
  if (::listen(fd_, max_conn) < 0) {
    printf("Failed to listen server.");
    close(fd_);
    exit(-1);
  }
}

int Socket::Accept(InetAdress &client_addr) {
  struct sockaddr_in peer_addr;
  socklen_t len = sizeof(peer_addr);
  int client_fd =
      accept4(fd_, (struct sockaddr *)&peer_addr, &len, SOCK_NONBLOCK);

  client_addr.SetSockAddr(peer_addr);
  printf("accept a new client: fd=%d, ip=%s, port=%d.\n", client_fd,
         client_addr.GetSockIp(), client_addr.GetSockPort());

  return client_fd;
}
