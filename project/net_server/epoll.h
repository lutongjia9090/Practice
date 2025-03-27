// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutonjia@163.com)
//

#pragma once
#include <sys/epoll.h>
#include <vector>

static constexpr int MAX_EVENTS = 100;

class Epoll {
public:
  using events = std::vector<epoll_event>;

public:
  Epoll();
  ~Epoll();

public:
  int GetFd() const;
  void AddFd(int fd, uint32_t type);
  events Loop(int timeout);

private:
  int epoll_fd_ = -1;
  epoll_event evs_[MAX_EVENTS];
};
