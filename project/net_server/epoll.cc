// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutonjia@163.com)
//

#include "epoll.h"
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

Epoll::Epoll() {
  epoll_fd_ = epoll_create(1);
  if (epoll_fd_ == -1) {
    printf("Failed to create epoll.");
    exit(1);
  }
}

Epoll::~Epoll() { close(epoll_fd_); }

void Epoll::AddFd(int fd, uint32_t type) {
  epoll_event ev;
  ev.events = type;
  ev.data.fd = fd;
  epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
}

int Epoll::GetFd() const { return epoll_fd_; }

Epoll::events Epoll::Loop(int timeout) {
  events evs;
  bzero(evs_, sizeof(evs));
  int fds = epoll_wait(epoll_fd_, evs_, MAX_EVENTS, timeout);
  if (fds < 0) {
    printf("epoll_wait failed.\n");
    exit(-1);
  }

  if (fds == 0) {
    printf("epoll_wait timeout.\n");
    return evs;
  }

  for (int i = 0; i < fds; ++i) {
    evs.emplace_back(evs_[i]);
  }
  return evs;
}
