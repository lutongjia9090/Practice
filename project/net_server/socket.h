// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutonjia@163.com)
//

#pragma once
#include "inet_adress.h"
#include <sys/socket.h>

int CreateSocket();

class Socket {
public:
  Socket(int fd);
  ~Socket();

public:
  int GetFd() const;
  void SetReuseAddr(bool on);
  void SetReusePort(bool on);
  void SetTcpNoDelay(bool on);
  void SetKeepAlive(bool on);
  void Bind(const sockaddr &server_addr);
  void Listen(int max_conn = 128); // hard code
  int Accept(InetAdress &client_addr);

private:
  const int fd_;
};
