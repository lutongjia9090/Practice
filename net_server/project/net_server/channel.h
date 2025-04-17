// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutonjia@163.com)
//

#pragma once
#include "epoll.h"
#include "socket.h"
#include <cstdint>
#include <sys/types.h>

class Channel {
public:
  Channel(int fd, Epoll *ep, bool is_listen);
  ~Channel();

public:
  int GetFd() const;
  void UseET();
  void EnableReading();
  void SetInEpoll();
  void SetREvent(uint32_t ev);
  bool IsInepool() const;
  uint32_t GetEvent() const;
  uint32_t GetREvent() const;
  void HandleEvent(Socket *server_sock);

private:
  // channel1(fd1) --|
  // channel2(fd2) --| one epoll
  int fd_ = -1;
  Epoll *epoll_ = nullptr;
  bool in_epoll_ = false;
  uint32_t events_ = 0;  // monitor events
  uint32_t revents_ = 0; // return events
  bool is_listen_ = false;
};
