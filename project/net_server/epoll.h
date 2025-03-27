// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutonjia@163.com)
//

#pragma once
#include <sys/epoll.h>
#include <vector>

static constexpr int MAX_EVENTS = 100;

class Channel;

class Epoll {
public:
  Epoll();
  ~Epoll();

public:
  int GetFd() const;
  void UpdateChannel(Channel *ch);
  std::vector<Channel *> Loop(int timeout = -1);

private:
  int epoll_fd_ = -1;
  epoll_event evs_[MAX_EVENTS];
};
